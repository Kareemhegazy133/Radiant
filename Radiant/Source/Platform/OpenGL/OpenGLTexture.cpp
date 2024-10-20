#include "rdpch.h"
#include "OpenGLTexture.h"

#include <stb_image.h>

namespace Radiant {

	namespace Utils {

		static GLenum RadiantImageFormatToGLDataFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::R8:		return GL_RED;
			case ImageFormat::RGB8:		return GL_RGB;
			case ImageFormat::RGBA8:	return GL_RGBA;
			case ImageFormat::RGBA32F:	return GL_RGBA32F;
			}

			RADIANT_ASSERT(false);
			return 0;
		}

		static GLenum RadiantImageFormatToGLInternalFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::R8:		return GL_R8;
			case ImageFormat::RGB8:		return GL_RGB8;
			case ImageFormat::RGBA8:	return GL_RGBA8;
			case ImageFormat::RGBA32F:	return GL_RGBA32F;
			}

			RADIANT_ASSERT(false);
			return 0;
		}

		static size_t GetMemorySize(ImageFormat format, uint32_t width, uint32_t height)
		{
			switch (format)
			{
			case ImageFormat::R8:			return width * height * 1;
			case ImageFormat::RGB8:			return width * height * 3;
			case ImageFormat::RGBA8:		return width * height * 4;
			case ImageFormat::RGBA32F:		return width * height * 4 * sizeof(float);
			}

			RADIANT_ASSERT(false);
			return 0;
		}

		static bool ValidateSpecification(const TextureSpecification& specification)
		{
			bool result = true;

			result = specification.Width > 0 && specification.Height > 0 && specification.Width < 65536 && specification.Height < 65536;
			RADIANT_ASSERT(result);

			return result;
		}

	}

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification, Buffer data)
		: m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height)
	{
		RADIANT_PROFILE_FUNCTION();

		m_InternalFormat = Utils::RadiantImageFormatToGLInternalFormat(m_Specification.Format);
		m_DataFormat = Utils::RadiantImageFormatToGLDataFormat(m_Specification.Format);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (data)
		{
			SetData(data);
		}
			
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		RADIANT_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(Buffer data)
	{
		RADIANT_PROFILE_FUNCTION();

		Utils::ValidateSpecification(m_Specification);
		auto size = (uint32_t)Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
		data = Buffer::Copy(data.Data, size);

		// Ensure data size matches the expected texture size
		RADIANT_ASSERT(data.Size == size, "Data must match the entire texture size!");
		// Upload texture data
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, m_DataFormat == GL_RGBA32F ? GL_FLOAT : GL_UNSIGNED_BYTE, data.Data);
	}


	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		RADIANT_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}
}
