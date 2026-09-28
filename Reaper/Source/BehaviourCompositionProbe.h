#pragma once

#include <Radiant/Radiant.h>

#include <string>
#include <unordered_map>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/**
 * RAD-101 verification scaffolding: proves that several behaviours on ONE entity
 * receive their hooks in the defined order, tear down in reverse, and never run
 * after their own OnDestroy (retires with RAD-92).
 *
 * WHY THIS CANNOT BE PROVED BY WATCHING FOR A CRASH, which is the whole reason
 * it is shaped the way it is. Before RAD-97, a behaviour destroying its own
 * entity deleted the instance whose method was executing, so the bug class was a
 * use-after-free and the Debug allocator or a crash would eventually surface it.
 * After RAD-97 the instance survives to the reap, so the remaining failure —
 * a sibling receiving OnUpdate AFTER its own OnDestroy has run — corrupts
 * nothing. It faults in no configuration. ASan has nothing to say about it. The
 * only evidence is ORDER, so this probe records order and checks it.
 *
 * IT ASSERTS RATHER THAN NARRATES. Every check is silent on success and emits a
 * GAME_WARN naming the violated property on failure — GAME_WARN because
 * GAME_TRACE compiles to ((void)0) in Dist (Log.h), and Dist is exactly where
 * this has to run. A single summary line at the end reports pass/fail, and that
 * line is load-bearing in its own right: a probe that logs nothing is
 * indistinguishable from a probe that never executed.
 *
 * IT IS DRIVEN BY A STEP COUNTER, NOT A CHEAT KEY, unlike the RAD-95/97/100
 * probes. That is deliberate: key-driven scaffolding never runs in a headless
 * smoke launch, so it proves nothing unless a human is sitting there. This one
 * exercises itself on every run, in every configuration.
 *
 * Whatever replaces it under RAD-67 must keep BOTH properties — assert rather
 * than log, and run without a human — or it will quietly stop testing.
 *
 * Watch for, on any run: exactly one "[behaviour-probe] SUMMARY" line reporting
 * 0 failures. Any "FAIL:" line names the exact property that broke.
 */
namespace BehaviourProbe {

	// One fixture per probe entity. Keyed by the entity's tag rather than held on
	// the behaviours, so a check can outlive the instances it is checking - which
	// the teardown assertions need.
	struct Fixture
	{
		std::string CreateOrder;    // digits, in the order OnCreate fired
		std::string UpdateOrder;    // digits, reset every step by the ticker
		std::string DestroyOrder;   // digits, in the order OnDestroy fired
		bool Destroyed[3] = { false, false, false };

		// "The ENTITY is being torn down", which is NOT the same as "some
		// behaviour's OnDestroy ran" — a detach runs one OnDestroy and the siblings
		// must carry on. The two are indistinguishable from inside OnDestroy by
		// design (one hook, one meaning), so the probe records its own intent: #2
		// raises this immediately before calling Destroy().
		bool EntityTeardown = false;
	};

	inline int g_Step = 0;
	inline int g_Failures = 0;
	inline int g_Checks = 0;
	inline bool g_Summarised = false;
	inline std::unordered_map<std::string, Fixture> g_Fixtures;

	inline void Fail(const std::string& what)
	{
		++g_Failures;
		GAME_WARN("[behaviour-probe] FAIL: {}", what);
	}

	inline void Expect(bool condition, const std::string& what)
	{
		++g_Checks;
		if (!condition)
			Fail(what);
	}

	inline Fixture& Get(const std::string& tag) { return g_Fixtures[tag]; }

	// The schedule. Spread out so each event lands on a quiet step and the
	// assertions that follow it have a step of their own to observe in.
	constexpr int kDetachStep  = 4;
	constexpr int kDestroyStep = 8;
	constexpr int kSummaryStep = 12;
}

/**
 * Three distinct types from one definition - which is also what satisfies the
 * duplicate-rejection rule (RAD-101 D5): OrderProbe<1> and OrderProbe<2> are
 * different types, so attaching all three to one entity is legal.
 */
template<int N>
class OrderProbe : public EntityBehaviour
{
public:
	void OnCreate() override
	{
		BehaviourProbe::Fixture& f = Fixture();

		// Attach-then-detach before the walk arrives must run NEITHER hook. The
		// fixture for that entity therefore has to stay empty, and this is where a
		// leak would show.
		if (Tag() == "ProbeNoHooks")
			BehaviourProbe::Fail("OnCreate fired on a behaviour detached before its first update");

		f.CreateOrder += static_cast<char>('0' + N);
	}

	void OnUpdate(Timestep) override
	{
		BehaviourProbe::Fixture& f = Fixture();

		// THE CENTRAL ASSERTION OF THIS PROBE. Both halves are needed: the first
		// catches this behaviour running after its own detach, the second catches a
		// SIBLING running after the entity's teardown began. The second is the one
		// RAD-101 introduced and the one that faults in no configuration.
		if (f.Destroyed[N - 1])
			BehaviourProbe::Fail("OnUpdate ran after this behaviour's own OnDestroy");
		if (f.EntityTeardown)
			BehaviourProbe::Fail("OnUpdate ran on a sibling after the entity's teardown began");

		f.UpdateOrder += static_cast<char>('0' + N);

		// Behaviour #2 is the actor in both scenarios, and in both it does
		// observable work AFTER the call - which is what distinguishes this from a
		// probe that would pass against broken code (RAD-92's standing warning).
		if (N == 2 && Tag() == "ProbeDetach" && BehaviourProbe::g_Step == BehaviourProbe::kDetachStep)
		{
			GetOwner().RemoveBehaviour<OrderProbe<2>>();
			BehaviourProbe::Expect(GetOwner().IsValid(), "entity should survive one of its behaviours detaching");
			BehaviourProbe::Expect(GetOwner().GetBehaviour<OrderProbe<2>>() == nullptr,
				"a detached behaviour should be unfindable immediately");
			BehaviourProbe::Expect(GetOwner().GetBehaviour<OrderProbe<1>>() != nullptr,
				"siblings should survive a detach");
		}

		if (N == 2 && Tag() == "ProbeDestroy" && BehaviourProbe::g_Step == BehaviourProbe::kDestroyStep)
		{
			// Raised BEFORE the call, because the hooks it arms fire during it
			f.EntityTeardown = true;
			GetOwner().Destroy();
			// Running at all past this line is the RAD-97 guarantee this card
			// inherits; reading our own state proves the instance is intact
			BehaviourProbe::Expect(!GetOwner().IsValid(), "entity should report invalid immediately after Destroy");
			BehaviourProbe::Expect(f.DestroyOrder == "321", "OnDestroy should fire 3,2,1 during the mark");
		}
	}

	void OnDestroy() override
	{
		BehaviourProbe::Fixture& f = Fixture();

		if (Tag() == "ProbeNoHooks")
			BehaviourProbe::Fail("OnDestroy fired for an OnCreate that never ran");

		if (f.Destroyed[N - 1])
			BehaviourProbe::Fail("OnDestroy fired twice on the same behaviour");

		f.Destroyed[N - 1] = true;
		f.DestroyOrder += static_cast<char>('0' + N);
	}

private:
	const std::string& Tag() const { return GetOwner().GetComponent<MetadataComponent>().Tag; }
	BehaviourProbe::Fixture& Fixture() const { return BehaviourProbe::Get(Tag()); }
};

/**
 * Advances the step counter, checks the per-step ordering, and reports once.
 * Its own entity, attached FIRST so it leads the walk order and therefore sees a
 * complete previous step before anyone updates in the new one.
 */
class BehaviourProbeTicker : public EntityBehaviour
{
public:
	void OnUpdate(Timestep) override
	{
		using namespace BehaviourProbe;

		// Check the step that just finished, then open a new one
		if (g_Step > 0)
		{
			CheckAndReset("ProbeOrder", "123");

			// #2 leaves at kDetachStep, and note it still updates ON that step -
			// it detaches from inside its own OnUpdate, which has already been
			// counted, and #3 runs afterwards because only ONE behaviour left.
			// From the next step on, 1 and 3 only.
			CheckAndReset("ProbeDetach", g_Step <= kDetachStep ? "123" : "13");

			// On the destroy step itself the expected order is "12", NOT "123":
			// #1 and #2 update, #2 condemns the entity from inside its own
			// OnUpdate, and #3 is correctly skipped because the whole entity is
			// now dead. Getting this expectation wrong is what the first run of
			// this probe actually caught - in the probe, not the engine.
			const char* expectedDestroy = g_Step < kDestroyStep ? "123" : (g_Step == kDestroyStep ? "12" : "");
			CheckAndReset("ProbeDestroy", expectedDestroy);

			// Never anything here, ever
			CheckAndReset("ProbeNoHooks", "");
		}

		++g_Step;

		if (g_Step == kSummaryStep && !g_Summarised)
		{
			g_Summarised = true;
			Verify();
		}
	}

private:
	static void CheckAndReset(const char* tag, const std::string& expected)
	{
		BehaviourProbe::Fixture& f = BehaviourProbe::Get(tag);
		BehaviourProbe::Expect(f.UpdateOrder == expected,
			std::string("update order on '") + tag + "' was '" + f.UpdateOrder + "', expected '" + expected + "'");
		f.UpdateOrder.clear();
	}

	void Verify() const
	{
		using namespace BehaviourProbe;

		Expect(Get("ProbeOrder").CreateOrder == "123", "OnCreate should fire in attach order");
		Expect(Get("ProbeNoHooks").CreateOrder.empty(), "the detached-early behaviour should have no OnCreate");

		// Detach ran ONE OnDestroy, for #2 alone
		Expect(Get("ProbeDetach").DestroyOrder == "2", "detach should run OnDestroy for exactly the detached behaviour");

		// The destroyed entity tore down in reverse, once each
		Expect(Get("ProbeDestroy").DestroyOrder == "321", "entity teardown should run OnDestroy in reverse attach order");

		// The query family agrees with the table on a live three-behaviour entity
		if (Entity order = GetLevel().FindEntityByName("ProbeOrder"))
		{
			Expect(order.HasBehaviour<OrderProbe<1>>() && order.HasBehaviour<OrderProbe<2>>() && order.HasBehaviour<OrderProbe<3>>(),
				"HasBehaviour should be true for every attached behaviour");
			Expect(order.GetBehaviours().size() == 3, "GetBehaviours should yield every attached behaviour");
		}
		else
		{
			Fail("the ProbeOrder entity went missing before the summary");
		}

		if (Entity detached = GetLevel().FindEntityByName("ProbeDetach"))
		{
			Expect(!detached.HasBehaviour<OrderProbe<2>>(), "HasBehaviour should be false for a detached behaviour");
			Expect(detached.GetBehaviours().size() == 2, "GetBehaviours should not yield a detached behaviour");
		}

		// Duplicate attach: assert in Debug/Release, warn-and-return-existing in
		// Dist. Only the Dist arm is reachable without tripping the assert, so the
		// recovery path is verified exactly where it matters - the shipping build.
#ifdef RD_DIST
		if (Entity order = GetLevel().FindEntityByName("ProbeOrder"))
		{
			OrderProbe<1>* existing = order.GetBehaviour<OrderProbe<1>>();
			Expect(order.AddBehaviour<OrderProbe<1>>() == existing,
				"a duplicate attach should return the existing instance rather than a second one");
			Expect(order.GetBehaviours().size() == 3, "a rejected duplicate should not grow the behaviour list");
		}
#endif

		GAME_WARN("[behaviour-probe] SUMMARY: {0} checks, {1} failures{2}",
			g_Checks, g_Failures, g_Failures == 0 ? " - PASS" : " - SEE FAIL LINES ABOVE");
	}
};

/**
 * Builds the four fixtures. Called once from GameLayer::OnAttach, after the level
 * exists, on both the created-debug and loaded-asset paths.
 */
inline void SpawnBehaviourCompositionProbe(Level& level)
{
	// First, so it leads the walk order and observes complete steps
	level.CreateEntity("ProbeTicker").AddBehaviour<BehaviourProbeTicker>();

	for (const char* tag : { "ProbeOrder", "ProbeDetach", "ProbeDestroy" })
	{
		Entity e = level.CreateEntity(tag);
		e.AddBehaviour<OrderProbe<1>>();
		e.AddBehaviour<OrderProbe<2>>();
		e.AddBehaviour<OrderProbe<3>>();
	}

	// Attached and detached before the walk ever reaches it: neither hook may fire
	Entity noHooks = level.CreateEntity("ProbeNoHooks");
	noHooks.AddBehaviour<OrderProbe<1>>();
	noHooks.RemoveBehaviour<OrderProbe<1>>();
	BehaviourProbe::Expect(noHooks.GetBehaviour<OrderProbe<1>>() == nullptr,
		"a behaviour detached before its first update should be unfindable");
}
