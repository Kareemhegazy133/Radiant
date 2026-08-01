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

		// Walks a contact event's shape back to the entity UUID stamped into
		// its body's userData at CreateBody. Returns 0 — the engine's "none"
		// id, as with AssetHandle — when the shape is already destroyed, which
		// an END event may legitimately name (v3 reports an end one step after
		// the destroy that caused it). Validity is checked BEFORE the id is
		// followed, so nothing here can dereference a dead slot.
		UUID ResolveEntityFromShape(b2ShapeId shape)
		{
			if (!b2Shape_IsValid(shape))
				return UUID(0);

			void* userData = b2Body_GetUserData(b2Shape_GetBody(shape));
			return UUID(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(userData)));
		}

		// Turns a component's packed id into a live body id, applying the one
		// stale-recovery policy every verb shares — this is the single home of
		// that policy, not a copy per verb (RAD-91). Unlike
		// ResolveEntityFromShape above, this is not a pure query: it logs, and
		// it clears packedId in place on the stale path.
		//
		// Two failures, deliberately different: a ZERO id returns null
		// SILENTLY (the body was never created, and CreateBody already
		// ERROR-logged why); a STALE id — nonzero, but no longer naming a live
		// body — warns, clears the field, and returns null. v3 makes that
		// second case DETECTABLE where v2's raw pointer would have dangled, so
		// we recover rather than crash, but we say so: a stale id means a
		// lifecycle path outside the entt signals touched the body.
		//
		// Callers test B2_IS_NULL and skip. Nothing here follows a dead slot.
		//
		// The Entity is passed rather than its UUID so that GetUUID() — a
		// component lookup — stays inside the cold warn branch. RADIANT_WARN
		// survives Dist (Core/Log.h), so its arguments are evaluated in every
		// config; hoisting that lookup to the call site would put it on the
		// hot path of every verb.
		b2BodyId ResolveBodyId(Entity entity, uint64_t& packedId, const char* verb)
		{
			if (packedId == 0)
				return b2_nullBodyId;

			b2BodyId body = b2LoadBodyId(packedId);
			if (!b2Body_IsValid(body))
			{
				RADIANT_WARN("PhysicsWorld2D: {0} called with a stale body id for entity {1}", verb, entity.GetUUID());
				packedId = 0;
				return b2_nullBodyId;
			}

			return body;
		}

		// The shape twin of ResolveBodyId — same contract, same two failure
		// cases. A zero shape id also means "the body died and took its shapes
		// with it", which DestroyBody records by zeroing the field; that is
		// still a silent no-op, not an error.
		b2ShapeId ResolveShapeId(Entity entity, uint64_t& packedId, const char* verb)
		{
			if (packedId == 0)
				return b2_nullShapeId;

			b2ShapeId shape = b2LoadShapeId(packedId);
			if (!b2Shape_IsValid(shape))
			{
				RADIANT_WARN("PhysicsWorld2D: {0} called with a stale shape id for entity {1}", verb, entity.GetUUID());
				packedId = 0;
				return b2_nullShapeId;
			}

			return shape;
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

	void PhysicsWorld2D::CreateBody(Entity entity, RigidBody2DComponent& component)
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

	void PhysicsWorld2D::DestroyBody(Entity entity, RigidBody2DComponent& component)
	{
		// Never-created body (CreateBody failed): destroying nothing is a
		// no-op, not a crash. This early return is load-bearing for the clear
		// below — a body that never existed cannot have orphaned a shape, so
		// there is no collider id to zero.
		if (component.RuntimeBodyId == 0)
			return;

		b2BodyId body = ResolveBodyId(entity, component.RuntimeBodyId, "DestroyBody");
		if (B2_IS_NON_NULL(body))
			b2DestroyBody(body);

		// Unconditional, and correct on both paths: either the body was just
		// destroyed, or it was already gone. Either way it took its shapes
		// with it — a surviving collider component (gameplay may remove just
		// the rigidbody and keep the collider) must not keep an id pointing at
		// the wreckage.
		component.RuntimeBodyId = 0;
		if (auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>())
			bc2d->RuntimeShapeId = 0;
	}

	void PhysicsWorld2D::Teleport(Entity entity, const glm::vec2& position, float rotation)
	{
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

		// Zero and stale body ids are both survivable skips here rather than a
		// null body handed to Box2D — see ResolveBodyId for which of them
		// warns
		b2BodyId body = ResolveBodyId(entity, rb2d.RuntimeBodyId, "Teleport");
		if (B2_IS_NULL(body))
			return;

		// Teleports are rare, deliberate acts — this line is the audit trail
		// of every explicit ECS→Box2D push
		RADIANT_TRACE("PhysicsWorld2D: teleport entity {0} to ({1}, {2})", entity.GetUUID(), position.x, position.y);

		b2Body_SetTransform(body, { position.x, position.y }, b2MakeRot(rotation));
		// SetTransform does not wake: a sleeping body teleported into mid-air
		// would hang there until touched (UE's SetBodyTransform defaults
		// bAutoWake true for the same reason)
		b2Body_SetAwake(body, true);
	}

	void PhysicsWorld2D::CreateBoxShape(Entity entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>();
		if (!rb2d)
		{
			RADIANT_ASSERT(false, "Entity must have a RigidBody2DComponent to create a box collider shape");
			return;
		}

		// A zero id keeps CreateBody's failure survivable and silent (it was
		// ERROR-logged there). A STALE id now warns and clears like every
		// other verb — before RAD-91 this one path failed silently and left
		// the dead id in place, which is how a policy starts drifting.
		b2BodyId body = ResolveBodyId(entity, rb2d->RuntimeBodyId, "CreateBoxShape");
		if (B2_IS_NULL(body))
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
		// Phase 3.
		shapeDef.material.friction = component.Friction;
		shapeDef.material.restitution = component.Restitution;
		// b2DefaultShapeDef leaves this false, so nothing reports contacts
		// unless the component opts in (RAD-29)
		shapeDef.enableContactEvents = component.EnableContactEvents;

		component.RuntimeShapeId = b2StoreShapeId(b2CreatePolygonShape(body, &shapeDef, &box));
	}

	void PhysicsWorld2D::DestroyBoxShape(Entity entity, BoxCollider2DComponent& component)
	{
		// Never-created shape (CreateBoxShape failed or skipped), or the body
		// already died and took the shape with it (DestroyBody zeroes the
		// id): destroying nothing is a no-op, not a crash. A stale id is the
		// separate case the resolver warns about.
		b2ShapeId shape = ResolveShapeId(entity, component.RuntimeShapeId, "DestroyBoxShape");
		if (B2_IS_NULL(shape))
			return;

		// true: the surviving body's mass must reflect the lost shape
		b2DestroyShape(shape, true);
		component.RuntimeShapeId = 0;
	}

	void PhysicsWorld2D::UpdateBoxShape(Entity entity, BoxCollider2DComponent& component)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		// This verb owns its own zero-id case, deliberately: refreshing a
		// shape that does not exist is a caller STATE mistake (the call site
		// believes it configured a shape it never got), where destroying
		// nothing is merely a no-op. The shared resolver stays silent on zero
		// for everyone else rather than growing a policy flag for this one
		// caller.
		if (component.RuntimeShapeId == 0)
		{
			RADIANT_WARN("PhysicsWorld2D: UpdateBoxShape on entity {0} with no live shape - refresh skipped", entity.GetUUID());
			return;
		}

		b2ShapeId shape = ResolveShapeId(entity, component.RuntimeShapeId, "UpdateBoxShape");
		if (B2_IS_NULL(shape))
			return;

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

		// Applies to contacts created from now on: a contact copies the flag
		// when it is BORN (contact.c:253), so toggling this while two shapes
		// are already touching yields an unpaired event — an End with no
		// matching Begin, or a Begin that never ends. Box2D documents the same
		// caveat (box2d.h:580) and offers no fix; treat the flag as spawn-time
		// configuration and this line as the completeness case.
		b2Shape_EnableContactEvents(shape, component.EnableContactEvents);

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

		// Same transient-array contract as the move events: copy out before
		// anything else runs. This copy is what makes contact dispatch safe —
		// once the records are engine-typed, gameplay can destroy bodies and
		// spawn entities freely, because nothing will touch Box2D's arrays
		// again this step (RAD-29).
		m_ContactEvents.clear();
		b2ContactEvents contactEvents = b2World_GetContactEvents(m_WorldId);
		m_ContactEvents.reserve(static_cast<size_t>(contactEvents.beginCount) + contactEvents.endCount);

		// Begins BEFORE ends, deliberately: one batch can hold both an End for
		// a contact that died and a Begin for the one replacing it, and Box2D
		// gives no cross-array ordering. Draining begins first means an overlap
		// counter goes 1 -> 2 -> 1 rather than 1 -> 0 -> 1, so "I left the
		// ground" never fires spuriously for a single step. This order is a
		// guarantee consumers may rely on, not an implementation detail.
		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			const b2ContactBeginTouchEvent& beginTouchEvent = contactEvents.beginEvents[i];
			// Nothing runs between b2World_Step returning and this loop, so a
			// shape here CANNOT have been destroyed — unlike the end events
			// below. An invalid id means the drain moved away from the step.
			RADIANT_ASSERT(b2Shape_IsValid(beginTouchEvent.shapeIdA) && b2Shape_IsValid(beginTouchEvent.shapeIdB),
				"Contact drain: begin event names a destroyed shape - is the drain still directly after b2World_Step?");
			m_ContactEvents.push_back({ ResolveEntityFromShape(beginTouchEvent.shapeIdA), ResolveEntityFromShape(beginTouchEvent.shapeIdB), ContactPhase::Begin });
		}

		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			const b2ContactEndTouchEvent& endTouchEvent = contactEvents.endEvents[i];
			// These CAN name destroyed shapes, and routinely do: v3 keeps two
			// end-event buffers and swaps them per step (world.c:807), so an
			// end caused by a destroy between the last two steps surfaces now,
			// with the shape long gone. ResolveEntityFromShape answers 0 for a dead
			// side; the surviving side still gets told (see ContactEvent).
			m_ContactEvents.push_back({ ResolveEntityFromShape(endTouchEvent.shapeIdA), ResolveEntityFromShape(endTouchEvent.shapeIdB), ContactPhase::End });
		}
	}

}