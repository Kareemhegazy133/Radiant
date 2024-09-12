#pragma once

#include <Engine.h>

using namespace Engine;

class GameContext
{
public:
    static void SetLevel(Level* level) { s_Level = level; }
    static Level* GetLevel() { return s_Level; }

private:
    static Level* s_Level;
};
