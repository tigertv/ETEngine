#pragma once
#include "AbstractComponent.h"

#include <EtCore/Content/AssetPointer.h>


class SpriteComponent final : public AbstractComponent
{
public:
	SpriteComponent(T_Hash const textureAsset, vec2 const& pivot = vec2(0), vec4 const& color = vec4(1));
	virtual ~SpriteComponent() = default;

	vec2 GetPivot() const { return m_Pivot; }
	vec4 const& GetColor() const { return m_Color; }

	void SetPivot(vec2 const& pivot);
	void SetColor(vec4 const& color);
	void SetTexture(T_Hash const textureAsset);

protected:
	void Init() override;
	void Deinit() override;
	void Update() override {}

private:

	core::T_SlotId m_Id = core::INVALID_SLOT_ID;
	T_Hash m_TextureAssetId;
	vec2 m_Pivot;
	vec4 m_Color;

	// -------------------------
	// Disabling default copy constructor and default 
	// assignment operator.
	// -------------------------
	SpriteComponent( const SpriteComponent& obj );
	SpriteComponent& operator=( const SpriteComponent& obj );
};
