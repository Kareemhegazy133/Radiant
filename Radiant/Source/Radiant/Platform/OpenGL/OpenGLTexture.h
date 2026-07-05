#pragma once

#include <glad/glad.h>

#include "Radiant/Renderer/Texture.h"

namespace Radiant {

	/**
	 * GL 2D texture using immutable storage with a single mip level —
	 * TextureSpecification::GenerateMips is not honored. Owns the GL texture
	 * handle.
	 */
	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification, Buffer data = Buffer());
		virtual ~OpenGLTexture2D();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetData(Buffer data) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == ((OpenGLTexture2D&)other).m_RendererID;
		}

	private:
		TextureSpecification m_Specification;

		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

}