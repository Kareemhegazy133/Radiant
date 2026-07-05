#pragma once

#include "Radiant/Core/Base.h"

namespace Radiant {

	/**
	 * Attachment pixel formats. RED_INTEGER exists for the entity-ID picking
	 * attachment (read back via ReadPixel) — pre-built editor plumbing; no
	 * current shader writes it (RAD-54).
	 */
	enum class FramebufferTextureFormat
	{
		None = 0,

		// Color
		RGBA8,
		RED_INTEGER,

		// Depth/stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	/** Size (pixels), attachment list, and sample count for Framebuffer::Create. */
	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false; // Declared but never read by the GL backend
	};

	/**
	 * Off-screen render target with color and depth attachments. Ref-counted
	 * like all renderer resources; must be released before the graphics
	 * context. Main-thread only.
	 *
	 * Bind() routes subsequent draws to this target and sets the viewport to
	 * its size; Unbind() returns to the default (window) framebuffer.
	 */
	class Framebuffer : public RefCounted
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		/**
		 * Destroys and recreates every attachment at the new size (pixels) —
		 * a GPU reallocation, never to be called per frame. Out-of-range sizes
		 * are rejected with a warning.
		 */
		virtual void Resize(uint32_t width, uint32_t height) = 0;
		/**
		 * Reads one pixel from a color attachment — a synchronous GPU
		 * read-back that stalls the pipeline. The GL backend hardwires the
		 * read format to integer, so this is only meaningful for RED_INTEGER
		 * attachments (entity-ID picking).
		 */
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		/** Clears one color attachment to an integer value (integer attachments only in the GL backend). */
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		/** Backend texture handle of a color attachment (a GL object id today) — exposed so ImGui can display the target. */
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};


}