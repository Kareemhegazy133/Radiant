#pragma once

#include "Renderer/Shader.h"

// TODO: REMOVE!
typedef unsigned int GLenum;

namespace Radiant {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader() override;

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetInt(const std::string& name, int value) const override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) const override;
		virtual void SetFloat(const std::string& name, float value) const override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) const override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) const override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) const override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) const override;

		virtual const std::string& GetName() const override { return m_Name; }

		void UploadUniformInt(const std::string& name, int value) const;
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count) const;

		void UploadUniformFloat(const std::string& name, float value) const;
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value) const;
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value) const;
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value) const;

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix) const;
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix) const;
	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
	private:
		uint32_t m_RendererID;
		std::string m_Name;
	};

}