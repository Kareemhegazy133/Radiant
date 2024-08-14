#pragma once

#include "Scene/Entities/Entity.h"

namespace Engine {

    // Action class representing a possible action the AI can perform
    class Action
    {
    public:
        std::string name;
        std::unordered_map<std::string, bool> preconditions;
        std::unordered_map<std::string, bool> effects;
        float cost;

        Action(const std::string& name, float cost)
            : name(name), cost(cost) {}

        virtual bool Perform(Entity& entity) = 0; // Override this in derived classes

        bool IsAchievable(const std::unordered_map<std::string, bool>& currentState) const;
    };

    // Goal class representing a desired state
    class Goal
    {
    public:
        std::string name;
        std::unordered_map<std::string, bool> conditions;

        Goal(const std::string& name, const std::unordered_map<std::string, bool>& conditions)
            : name(name), conditions(conditions) {}
    };

    // Planner class that finds the optimal sequence of actions to achieve a goal
    class Planner
    {
    public:
        std::vector<Action*> Plan(const std::unordered_map<std::string, bool>& initialState,
            const Goal& goal,
            const std::vector<Action*>& availableActions);

    private:
        bool CheckPreconditions(const std::unordered_map<std::string, bool>& state,
            const std::unordered_map<std::string, bool>& preconditions) const;

        std::unordered_map<std::string, bool> ApplyEffects(const std::unordered_map<std::string, bool>& state,
            const std::unordered_map<std::string, bool>& effects) const;
    };

    // EXAMPLE IMPLEMENTATION FOR AN ENTITY:
    // 
    // Entity class that uses GOAP to manage its behavior
    //class NPC {
    //public:
    //    Goal currentGoal;
    //    std::vector<Action*> availableActions;
    //    Planner planner;

    //    void Update();

    //private:
    //    std::unordered_map<std::string, bool> currentState;
    //    std::vector<Action*> currentPlan;

    //    void ExecutePlan();
    //};

}