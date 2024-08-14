#pragma once

#include "Enginepch.h"

namespace Engine {

    // Node Status
    enum class Status
    {
        Success,
        Failure,
        Running
    };

    // Base Node Class
    class Node
    {
    public:
        virtual ~Node() = default;
        virtual Status Tick() = 0;
    };

    // Composite Node: Can have multiple children
    class Composite : public Node
    {
    protected:
        std::vector<Node*> children;
    public:
        void AddChild(Node* child) { children.push_back(child); }
    };

    // Selector Node: Succeeds if any child succeeds
    class Selector : public Composite
    {
    public:
        Status Tick() override
        {
            for (auto& child : children)
            {
                Status status = child->Tick();
                if (status == Status::Success)
                {
                    return Status::Success;
                }

                if (status == Status::Running)
                {
                    return Status::Running;
                }
            }
            return Status::Failure;
        }
    };

    // Sequence Node: Succeeds only if all children succeed
    class Sequence : public Composite
    {
    public:
        Status Tick() override
        {
            for (auto& child : children)
            {
                Status status = child->Tick();
                if (status == Status::Failure)
                {
                    return Status::Failure;
                }

                if (status == Status::Running)
                {
                    return Status::Running;
                }
            }
            return Status::Success;
        }
    };

    // Decorator Node: Modifies the behavior of a single child
    class Decorator : public Node
    {
    protected:
        Node* child;
    public:
        void SetChild(Node* node) { child = node; }
    };

    // Inverter Decorator: Inverts the result of its child
    class Inverter : public Decorator
    {
    public:
        Status Tick() override
        {
            Status status = child->Tick();
            if (status == Status::Success)
            {
                return Status::Failure;
            }

            if (status == Status::Failure)
            {
                return Status::Success;
            }
            return Status::Running;
        }
    };

    // Action Node: Performs an action, leaf node
    class Action : public Node
    {
    public:
        std::function<Status()> func;

        explicit Action(std::function<Status()> actionFunc) : func(actionFunc) {}

        Status Tick() override
        {
            return func();
        }
    };

    // Example Entity class that uses Behavior Tree to manage its behavior
    //class Entity {
    //public:
    //    Node* behaviorTreeRoot;

    //    void Update();
    //};

}
