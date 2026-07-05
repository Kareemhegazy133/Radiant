#pragma once

namespace Radiant {

	/**
	 * Compiled GPU shader program. Ref-counted like all renderer resources;
	 * must be released before the graphics context. Main-thread only.
	 *
	 * Create(filepath) reads one GLSL source file split into stages by `#type
	 * vertex` / `#type fragment` markers and derives the shader's name from the
	 * filename. Failure semantics: an unreadable file logs an error and yields
	 * an empty program; compile/link failures log the driver's info log and
	 * assert — shader source is treated as programmer-owned content, not
	 * recoverable config.
	 *
	 * The Set* interface resolves uniforms by string name on every call — a
	 * GL-ism the RHI v2 redesign removes; the Renderer2D shaders already bypass
	 * it via UniformBuffer blocks.
	 */
	class Shader : public RefCounted
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) const = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) const = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) const = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) const = 0;

		virtual const std::string& GetName() const = 0;

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);

	};

	/**
	 * Name-keyed shader cache with shared (Ref) ownership — shaders live as
	 * long as the library or any outstanding Ref. Add() asserts on duplicate
	 * names and Get() asserts on missing ones: lookups are programmer errors,
	 * not recoverable config.
	 */
	class ShaderLibrary
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const std::string& filepath);
		Ref<Shader> Load(const std::string& name, const std::string& filepath);

		Ref<Shader> Get(const std::string& name);

		bool Exists(const std::string& name) const;
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}