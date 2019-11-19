#pragma once
#include "AbstractComponent.h"


class btRigidBody;
class btCollisionShape;


class RigidBodyComponent final : public AbstractComponent
{
public:
	RigidBodyComponent(bool isStatic = false);
	~RigidBodyComponent();

	void Init() override;
	void Deinit() override;
	void Update() override {}

	float GetMass() const { return m_Mass; }
	void SetMass(float val) { m_Mass = val; }

	void SetCollisionShape(btCollisionShape* val) { m_pCollisionShape = val; }

	void SetPosition(const vec3 &pos);
	vec3 GetPosition();
	void SetRotation(const quat &rot);
	quat GetRotation();

	void ApplyImpulse(const vec3 &force, const vec3 &offset = vec3(0));
	void ApplyForce(const vec3 &force, const vec3 &offset = vec3(0));

private:
	btRigidBody* m_pBody = nullptr;

	btCollisionShape* m_pCollisionShape = nullptr;
	float m_Mass = 0;

	bool m_IsDynamic = false;
};