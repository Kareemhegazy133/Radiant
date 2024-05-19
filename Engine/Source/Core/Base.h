#pragma once

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(x) std::bind(&GameApplication::x, this, std::placeholders::_1)