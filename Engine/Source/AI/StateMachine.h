#pragma once

enum class PlayerState;

namespace Engine {

    class StateMachine
    {
    public:
        using StateCallback = std::function<void()>;

        StateMachine() = default;
        ~StateMachine() = default;

        template<typename StateEnum>
        void AddState(StateEnum state,
            StateCallback onEnter = nullptr,
            StateCallback onUpdate = nullptr,
            StateCallback onExit = nullptr,
            bool IsBlocking = false);

        template<typename StateEnum>
        void SetState(StateEnum newState);

        void Update();

        void SetLocked(bool lock);

        bool IsLocked() const;

    private:
        struct State
        {
            StateCallback OnEnter;
            StateCallback OnUpdate;
            StateCallback OnExit;
            // Whether this state blocks from switching to other states till its done
            bool IsBlocking;
        };

        std::unordered_map<int, State> m_States;
        int m_CurrentState = -1;
        int m_PreviousState = -1;
        // Whether the state machine is locked because the current state is blocking
        bool m_IsLocked = false;
    };

}