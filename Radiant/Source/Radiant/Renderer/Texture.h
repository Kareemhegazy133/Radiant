#pragma once

#include "Radiant/Asset/Asset.h"

#include "Radiant/Core/Buffer.h"

namespace Radiant {

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F
	};

	/** Dimensions are in pixels. Defaults describe a 1x1 RGBA8 texture. */
	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true; // Currently ignored — the GL backend allocates a single mip level
	};

	/**
	 * GPU texture resource. An Asset, so Ref-counted and addressable by handle;
	 * like all renderer resources it must be released before the graphics
	 * context and is main-thread only.
	 */
	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		/**
		 * Uploads pixel data for the entire texture. `data` is a non-owning
		 * view: its size must equal the full texture size in bytes (asserted)
		 * and it is consumed synchronously — the caller keeps ownership and may
		 * free the memory on return.
		 */
		virtual void SetData(Buffer data) = 0;

		/** Binds the texture to the given texture slot for subsequent draws. */
		virtual void Bind(uint32_t slot = 0) const = 0;

		/** Backend texture handle (a GL object id today) — exposed for ImGui image display. */
		virtual uint32_t GetRendererID() const = 0;

		/** Identity comparison by backend handle — distinct textures with identical pixels are not equal. */
		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		/**
		 * Creates a texture for the active API. If `data` is non-empty it is
		 * uploaded immediately under the SetData contract (must cover the whole
		 * texture; caller keeps ownership).
		 */
		static Ref<Texture2D> Create(const TextureSpecification& specification, Buffer data = Buffer());

		static AssetType GetStaticType() { return AssetType::Texture2D; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	};

}