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
			case ImageFormat::RGBA32F:	return GL_RGBA;  // RGBA32F can use GL_RGBA for data format
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

		uint32_t bpp = 0;
		GLenum dataType = GL_UNSIGNED_BYTE;

		// Determine bytes per pixel and data type based on m_DataFormat
		switch (m_DataFormat)
		{
		case GL_RED:
			bpp = 1;  // R8 format (1 byte per pixel)
			break;
		case GL_RGB:
			bpp = 3;  // RGB8 format (3 bytes per pixel)
			break;
		case GL_RGBA:
			bpp = 4;  // RGBA8 format (4 bytes per pixel)
			break;
		case GL_RGBA32F:
			bpp = 16; // RGBA32F format (4 channels * 4 bytes per channel = 16 bytes per pixel)
			dataType = GL_FLOAT;
			break;
		default:
			RADIANT_ASSERT(false, "Unsupported texture data format!");
			return;
		}

		data = data.Copy(data.Data, m_Width * m_Height * bpp);

		// Ensure data size matches the expected texture size
		RADIANT_ASSERT(data.Size == m_Width * m_Height * bpp, "Data must match the entire texture size!");

		// Upload texture data
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, dataType, data.Data);
	}


	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		RADIANT_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_RendererID);
	}
}
