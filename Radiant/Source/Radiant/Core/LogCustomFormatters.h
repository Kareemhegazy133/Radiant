#pragma once

#include "UUID.h"

// fmt — not std::format — is the customization point that matters here: spdlog
// uses its bundled fmt (SPDLOG_USE_STD_FORMAT is not defined), so a
// std::formatter specialization is invisible to every log macro.
#pragma warning(push, 0)
#include <spdlog/fmt/fmt.h>
#pragma warning(pop)

#include <glm/glm.hpp>

#include <filesystem>

namespace fmt {

	template <>
	struct formatter<Radiant::UUID> : formatter<uint64_t>
	{
		template <typename FormatContext>
		auto format(const Radiant::UUID id, FormatContext& ctx) const
		{
			return formatter<uint64_t>::format(static_cast<uint64_t>(id), ctx);
		}
	};

	template <>
	struct formatter<std::filesystem::path> : formatter<std::string>
	{
		template <typename FormatContext>
		auto format(const std::filesystem::path& path, FormatContext& ctx) const
		{
			return formatter<std::string>::format(path.string(), ctx);
		}
	};

	// The glm formatters accept an optional presentation spec: {:f} fixed
	// (default) or {:e} scientific
	template<>
	struct formatter<glm::vec2>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec2& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
				: format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
		}
	};

	template<>
	struct formatter<glm::vec3>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec3& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
				: format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
		}
	};

	template<>
	struct formatter<glm::vec4>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec4& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z, vec.w)
				: format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z, vec.w);
		}
	};

}

// Compile-time proof the specializations are wired to the stack the log macros
// actually use — if a formatter regresses (fmt upgrade, namespace change), the
// build breaks here instead of at some far-away log call site.
static_assert(fmt::is_formattable<Radiant::UUID>::value, "fmt cannot format Radiant::UUID - LogCustomFormatters.h is broken");
static_assert(fmt::is_formattable<std::filesystem::path>::value, "fmt cannot format std::filesystem::path - LogCustomFormatters.h is broken");
static_assert(fmt::is_formattable<glm::vec2>::value, "fmt cannot format glm::vec2 - LogCustomFormatters.h is broken");
static_assert(fmt::is_formattable<glm::vec3>::value, "fmt cannot format glm::vec3 - LogCustomFormatters.h is broken");
static_assert(fmt::is_formattable<glm::vec4>::value, "fmt cannot format glm::vec4 - LogCustomFormatters.h is broken");