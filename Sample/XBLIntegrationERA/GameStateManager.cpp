#include "pch.h"
#include "GameStateManager.h"

GameStateManager::GameStateManager()
{
	m_State = TitleState_Start;
}


///
/*
	====Start====
	====Main Menu ====
	====Lobby====;====Option====
	====InGame====
	====Result==== ---- Back to Lobby----- =====Leaderboards=======
*/
///
void GameStateManager::Update(const XSF::GamepadReading& input)
{
	switch (m_State)
	{
	case TitleState_Start:
		if (input.IsAPressed())
		{
			m_State = TitleState_MainMenu;
			WaitForInput();
		}
		break;
	case TitleState_MainMenu:
		if (input.IsAPressed())
		{
			m_State = TitleState_Lobby;
			WaitForInput(); 
		}
		if (input.IsYPressed())
		{
			m_State = TitleState_Option_Menu;
			WaitForInput();
		}
		break;
	case TitleState_Lobby:
		if (input.IsAPressed())
		{
			m_State = TitleState_In_Gameplay;
			WaitForInput();
		}
		if (input.IsBPressed())
		{
			m_State = TitleState_MainMenu;
			WaitForInput();
		}
		break;
	case TitleState_Option_Menu:
		if (input.IsBPressed())
		{
			m_State = TitleState_MainMenu;
			WaitForInput();
		}
		if (input.IsAPressed())
		{
			WaitForInput();
		}
		break;
	case TitleState_In_Gameplay:
		if (input.IsRightShoulderPressed() && input.IsLeftShoulderPressed())
		{
			m_State = TitleState_Result;
			WaitForInput();
		}
		if (input.IsAPressed())
		{
			WaitForInput();
		}
		break;
	case TitleState_Result:
		if (input.IsAPressed())
		{
			m_State = TitleState_Lobby;
			WaitForInput();
		}
		if (input.IsYPressed())
		{
			m_State = TitleState_Leaderboard;
			WaitForInput();
		}
		break;
	case TitleState_Leaderboard:
		if (input.IsAPressed())
		{
			WaitForInput();
		}
		if (input.IsBPressed())
		{
			m_State = TitleState_Result;
			WaitForInput();
		}
		break;

	}
}

TitleState GameStateManager::GetGameState()
{
	return m_State;
}

void GameStateManager::WaitForInput()
{
	Sleep(1000);
}