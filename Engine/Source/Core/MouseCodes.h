#pragma once

namespace Engine
{
	using MouseCode = uint16_t;

	namespace Mouse
	{
		enum : MouseCode
		{
			// From SFML Mouse Button
			Left,       //!< The left mouse button
			Right,      //!< The right mouse button
			Middle,     //!< The middle (wheel) mouse button
			XButton1,   //!< The first extra mouse button
			XButton2,   //!< The second extra mouse button
		};
	}
}