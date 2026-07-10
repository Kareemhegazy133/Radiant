#include "Radiant/rdpch.h"
#include "PhysicsWorld2D.h"

#include <box2d/box2d.h>

#include <mutex>

// Level.h chains Entity.h and EntityTemplates.h (Entity's template method
// definitions require a complete Level)
#include "Radiant/ECS/Level.h"
#include "Radiant/ECS/Components.h"

namespace Radiant {

	namespace {

		b2BodyType RigidBody2DTypeToBox2D(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DComponent::BodyType::Static:    return b2_staticBody;
				case RigidBody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
				case RigidBody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
			}

			RADIANT_ASSERT(false, "Unknown body type");
			return b2_staticBody;
		}

		// Box2D's internal assert callback: log through our sink, then return
		// nonzero so Box2D raises its breakpoint at the faulting call site.
		// RADIANT_ASSERT cannot take format arguments (Assert.h), so the hook
		// logs ERROR directly — same log-then-break machinery, richer message.
		// In Dist the Box2D vendor project defines NDEBUG, B2_ASSERT compiles
		// out, and this callback is never invoked.
		int Box2DAssertCallback(const char* condition, const char* fileName, int lineNumber)
		{
			RADIANT_ERROR("Box2D assertion failed: '{0}' at {1}:{2}", condition, fileName, lineNumber);
			return 1; // nonzero requests B2_BREAKPOINT at the call site
		}

		// b2SetAssertFcn is process-global library state — install exactly once,
		// not per world
		std::once_flag s_AssertHookInstalled;

		void InstallBox2DAssertHook()
		{
			std::call_once(s_AssertHookInstalled, []() { b2SetAssertFcn(&Box2DAssertCallback); });
		}

	}

	PhysicsWorld2D::PhysicsWorld2D(const std::string& debugName, const glm::vec2& gravity)
		: m_DebugName(debugName)
	{
		InstallBox2DAssertHook();

		// Defs must come from b2DefaultWorldDef(): v3 stamps a validation cookie
		// into every def and b2CreateWorld rejects one that is zero-initialized
		// by hand
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { gravity.x, gravity.y };

		m_WorldId = b2CreateWorld(&worldDef);
		if (!b2World_IsValid(m_WorldId))
		{
			// The null id comes back when the process world limit (128) is
			// exhausted — leaked Levels, a programmer error; ERROR survives Dist
			// so a later null-world crash has a cause on record
			RADIANT_ERROR("PhysicsWorld2D: b2CreateWorld failed for '{0}' - process world limit reached?", m_DebugName);
			RADIANT_ASSERT(false, "b2CreateWorld failed - process world limit reached?");
			return;
		}

		RADIANT_TRACE("PhysicsWorld2D created (world {0}) for '{1}'", m_WorldId.index1, m_DebugName);
	}

	PhysicsWorld2D::~PhysicsWorld2D()
	{
		// A failed constructor leaves the null id — nothing to destroy
		if (!b2World_IsValid(m_WorldId))
			return;

		RADIANT_TRACE("PhysicsWorld2D destroyed (world {0}) for '{1}'", m_WorldId.index1, m_DebugName);
		b2DestroyWorld(m_WorldId);
		m_WorldId = {};
	}

	void PhysicsWorld2D::CreateBody(Entity& entity, RigidBody2DComponent& component)
	{
		// Failed world creation (ERROR'd in the ctor) leaves the null id —
		// no-op instead of feeding Box2D an invalid world (UB in Dist, where
		// both assert layers compile out)
		if (B2_IS_NULL(m_WorldId))
			return;

		auto& transform = entity.GetComponent<TransformComponent>();

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = RigidBody2DTypeToBox2D(component.Type);
		bodyDef.position = { transform.Translation.x, transform.Translation.y };
		bodyDef.rotation = b2MakeRot(transform.Rotation.z);
		// v3 takes fixed rotation in the def — no post-create call like v2
		bodyDef.fixedRotation = component.FixedRotation;

		// The entity's UUID rides in the body's user data: RAD-29's event drain
		// resolves contact events back to entities through this
		bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entity.GetUUID()));

		b2BodyId body = b2CreateBody(m_WorldId, &bodyDef);
		if (B2_IS_NULL(body))
		{
			// The null id comes back when the world is locked (creation
			// mid-step) — ERROR survives Dist so the later no-body symptom has
			// a cause on record
			RADIANT_ERROR("PhysicsWorld2D: b2CreateBody failed for entity {0} - world locked (created during Step)?", entity.GetUUID());
			RADIANT_ASSERT(false, "b2CreateBody failed - world locked?");
			return;
		}

		component.RuntimeBodyId = b2StoreBodyId(body);
	}

	void PhysicsWorld2D::DestroyBody(Entity& entity, RigidBody2DComponent& component)
	{
		// Never-created body (CreateBody failed): destroying nothing is a
		// no-op, not a crash
		if (component.RuntimeBodyId == 0)
			return;

		b2BodyId body = b2LoadBodyId(component.RuntimeBodyId);

		// A stale ticket (generation mismatch, world already gone) is
		// DETECTABLE in v3 where the v2 pointer would have dangled — recover
		// instead of crashing, but say so: a stale id here means a lifecycle
		// path outside the entt signals touched the body
		if (!b2Body_IsValid(body))
		{
			RADIANT_WARN("PhysicsWorld2D: DestroyBody called with a stale body id for entity {0}", entity.GetUUID());
			component.RuntimeBodyId = 0;
			return;
		}

		b2DestroyBody(body);
		component.RuntimeBodyId = 0;
	}

	void PhysicsWorld2D::CreateBoxShape(Entity& entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>();
		if (!rb2d)
		{
			RADIANT_ASSERT(false, "Entity must have a RigidBody2DComponent to create a box collider shape");
			return;
		}

		b2BodyId body = b2LoadBodyId(rb2d->RuntimeBodyId);
		// CreateBody failure was ERROR-logged there; keep that failure
		// survivable here instead of feeding Box2D a null body
		if (!b2Body_IsValid(body))
			return;

		// 1:1 port of the v2 fixture (RAD-28): the box's LOCAL rotation repeats
		// the body's world rotation, so a rotated entity's collider turns twice
		// — a pre-existing quirk, invisible while everything spawns at rotation
		// 0; dies with RAD-28's sync rework
		b2Polygon box = b2MakeOffsetBox(
			component.Size.x * transform.Scale.x,
			component.Size.y * transform.Scale.y,
			{ component.Offset.x, component.Offset.y },
			b2MakeRot(transform.Rotation.z));

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = component.Density;
		// v3.1 moved friction/restitution into the shape's surface material;
		// the restitution THRESHOLD is world-level in v3 (b2WorldDef), so the
		// component's RestitutionThreshold maps to nothing and is removed in
		// Phase 3. enableContactEvents stays default-false until RAD-29 builds
		// the event drain.
		shapeDef.material.friction = component.Friction;
		shapeDef.material.restitution = component.Restitution;

		b2CreatePolygonShape(body, &shapeDef, &box);
	}

	void PhysicsWorld2D::Step(Timestep ts)
	{
		RADIANT_PROFILE_FUNCTION();

		// Null world (creation failed): no-op — see the failure-semantics
		// contract in the class doc
		if (B2_IS_NULL(m_WorldId))
			return;

		// v3's sub-steps replace v2's (6, 2) velocity/position iterations; 4 is
		// the library's recommended default (box2d.h:38)
		constexpr int subStepCount = 4;
		b2World_Step(m_WorldId, ts, subStepCount);
	}

	void PhysicsWorld2D::SubmitTransform(Entity& entity)
	{
		RADIANT_PROFILE_FUNCTION();

		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

		// A zero id means CreateBody failed (already ERROR-logged there): skip,
		// so that failure stays survivable instead of feeding Box2D a null body
		RADIANT_ASSERT(rb2d.RuntimeBodyId != 0, "Entity must have a body id to submit a transform for physics");
		if (rb2d.RuntimeBodyId == 0)
			return;

		b2BodyId body = b2LoadBodyId(rb2d.RuntimeBodyId);
		// A stale id per-step is a lifecycle bug — break loudly in dev rather
		// than warn-spam sixty times a second
		RADIANT_ASSERT(b2Body_IsValid(body), "SubmitTransform: stale body id - lifecycle bug");
		if (!b2Body_IsValid(body))
			return;

		// Known defect (RAD-28): v3's own docs call SetTransform "a teleport
		// ... fairly expensive" (box2d.h:247) — yet we teleport every body
		// every step. Push becomes explicit-teleport-only in RAD-28.
		b2Body_SetTransform(body, { transform.Translation.x, transform.Translation.y }, b2MakeRot(transform.Rotation.z));

		if (auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>())
		{
			// Known defect (RAD-28): per-step shape destroy/recreate wrecks
			// contact persistence, sleeping, and warm-starting, and allocates
			// every step. Destroying only the FIRST shape mirrors v2's
			// head-fixture removal — safe only while bodies carry one shape.
			b2ShapeId shape;
			if (b2Body_GetShapes(body, &shape, 1) > 0)
			{
				// Mass update deferred: CreateBoxShape refreshes it anyway
				// (b2ShapeDef.updateBodyMass defaults true)
				b2DestroyShape(shape, false);
			}
			CreateBoxShape(entity, *bc2d);
		}
	}

	void PhysicsWorld2D::RetrieveTransform(Entity& entity)
	{
		RADIANT_PROFILE_FUNCTION();

		auto& transform = entity.GetComponent<TransformComponent>();
		auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>();
		if (!rb2d)
		{
			RADIANT_WARN("PhysicsWorld2D: cannot retrieve transform for entity '{0}' - no RigidBody2DComponent", entity.GetComponent<MetadataComponent>().Tag);
			return;
		}

		// Same survivable-CreateBody-failure skip as SubmitTransform
		if (rb2d->RuntimeBodyId == 0)
			return;

		b2BodyId body = b2LoadBodyId(rb2d->RuntimeBodyId);
		if (!b2Body_IsValid(body))
			return;

		b2Vec2 position = b2Body_GetPosition(body);
		transform.Translation.x = position.x;
		transform.Translation.y = position.y;
		// v3 stores rotation as a cosine/sine pair; convert back to the Euler Z
		// radians the Transform owns
		transform.Rotation.z = b2Rot_GetAngle(b2Body_GetRotation(body));
	}

}