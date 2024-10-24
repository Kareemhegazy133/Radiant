#include "rdpch.h"
#include "Font.h"

#include "MSDFData.h"

#include "Asset/AssetManager.h"
#include "Utilities/FileSystem.h"

namespace Radiant {

	struct FontInput {
		Buffer fontData;
		msdf_atlas::GlyphIdentifierType glyphIdentifierType;
		const char* charsetFilename;
		double fontScale;
		const char* fontName;
	};

	struct Configuration {
		msdf_atlas::ImageType imageType;
		msdf_atlas::ImageFormat imageFormat;
		msdf_atlas::YDirection yDirection;
		int width, height;
		double emSize;
		double pxRange;
		double angleThreshold;
		double miterLimit;
		void (*edgeColoring)(msdfgen::Shape&, double, unsigned long long);
		bool expensiveColoring;
		unsigned long long coloringSeed;
		msdf_atlas::GeneratorAttributes generatorAttributes;
	};

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define DEFAULT_MITER_LIMIT 1.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREADS 8

	namespace Utils {

		static std::filesystem::path GetCacheDirectory()
		{
			// TODO: Should make a general cache directory + FontAtlases
			return "Assets/Cache/FontAtlases";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::filesystem::path cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	struct AtlasHeader
	{
		uint32_t Type = 0;
		uint32_t Width = 0, Height = 0;
	};

	static bool TryReadFontAtlasFromCache(const std::string& fontName, float fontSize, AtlasHeader& header, void*& pixels, Buffer& storageBuffer)
	{
		std::string filename = std::format("{0}-{1}.rdfa", fontName, fontSize);
		std::filesystem::path filepath = Utils::GetCacheDirectory() / filename;

		if (std::filesystem::exists(filepath))
		{
			storageBuffer = FileSystem::ReadBytes(filepath);
			header = *storageBuffer.As<AtlasHeader>();
			pixels = (uint8_t*)storageBuffer.Data + sizeof(AtlasHeader);
			return true;
		}
		return false;
	}

	static void CacheFontAtlas(const std::string& fontName, float fontSize, AtlasHeader header, const void* pixels)
	{
		Utils::CreateCacheDirectoryIfNeeded();

		std::string filename = std::format("{0}-{1}.rdfa", fontName, fontSize);
		std::filesystem::path filepath = Utils::GetCacheDirectory() / filename;

		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
		if (!stream)
		{
			stream.close();
			RADIANT_ERROR("Font: Failed to cache font atlas to {0}", filepath.string());
			return;
		}

		stream.write((char*)&header, sizeof(AtlasHeader));
		stream.write((char*)pixels, header.Width * header.Height * 4);
	}

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, const Configuration& config)
	{
		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(config.width, config.height);
		generator.setAttributes(config.generatorAttributes);
		generator.setThreadCount(THREADS);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		AtlasHeader header;
		header.Width = bitmap.width;
		header.Height = bitmap.height;
		CacheFontAtlas(fontName, fontSize, header, bitmap.pixels);

		TextureSpecification spec;
		spec.Width = header.Width;
		spec.Height = header.Height;
		spec.Format = ImageFormat::RGBA8;
		spec.GenerateMips = false;

		Ref<Texture2D> texture = Texture2D::Create(spec, bitmap.pixels);
		return texture;
	}

	static Ref<Texture2D> CreateCachedAtlas(AtlasHeader header, const void* pixels)
	{
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA8;
		spec.Width = header.Width;
		spec.Height = header.Height;
		spec.GenerateMips = false;
		Ref<Texture2D> texture = Texture2D::Create(spec, pixels);
		return texture;
	}

	Font::Font(const std::filesystem::path& filepath)
		: m_MSDFData(new MSDFData())
	{
		m_Name = filepath.stem().string();

		Buffer buffer = FileSystem::ReadBytes(filepath);
		CreateAtlas(buffer);
		buffer.Release();
	}

	Font::Font(const std::string& name, Buffer buffer)
		: m_Name(name), m_MSDFData(new MSDFData())
	{
		CreateAtlas(buffer);
	}

	Font::~Font()
	{
		delete m_MSDFData;
		m_MSDFData = nullptr;
	}

	Ref<Font> Font::Init(const std::filesystem::path& filepath /*= "Assets/Fonts/OpenSans/OpenSans-Regular.ttf"*/)
	{
		s_DefaultFont = CreateRef<Font>(filepath);
		return s_DefaultFont;
	}

	Ref<Font> Font::GetDefaultFont()
	{
		RADIANT_ASSERT(s_DefaultFont, "Font: No default font has been initialized");
		return s_DefaultFont;
	}

	Ref<Font> Font::GetFontAssetForTextComponent(TextComponent& textComponent)
	{
		if (textComponent.FontHandle == s_DefaultFont->Handle || !AssetManager::IsAssetHandleValid(textComponent.FontHandle))
		{
			// Set the TextComponent's handle to the default font's handle we are using to serialize correctly
			textComponent.FontHandle = s_DefaultFont->Handle;
			return s_DefaultFont;
		}

		return AssetManager::GetAsset<Font>(textComponent.FontHandle);
	}

	void Font::CreateAtlas(Buffer buffer)
	{
		FontInput fontInput = { };
		Configuration config = { };
		fontInput.fontData = buffer;
		fontInput.glyphIdentifierType = msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT;
		fontInput.fontScale = -1;
		config.imageType = msdf_atlas::ImageType::MTSDF;
		config.imageFormat = msdf_atlas::ImageFormat::BINARY_FLOAT;
		config.yDirection = msdf_atlas::YDirection::BOTTOM_UP;
		config.edgeColoring = msdfgen::edgeColoringInkTrap;
		const char* imageFormatName = nullptr;
		int fixedWidth = -1, fixedHeight = -1;
		config.generatorAttributes.config.overlapSupport = true;
		config.generatorAttributes.scanlinePass = true;
		double minEmSize = 0;
		double rangeValue = 2.0;
		msdf_atlas::TightAtlasPacker::DimensionsConstraint atlasSizeConstraint = msdf_atlas::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;
		config.angleThreshold = DEFAULT_ANGLE_THRESHOLD;
		config.miterLimit = DEFAULT_MITER_LIMIT;

		config.emSize = 50.0;

		// Load fonts
		bool anyCodepointsAvailable = false;
		class FontHolder
		{
			msdfgen::FreetypeHandle* ft;
			msdfgen::FontHandle* font;
		public:
			FontHolder() : ft(msdfgen::initializeFreetype()), font(nullptr) {}
			~FontHolder()
			{
				if (ft)
				{
					if (font)
						msdfgen::destroyFont(font);
					msdfgen::deinitializeFreetype(ft);
				}
			}
			bool load(Buffer buffer)
			{
				if (ft && buffer)
				{
					if (font)
						msdfgen::destroyFont(font);
					if ((font = msdfgen::loadFontData(ft, buffer.As<const msdfgen::byte>(), int(buffer.Size))))
						return true;
				}
				return false;
			}
			operator msdfgen::FontHandle* () const
			{
				return font;
			}
		} font;

		bool success = font.load(fontInput.fontData);
		RADIANT_ASSERT(success, "Font: Could not load font");

		if (fontInput.fontScale <= 0)
			fontInput.fontScale = 1;

		// Load character set
		fontInput.glyphIdentifierType = msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT;
		msdf_atlas::Charset charset;

		// From imgui_draw.cpp
		static const uint32_t charsetRanges[] =
		{
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
			0x2DE0, 0x2DFF, // Cyrillic Extended-A
			0xA640, 0xA69F, // Cyrillic Extended-B
			0, 
		};

		for (int range = 0; range < 8; range += 2)
		{
			for (uint32_t c = charsetRanges[range]; c <= charsetRanges[range + 1]; c++)
				charset.add(c);
		}

		// Load Glyphs
		m_MSDFData->FontGeometry = msdf_atlas::FontGeometry(&m_MSDFData->Glyphs);
		int glyphsLoaded = -1;
		switch (fontInput.glyphIdentifierType)
		{
		case msdf_atlas::GlyphIdentifierType::GLYPH_INDEX:
			glyphsLoaded = m_MSDFData->FontGeometry.loadGlyphset(font, fontInput.fontScale, charset);
			break;
		case msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT:
			glyphsLoaded = m_MSDFData->FontGeometry.loadCharset(font, fontInput.fontScale, charset);
			anyCodepointsAvailable |= glyphsLoaded > 0;
			break;
		}
		RADIANT_ASSERT(glyphsLoaded >= 0);
		RADIANT_INFO("Font: Loaded {0} glyphs from font (out of {1})", glyphsLoaded, charset.size());

		// List missing glyphs
		if (glyphsLoaded < (int)charset.size())
		{
			RADIANT_WARN("Font: Font {0} is missing {1} {2}",
				m_Name,
				(int)charset.size() - glyphsLoaded,
				fontInput.glyphIdentifierType == msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT ? "codepoints" : "glyphs"
			);
		}

		if (fontInput.fontName)
			m_MSDFData->FontGeometry.setName(fontInput.fontName);

		// Determine final atlas dimensions, scale and range, pack glyphs
		double pxRange = rangeValue;
		bool fixedDimensions = fixedWidth >= 0 && fixedHeight >= 0;
		bool fixedScale = config.emSize > 0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		if (fixedDimensions)
			atlasPacker.setDimensions(fixedWidth, fixedHeight);
		else
			atlasPacker.setDimensionsConstraint(atlasSizeConstraint);
		atlasPacker.setPadding(config.imageType == msdf_atlas::ImageType::MSDF || config.imageType == msdf_atlas::ImageType::MTSDF ? 0 : -1);
		// TODO: In this case (if padding is -1), the border pixels of each glyph are black, but still computed. For floating-point output, this may play a role.
		if (fixedScale)
			atlasPacker.setScale(config.emSize);
		else
			atlasPacker.setMinimumScale(minEmSize);
		atlasPacker.setPixelRange(pxRange);
		atlasPacker.setMiterLimit(config.miterLimit);

		if (int remaining = atlasPacker.pack(m_MSDFData->Glyphs.data(), (int)m_MSDFData->Glyphs.size()))
		{
			if (remaining < 0)
			{
				RADIANT_ASSERT(false);
			}
			else
			{
				RADIANT_ERROR("Font: Error: Could not fit {0} out of {1} glyphs into the atlas.", remaining, (int)m_MSDFData->Glyphs.size());
				RADIANT_ASSERT(false);
			}
		}

		atlasPacker.getDimensions(config.width, config.height);
		RADIANT_ASSERT(config.width > 0 && config.height > 0);
		config.emSize = atlasPacker.getScale();
		config.pxRange = atlasPacker.getPixelRange();

		if (!fixedScale)
			RADIANT_INFO("Font: Glyph size: {0} pixels/EM", config.emSize);
		if (!fixedDimensions)
			RADIANT_INFO("Font: Atlas dimensions: {0} x {1}", config.width, config.height);

		// Edge Coloring
		if (config.imageType == msdf_atlas::ImageType::MSDF || config.imageType == msdf_atlas::ImageType::MTSDF)
		{
			if (config.expensiveColoring)
			{
				msdf_atlas::Workload([&glyphs = m_MSDFData->Glyphs, &config](int i, int threadNo) -> bool
					{
						unsigned long long glyphSeed = (LCG_MULTIPLIER * (config.coloringSeed ^ i) + LCG_INCREMENT) * !!config.coloringSeed;
						glyphs[i].edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
						return true;
					}, (int)m_MSDFData->Glyphs.size()).finish(THREADS);
			}
			else
			{
				unsigned long long glyphSeed = config.coloringSeed;
				for (msdf_atlas::GlyphGeometry& glyph : m_MSDFData->Glyphs)
				{
					glyphSeed *= LCG_MULTIPLIER;
					glyph.edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
				}
			}
		}

		// Check cache here
		Buffer storageBuffer;
		AtlasHeader header;
		void* pixels;
		if (TryReadFontAtlasFromCache(m_Name, (float)config.emSize, header, pixels, storageBuffer))
		{
			m_TextureAtlas = CreateCachedAtlas(header, pixels);
			storageBuffer.Release();
		}
		else
		{
			Ref<Texture2D> texture;
			switch (config.imageType)
			{
			case msdf_atlas::ImageType::MSDF:
				texture = CreateAndCacheAtlas<byte, float, 3, msdf_atlas::msdfGenerator>(m_Name, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
				break;
			case msdf_atlas::ImageType::MTSDF:
				texture = CreateAndCacheAtlas<byte, float, 4, msdf_atlas::mtsdfGenerator>(m_Name, (float)config.emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
				break;
			}

			m_TextureAtlas = texture;
		}
	}

	void Font::Shutdown()
	{
		s_DefaultFont.reset();
	}

}