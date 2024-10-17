#include "rdpch.h"
#include "Font.h"

#include "MSDFData.h"

#include "Utilities/FileSystem.h"

namespace Radiant {

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		TextureSpecification spec;
		spec.Width = bitmap.width;
		spec.Height = bitmap.height;
		spec.Format = ImageFormat::RGB8;
		spec.GenerateMips = false;

		Ref<Texture2D> texture = Texture2D::Create(spec);

		texture->SetData(Buffer(bitmap.pixels));
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

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = m_MSDFData->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
				}, m_MSDFData->Glyphs.size()).finish(THREAD_COUNT);
		}
		else {
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : m_MSDFData->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		m_TextureAtlas = CreateAndCacheAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>("Test", (float)emSize, m_MSDFData->Glyphs, m_MSDFData->FontGeometry, width, height);

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

}