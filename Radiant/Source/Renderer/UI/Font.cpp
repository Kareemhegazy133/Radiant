#include "rdpch.h"
#include "Font.h"

#include "MSDFData.h"

#include "Utilities/FileSystem.h"

namespace Radiant {

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define DEFAULT_MITER_LIMIT 1.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREADS 8

	namespace Utils {

		static std::filesystem::path GetCacheDirectory()
		{
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
		stream.write((char*)pixels, header.Width * header.Height * 3);
	}

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
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
		spec.Format = ImageFormat::RGB8;
		spec.GenerateMips = false;

		Ref<Texture2D> texture = Texture2D::Create(spec);

		texture->SetData(Buffer(bitmap.pixels));
		return texture;
	}

	static Ref<Texture2D> CreateCachedAtlas(AtlasHeader header, const void* pixels)
	{
		TextureSpecification spec;
		spec.Format = ImageFormat::RGB8;
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


	Ref<Font> Font::GetDefaultFont()
	{
		static Ref<Font> DefaultFont;
		if (!DefaultFont)
			DefaultFont = CreateRef<Font>("Assets/Fonts/OpenSans/OpenSans-Regular.ttf");

		return DefaultFont;
	}

	void Font::CreateAtlas(Buffer buffer)
	{
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		RADIANT_ASSERT(ft);

		msdfgen::FontHandle* font = msdfgen::loadFontData(ft, buffer.As<const msdfgen::byte>(), int(buffer.Size));
		if (!font)
		{
			RADIANT_ERROR("Font: Failed to load font");
			return;
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		// From imgui_draw.cpp
		static const CharsetRange charsetRanges[] =
		{
			{ 0x0020, 0x00FF }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		m_MSDFData->FontGeometry = msdf_atlas::FontGeometry(&m_MSDFData->Glyphs);
		int glyphsLoaded = m_MSDFData->FontGeometry.loadCharset(font, fontScale, charset);
		RADIANT_INFO("Font: Loaded {0} glyphs from font (out of {1})", glyphsLoaded, charset.size());

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		// atlasPacker.setDimensionsConstraint()
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(m_MSDFData->Glyphs.data(), (int)m_MSDFData->Glyphs.size());
		RADIANT_ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = m_MSDFData->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
				}, m_MSDFData->Glyphs.size()).finish(THREADS);
		}
		else {
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : m_MSDFData->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		// Check cache here
		Buffer storageBuffer;
		AtlasHeader header;
		void* pixels;
		if (TryReadFontAtlasFromCache(m_Name, (float)emSize, header, pixels, storageBuffer))
		{
			m_TextureAtlas = CreateCachedAtlas(header, pixels);
			storageBuffer.Release();
		}
		else
		{
			m_TextureAtlas = CreateAndCacheAtlas<byte, float, 3, msdf_atlas::msdfGenerator>(m_Name, (float)emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, width, height);
		}

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

}