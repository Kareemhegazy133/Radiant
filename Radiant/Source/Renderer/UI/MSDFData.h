#pragma once

#undef INFINITE
#include <vector>

#include "msdf-atlas-gen.h"

namespace Radiant {

	struct MSDFData
	{
		msdf_atlas::FontGeometry FontGeometry;
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
	};

}
