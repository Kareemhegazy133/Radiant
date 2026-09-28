#pragma once

namespace Radiant {

	/** API-agnostic vocabulary for vertex attribute types, used to describe vertex buffer layouts. */
	enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	/** Size of a ShaderDataType in bytes. Asserts on ShaderDataType::None. */
	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return 4;
			case ShaderDataType::Float2:   return 4 * 2;
			case ShaderDataType::Float3:   return 4 * 3;
			case ShaderDataType::Float4:   return 4 * 4;
			case ShaderDataType::Mat3:     return 4 * 3 * 3;
			case ShaderDataType::Mat4:     return 4 * 4 * 4;
			case ShaderDataType::Int:      return 4;
			case ShaderDataType::Int2:     return 4 * 2;
			case ShaderDataType::Int3:     return 4 * 3;
			case ShaderDataType::Int4:     return 4 * 4;
			case ShaderDataType::Bool:     return 1;
		}

		RADIANT_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	/**
	 * One vertex attribute within a BufferLayout. Offset is computed by the
	 * owning layout when the layout is constructed — callers never set it.
	 */
	struct BufferElement
	{
		std::string Name;
		ShaderDataType Type;
		uint32_t Size;
		size_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
				case ShaderDataType::Float:   return 1;
				case ShaderDataType::Float2:  return 2;
				case ShaderDataType::Float3:  return 3;
				case ShaderDataType::Float4:  return 4;
				case ShaderDataType::Mat3:    return 3; // 3 * float3
				case ShaderDataType::Mat4:    return 4; // 4 * float4
				case ShaderDataType::Int:     return 1;
				case ShaderDataType::Int2:    return 2;
				case ShaderDataType::Int3:    return 3;
				case ShaderDataType::Int4:    return 4;
				case ShaderDataType::Bool:    return 1;
			}

			RADIANT_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};

	/**
	 * Ordered description of a vertex's attributes. Computes tightly-packed
	 * per-element offsets and the overall stride (bytes) at construction.
	 * Plain value type — copied freely, no GPU state.
	 */
	class BufferLayout
	{
	public:
		BufferLayout() {}

		BufferLayout(std::initializer_list<BufferElement> elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		void CalculateOffsetsAndStride()
		{
			size_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	/**
	 * GPU vertex buffer. Ref-counted like all renderer resources; must be
	 * released before the graphics context is destroyed. Main-thread only.
	 *
	 * Create(size) allocates uninitialized storage intended for per-frame
	 * streaming via SetData (the batcher's path); Create(vertices, size)
	 * uploads static data once, copying it during the call so the caller may
	 * free it on return. Sizes are in bytes.
	 */
	class VertexBuffer : public RefCounted
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		/**
		 * Uploads `size` bytes from `data` to the start of the buffer. The upload
		 * is synchronous — the caller keeps ownership of `data` and may free it
		 * on return. `size` must not exceed the size given at creation.
		 */
		virtual void SetData(const void* data, uint32_t size) = 0;

		virtual const BufferLayout& GetLayout() const = 0;
		/** Must be called before the buffer is added to a VertexArray (asserted there). */
		virtual void SetLayout(const BufferLayout& layout) = 0;

		static Ref<VertexBuffer> Create(uint32_t size);
		static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
	};

	/**
	 * GPU index buffer of 32-bit indices; `count` is the number of indices, not
	 * bytes. The source array is copied to the GPU during Create and may be
	 * freed on return. Ref-counted; must be released before the graphics
	 * context; main-thread only.
	 */
	class IndexBuffer : public RefCounted
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32_t GetCount() const = 0;

		static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
	};

}
