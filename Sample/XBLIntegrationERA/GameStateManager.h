#include "pch.h"

class GameStateManager
{
public:
	GameStateManager();
	void Update(const XSF::GamepadReading& input);
	TitleState GetGameState();
	void WaitForInput();

private:
	TitleState m_State;
};
