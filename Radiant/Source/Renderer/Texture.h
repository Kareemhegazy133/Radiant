#pragma once

#include "Asset/Asset.h"

namespace Radiant {

	class Texture : public Asset
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path);

		static AssetType GetStaticType() { return AssetType::Texture2D; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	};

}