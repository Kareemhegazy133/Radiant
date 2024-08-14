#include "Enginepch.h"

#include "GOAP.h"

namespace Engine {

    // Check if the action is achievable given the current state
    bool Action::IsAchievable(const std::unordered_map<std::string, bool>& currentState) const
    {
        for (const auto& precondition : preconditions)
        {
            if (currentState.at(precondition.first) != precondition.second)
            {
                return false;
            }
        }
        return true;
    }

    // Planner: Find the optimal sequence of actions to achieve the goal
    std::vector<Action*> Planner::Plan(const std::unordered_map<std::string, bool>& initialState,
        const Goal& goal,
        const std::vector<Action*>& availableActions)
    {
        std::priority_queue<std::pair<float, std::vector<Action*>>,
            std::vector<std::pair<float, std::vector<Action*>>>,
            std::greater<>> frontier;

        frontier.emplace(0.0f, std::vector<Action*>{});

        std::unordered_map<std::string, bool> currentState = initialState;

        while (!frontier.empty())
        {
            auto [currentCost, currentPlan] = frontier.top();
            frontier.pop();

            // Check if the current state meets the goal conditions
            if (CheckPreconditions(currentState, goal.conditions))
            {
                return currentPlan;
            }

            for (auto* action : availableActions) {
                if (action->IsAchievable(currentState))
                {
                    std::vector<Action*> newPlan = currentPlan;
                    newPlan.push_back(action);

                    std::unordered_map<std::string, bool> newState = ApplyEffects(currentState, action->effects);
                    frontier.emplace(currentCost + action->cost, newPlan);
                    currentState = newState;
                }
            }
        }

        return {}; // Return an empty plan if no valid plan found
    }

    // Helper function to check if state meets preconditions
    bool Planner::CheckPreconditions(const std::unordered_map<std::string, bool>& state,
        const std::unordered_map<std::string, bool>& preconditions) const
    {
        for (const auto& precondition : preconditions)
        {
            if (state.at(precondition.first) != precondition.second)
            {
                return false;
            }
        }
        return true;
    }

    // Apply the effects of an action to the state
    std::unordered_map<std::string, bool> Planner::ApplyEffects(const std::unordered_map<std::string, bool>& state,
        const std::unordered_map<std::string, bool>& effects) const
    {
        std::unordered_map<std::string, bool> newState = state;
        for (const auto& effect : effects)
        {
            newState[effect.first] = effect.second;
        }
        return newState;
    }

    // EXAMPLE IMPLEMENTATION FOR AN ENTITY:
    // 
    // Entity: Update function to manage behavior
    //void Entity::Update() {
    //    if (currentPlan.empty() || !CheckPreconditions(currentState, currentGoal.conditions)) {
    //        currentPlan = planner.Plan(currentState, currentGoal, availableActions);
    //    }

    //    ExecutePlan();
    //}

    //// Execute the current plan
    //void Entity::ExecutePlan() {
    //    if (!currentPlan.empty()) {
    //        Action* action = currentPlan.front();
    //        if (action->Perform(*this)) {
    //            currentPlan.erase(currentPlan.begin()); // Remove the action from the plan after performing it
    //        }
    //    }
    //}

    // Example of a Derived Action (AttackAction.h and .cpp)
    // 
    //#pragma once
    //#include "GOAP.h"
    //
    //// Example Action: Attack
    //    class AttackAction : public Action {
    //    public:
    //        AttackAction()
    //            : Action("Attack", 2.0f) {
    //            preconditions["HasWeapon"] = true;
    //            effects["EnemyDefeated"] = true;
    //        }
    //
    //        bool Perform(Entity& entity) override {
    //            // Implement the logic for performing the attack
    //            return true; // Return true if the action was successful
    //        }
    //    };

}
