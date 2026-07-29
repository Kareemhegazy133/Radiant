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
			// Wherever the body went, its shapes went with it
			if (auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>())
				bc2d->RuntimeShapeId = 0;
			return;
		}

		b2DestroyBody(body);
		component.RuntimeBodyId = 0;

		// The body took its shapes with it — a surviving collider component
		// (gameplay may remove just the rigidbody and keep the collider) must
		// not keep a ticket to the wreckage
		if (auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>())
			bc2d->RuntimeShapeId = 0;
	}

	void PhysicsWorld2D::Teleport(Entity& entity, const glm::vec2& position, float rotation)
	{
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

		// A zero id means CreateBody failed (already ERROR-logged there):
		// survivable skip instead of feeding Box2D a null body
		if (rb2d.RuntimeBodyId == 0)
			return;

		b2BodyId body = b2LoadBodyId(rb2d.RuntimeBodyId);
		if (!b2Body_IsValid(body))
		{
			RADIANT_WARN("PhysicsWorld2D: Teleport called with a stale body id for entity {0}", entity.GetUUID());
			rb2d.RuntimeBodyId = 0;
			return;
		}

		// Teleports are rare, deliberate acts — this line is the audit trail
		// of every explicit ECS→Box2D push
		RADIANT_TRACE("PhysicsWorld2D: teleport entity {0} to ({1}, {2})", entity.GetUUID(), position.x, position.y);

		b2Body_SetTransform(body, { position.x, position.y }, b2MakeRot(rotation));
		// SetTransform does not wake: a sleeping body teleported into mid-air
		// would hang there until touched (UE's SetBodyTransform defaults
		// bAutoWake true for the same reason)
		b2Body_SetAwake(body, true);
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

		// The shape is BODY-LOCAL: no local rotation — the body already carries
		// the world rotation. (Passing the world rotation here doubled a
		// rotated entity's collider rotation; fixed by RAD-28.)
		b2Polygon box = b2MakeOffsetBox(
			component.Size.x * transform.Scale.x,
			component.Size.y * transform.Scale.y,
			{ component.Offset.x, component.Offset.y },
			b2Rot_identity);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = component.Density;
		// v3.1 moved friction/restitution into the shape's surface material;
		// the restitution THRESHOLD is world-level in v3 (b2WorldDef), so the
		// component's RestitutionThreshold maps to nothing and is removed in
		// Phase 3. enableContactEvents stays default-false until RAD-29 builds
		// the event drain.
		shapeDef.material.friction = component.Friction;
		shapeDef.material.restitution = component.Restitution;

		component.RuntimeShapeId = b2StoreShapeId(b2CreatePolygonShape(body, &shapeDef, &box));
	}

	void PhysicsWorld2D::DestroyBoxShape(Entity& entity, BoxCollider2DComponent& component)
	{
		// Never-created shape (CreateBoxShape failed or skipped), or the body
		// already died and took the shape with it (DestroyBody zeroes the
		// ticket): destroying nothing is a no-op, not a crash
		if (component.RuntimeShapeId == 0)
			return;

		b2ShapeId shape = b2LoadShapeId(component.RuntimeShapeId);

		// A stale ticket means a lifecycle path outside the entt signals
		// touched the shape — recover instead of crashing, but say so
		if (!b2Shape_IsValid(shape))
		{
			RADIANT_WARN("PhysicsWorld2D: DestroyBoxShape called with a stale shape id for entity {0}", entity.GetUUID());
			component.RuntimeShapeId = 0;
			return;
		}

		// true: the surviving body's mass must reflect the lost shape
		b2DestroyShape(shape, true);
		component.RuntimeShapeId = 0;
	}

	void PhysicsWorld2D::UpdateBoxShape(Entity& entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		// No live shape to refresh: CreateBoxShape failed (ERROR-logged
		// there), or the body died and took the shape with it — a state
		// mistake by the caller, warn and recover
		if (component.RuntimeShapeId == 0)
		{
			RADIANT_WARN("PhysicsWorld2D: UpdateBoxShape on entity {0} with no live shape - refresh skipped", entity.GetUUID());
			return;
		}

		b2ShapeId shape = b2LoadShapeId(component.RuntimeShapeId);
		if (!b2Shape_IsValid(shape))
		{
			RADIANT_WARN("PhysicsWorld2D: UpdateBoxShape called with a stale shape id for entity {0}", entity.GetUUID());
			component.RuntimeShapeId = 0;
			return;
		}

		RADIANT_TRACE("PhysicsWorld2D: collider refresh for entity {0}", entity.GetUUID());

		// Same geometry derivation as CreateBoxShape: body-local, no local
		// rotation
		b2Polygon box = b2MakeOffsetBox(
			component.Size.x * transform.Scale.x,
			component.Size.y * transform.Scale.y,
			{ component.Offset.x, component.Offset.y },
			b2Rot_identity);

		// In-place mutation keeps the shape id and its contacts — never
		// destroy/recreate (playbook §4). v3's geometry setter deliberately
		// leaves body mass untouched (box2d.h:637) and density defers it too:
		// one ApplyMassFromShapes at the end instead of three recomputes.
		b2Shape_SetPolygon(shape, &box);
		b2Shape_SetDensity(shape, component.Density, false);
		b2Shape_SetFriction(shape, component.Friction);
		b2Shape_SetRestitution(shape, component.Restitution);

		b2Body_ApplyMassFromShapes(b2Shape_GetBody(shape));
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

		// Drain the move events NOW: Box2D's event array is transient ("do
		// not store a reference", box2d.h:44) — copy into engine types before
		// anything else runs. clear() keeps capacity, so steady-state refills
		// allocate nothing.
		m_MoveEvents.clear();
		b2BodyEvents bodyEvents = b2World_GetBodyEvents(m_WorldId);
		m_MoveEvents.reserve(bodyEvents.moveCount);
		for (int i = 0; i < bodyEvents.moveCount; ++i)
		{
			const b2BodyMoveEvent& bodyMoveEvent = bodyEvents.moveEvents[i];

			// The UUID stamped into userData at CreateBody rides back out here
			UUID entityId(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(bodyMoveEvent.userData)));
			m_MoveEvents.push_back({ entityId, { bodyMoveEvent.transform.p.x, bodyMoveEvent.transform.p.y }, b2Rot_GetAngle(bodyMoveEvent.transform.q) });

			// A body earning sleep is the visible proof that contact
			// persistence survived the step (RAD-28's AC) — and rare enough
			// to log
			if (bodyMoveEvent.fellAsleep)
				RADIANT_TRACE("PhysicsWorld2D: body for entity {0} fell asleep", entityId);
		}
	}

}