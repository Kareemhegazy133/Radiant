#include "Enginepch.h"

#include "StateMachine.h"

namespace Engine {

    template<typename StateEnum>
    void StateMachine::AddState(StateEnum state,
        StateCallback onEnter,
        StateCallback onUpdate,
        StateCallback onExit,
        bool IsBlocking)
    {
        int stateKey = static_cast<int>(state);
        m_States[stateKey] = { onEnter, onUpdate, onExit, IsBlocking };
    }

    template<typename StateEnum>
    void StateMachine::SetState(StateEnum newState)
    {
        if (IsLocked())
        {
            //ENGINE_INFO("Cannot set new state, state machine is locked.");
            return;
        }   

        int stateKey = static_cast<int>(newState);

        if (m_CurrentState != -1 && m_States[m_CurrentState].OnExit)
            m_States[m_CurrentState].OnExit();

        m_PreviousState = m_CurrentState;
        m_CurrentState = stateKey;

        if (m_States[m_CurrentState].OnEnter)
            m_States[m_CurrentState].OnEnter();

        m_IsLocked = m_States[m_CurrentState].IsBlocking;
    }

    void StateMachine::Update()
    {
        if (m_CurrentState != -1 && m_States[m_CurrentState].OnUpdate)
        {
            m_States[m_CurrentState].OnUpdate();
        }
    }

    void StateMachine::SetLocked(bool lock)
    {
        m_IsLocked = lock;
    }

    bool StateMachine::IsLocked() const
    {
        return m_IsLocked;
    }


    // Explicit template instantiation
    template void StateMachine::AddState<PlayerState>(PlayerState, StateCallback, StateCallback, StateCallback, bool);
    template void StateMachine::SetState<PlayerState>(PlayerState);
}
