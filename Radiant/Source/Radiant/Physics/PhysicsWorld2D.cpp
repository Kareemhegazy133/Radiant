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
		//
		// ALWAYS PASS __func__ FOR verb, never a string literal. A literal
		// duplicates the enclosing function's name, so it can be mistyped and it
		// silently stops matching the moment a verb is renamed - and a log line
		// naming the wrong verb is worse than no log line, because it sends the
		// reader to the wrong call site. __func__ makes both impossible rather
		// than unlikely, at a cost of one token. (Audited 2026-08-03 before the
		// switch: all 16 literals still matched, so this is preventive.)
		//
		// Unreal does the same thing, unwrapped, at the call site:
		//   UE_LOG(LogNavigation, Warning, TEXT("%hs Unhandled world type..."),
		//          __FUNCTION__);            // NavigationSystemBase.cpp:68
		// It uses __FUNCTION__ (qualified: "PhysicsWorld2D::SetTransform")
		// because its log lines do not already name the class; ours do, so the
		// qualified form would stutter and __func__ - also standard C++ rather
		// than a compiler extension - is the right half.
		//
		// Rejected: an enum of verb names. Type-safe but not correct-safe -
		// nothing stops one verb passing another's enumerator - and it costs an
		// enum entry plus a ToString case per verb to hand-maintain a table the
		// compiler already has, the O(N)-edits-per-feature pattern RAD-94
		// forbids. Also rejected: std::source_location as a defaulted parameter,
		// which would remove the argument entirely, but whose function_name() on
		// MSVC is the full decorated signature - "void __cdecl
		// Radiant::PhysicsWorld2D::SetTransform(class Radiant::Entity,const
		// struct glm::vec<2,float,0> &,float)" - where __func__ gives exactly
		// "SetTransform" (measured 2026-08-03).
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

		// The door every GAMEPLAY verb comes through: the component fetch plus
		// the resolution above, so a verb body is a resolve, a null check, and
		// one Box2D call. ResolveBodyId remains the door for callers that
		// already hold the component — the entt signal handlers, Teleport —
		// which is why this wraps it rather than repeating it: one policy, two
		// entry points, chosen by what the caller already has in hand.
		//
		// A missing RigidBody2DComponent is a different mistake from a dead id.
		// A dead id means something destroyed the body behind our back; a
		// missing component means the call site believes it is pushing a
		// physics object that was never one — a programmer error. So it WARNs
		// with the verb and entity (formatted, and RADIANT_WARN survives Dist)
		// and then asserts, breaking the debugger at the call that did it.
		// The two are separate statements because RADIANT_ASSERT takes no
		// format arguments (Assert.h) — the same split CreateBody uses.
		b2BodyId ResolveBody(Entity entity, const char* verb)
		{
			auto* rb2d = entity.TryGetComponent<RigidBody2DComponent>();
			if (!rb2d)
			{
				RADIANT_WARN("PhysicsWorld2D: {0} called on entity {1} with no RigidBody2DComponent", verb, entity.GetUUID());
				RADIANT_ASSERT(false, "Dynamics verb called on an entity with no RigidBody2DComponent");
				return b2_nullBodyId;
			}

			return ResolveBodyId(entity, rb2d->RuntimeBodyId, verb);
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

		b2BodyId body = ResolveBodyId(entity, component.RuntimeBodyId, __func__);
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

	void PhysicsWorld2D::SetTransform(Entity entity, const glm::vec2& position, float rotation)
	{
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();

		// Zero and stale body ids are both survivable skips here rather than a
		// null body handed to Box2D — see ResolveBodyId for which of them
		// warns
		b2BodyId body = ResolveBodyId(entity, rb2d.RuntimeBodyId, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_SetTransform(body, { position.x, position.y }, b2MakeRot(rotation));

		// Box2D's own SetTransform does not wake: a sleeping body placed in
		// mid-air would hang there until touched (UE's SetBodyTransform
		// defaults bAutoWake true for the same reason). Waking HERE rather than
		// leaving it to each caller is also what makes Teleport's velocity
		// policy correct by construction — see the header.
		b2Body_SetAwake(body, true);
	}

	void PhysicsWorld2D::Teleport(Entity entity, const glm::vec2& position, float rotation, TeleportType teleportType)
	{
		// Placement and the wake both live in SetTransform, so this verb is the
		// policy and the log and nothing else.
		SetTransform(entity, position, rotation);

		// Teleports are rare, deliberate acts — this line is the audit trail of
		// every explicit ECS→Box2D push, and is exactly what SetTransform omits
		// so a per-step caller does not drown the log (playbook §4). The mode is
		// named because the two are indistinguishable in the log otherwise, and
		// "why is it still spinning" is the question this log gets read to
		// answer.
		RADIANT_TRACE("PhysicsWorld2D: teleport entity {0} to ({1}, {2}) [{3}]",
			entity.GetUUID(), position.x, position.y,
			teleportType == TeleportType::ResetVelocity ? "reset velocity" : "keep velocity");

		if (teleportType != TeleportType::ResetVelocity)
			return;

		// Necessarily AFTER SetTransform's wake, which is now structural rather
		// than a comment to obey: v3 wakes a body from SetLinearVelocity only
		// when the velocity is NONZERO (body.c), and a sleeping body has no
		// b2BodyState to write into — so zeroing a sleeping body would be
		// dropped silently, on exactly the bodies that were asleep.
		//
		// Re-resolved rather than threaded out of SetTransform: handing a raw
		// b2BodyId back to a caller would put a vendor type in its hands to
		// save one array read, and this is the cold path (ResetVelocity only).
		// If SetTransform skipped on a zero/stale id, this skips identically
		// and the WARN was already issued there.
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		b2BodyId body = ResolveBodyId(entity, rb2d.RuntimeBodyId, __func__);
		if (B2_IS_NULL(body))
			return;

		// Velocity only: Box2D v3.1 has no ClearForces, so a force applied
		// earlier in this same fixed update still lands on the next step. The
		// caller asked for two contradictory things; we do not silently pick a
		// winner (see TeleportType).
		b2Body_SetLinearVelocity(body, b2Vec2_zero);
		b2Body_SetAngularVelocity(body, 0.0f);
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
		b2BodyId body = ResolveBodyId(entity, rb2d->RuntimeBodyId, __func__);
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
		b2ShapeId shape = ResolveShapeId(entity, component.RuntimeShapeId, __func__);
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

		b2ShapeId shape = ResolveShapeId(entity, component.RuntimeShapeId, __func__);
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

	// --- Dynamics verbs (RAD-90) -----------------------------------------
	//
	// Each one is a resolve, a null check, and one Box2D call. That uniformity
	// is the deliverable, not a coincidence: the guard policy lives once in
	// ResolveBody and the entity guard once in Level::ResolvePhysics, so
	// nothing here has a preamble to get subtly wrong (playbook §4).
	//
	// Every call passes wake = true and none of them exposes the flag: a
	// sleeping body IGNORES forces and impulses (box2d.h:290, :319), so
	// wake = false would be a silent no-op with no customer. UE's
	// SetLinearVelocity takes a bAutoWake it never reads — a parameter that
	// lies is worse than no parameter at all.

	void PhysicsWorld2D::ApplyForce(Entity entity, const glm::vec2& force)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_ApplyForceToCenter(body, { force.x, force.y }, true);
	}

	void PhysicsWorld2D::ApplyForceAtPoint(Entity entity, const glm::vec2& force, const glm::vec2& worldPoint)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		// Box2D derives the torque itself: cross(point - centreOfMass, force)
		b2Body_ApplyForce(body, { force.x, force.y }, { worldPoint.x, worldPoint.y }, true);
	}

	void PhysicsWorld2D::ApplyTorque(Entity entity, float torque)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_ApplyTorque(body, torque, true);
	}

	void PhysicsWorld2D::ApplyLinearImpulse(Entity entity, const glm::vec2& impulse)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_ApplyLinearImpulseToCenter(body, { impulse.x, impulse.y }, true);
	}

	void PhysicsWorld2D::ApplyLinearImpulseAtPoint(Entity entity, const glm::vec2& impulse, const glm::vec2& worldPoint)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_ApplyLinearImpulse(body, { impulse.x, impulse.y }, { worldPoint.x, worldPoint.y }, true);
	}

	void PhysicsWorld2D::ApplyAngularImpulse(Entity entity, float impulse)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_ApplyAngularImpulse(body, impulse, true);
	}

	void PhysicsWorld2D::SetLinearVelocity(Entity entity, const glm::vec2& velocity)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		// No wake argument on the v3 setters: they wake on a nonzero velocity
		// and only then (body.c). Setting {0,0} on a sleeping body is a no-op,
		// which is harmless — it is already stopped — but it does mean this is
		// not a way to wake something up.
		b2Body_SetLinearVelocity(body, { velocity.x, velocity.y });
	}

	void PhysicsWorld2D::SetAngularVelocity(Entity entity, float angularVelocity)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		b2Body_SetAngularVelocity(body, angularVelocity);
	}

	glm::vec2 PhysicsWorld2D::GetLinearVelocity(Entity entity)
	{
		// Non-const because this line can CLEAR a stale id — see the header
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return { 0.0f, 0.0f };

		b2Vec2 velocity = b2Body_GetLinearVelocity(body);
		return { velocity.x, velocity.y };
	}

	float PhysicsWorld2D::GetAngularVelocity(Entity entity)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return 0.0f;

		return b2Body_GetAngularVelocity(body);
	}

	void PhysicsWorld2D::MoveKinematic(Entity entity, const glm::vec2& position, float rotation, float fixedDelta)
	{
		b2BodyId body = ResolveBody(entity, __func__);
		if (B2_IS_NULL(body))
			return;

		// A design mistake rather than a runtime hazard: driving a DYNAMIC body
		// by target transform overwrites the velocities the solver just
		// computed, so it half-ignores gravity and slides. Asserted, not
		// warned — this verb runs once per platform per step, and a WARN here
		// would be 60 lines a second. b2Body_GetType is a pure read, so the
		// expression has no side effect to lose when asserts compile out in
		// Dist (playbook §8.5). Execution continues either way: Box2D handles
		// it coherently, and a false assert must never change behaviour.
		RADIANT_ASSERT(b2Body_GetType(body) == b2_kinematicBody,
			"MoveKinematic on a non-kinematic body - use forces or impulses to move a dynamic body");

		b2Body_SetTargetTransform(body, { { position.x, position.y }, b2MakeRot(rotation) }, fixedDelta);
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