#include "pch.h"
#include "TextManager.h"

TextManager::TextManager(XSF::D3DDevice* device)
{
	XSF_ERROR_IF_FAILED(m_Fonts.Create(device, L"Media\\Fonts\\SegoeUI_16_Outline"));
}

void TextManager::Initialize()
{
	m_TitleName = L"XBL Integration Sample";
	m_NotificationTitle = L"Available Actions";
	m_StatusTitle = L"Current States";
	m_InfoText = L"";
	m_CurrentState = TitleState_All;
	m_PreviousState = TitleState_All;
}

void TextManager::RenderHeader(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics)
{
	m_Fonts.Begin(context, viewport, useFastSemantics);
	float ViewportWidth = viewport->Width;
	m_Fonts.DrawText(ViewportWidth / 5 * 2, 100, 0xffffffff, m_TitleName);
	m_Fonts.End();
}

void TextManager::RenderNotifications(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics)
{
	m_Fonts.Begin(context, viewport, useFastSemantics);
	m_Fonts.DrawText(100, 250, 0xffffffff, m_NotificationTitle);
	m_Fonts.DrawText(100, 300, 0xffffffff, m_NotificationText);
	m_Fonts.End();
}

void TextManager::RenderStages(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics)
{
	m_Fonts.Begin(context, viewport, useFastSemantics);
	float ViewportWidth = viewport->Width;
	m_Fonts.DrawText(ViewportWidth / 5 * 3, 250, 0xffffffff, m_StatusTitle);
	m_Fonts.DrawText(ViewportWidth / 5 * 3, 300, 0xffffffff, m_StatusText);
	m_Fonts.End();
}

void TextManager::RenderInformation(XSF::D3DDeviceContext * context, const D3D11_VIEWPORT * viewport, BOOL useFastSemantics)
{
	m_Fonts.Begin(context, viewport, useFastSemantics);
	float ViewportWidth = viewport->Width;
	m_Fonts.DrawText(ViewportWidth / 3, 400, 0xffffffff, m_InfoText);
	m_Fonts.End();
}

void TextManager::UpdateDisplayText(TitleState state)
{
	m_CurrentState = state;

	if (!IsStateChanged())
		return;

	switch (state)
	{
	case TitleState_Start:
		SetupTextForStartState();
		break;
	case TitleState_MainMenu:
		SetupTextForMainmenuState();
		break;
	case TitleState_Lobby:
		SetupTextForLobbyState();
		break;
	case TitleState_Option_Menu:
		SetupTextForOptionState();
		break;
	case TitleState_In_Gameplay:
		SetupTextForInGameState();
		break;
	case TitleState_Result:
		SetupTextForResultState();
		break;
	case TitleState_Leaderboard:
		SetupTextForLeaderboard();
		break;
	default:
		SetupTextForErrorState();
		break;
	}
}

bool TextManager::IsStateChanged()
{
	if (m_CurrentState != m_PreviousState)
		return true;
	return false;
}


void TextManager::SetupTextForStartState()
{
	m_NotificationText = L"Press (A) to go to Main Menu";
	m_StatusText = L"Now you are at Start";
}

void TextManager::SetupTextForMainmenuState()
{
	m_NotificationText = L"Press (A) to go to Lobby, Press (Y) to go to Option";
	m_StatusText = L"Now you are at Main Menu";
}

void TextManager::SetupTextForLobbyState()
{
	m_NotificationText = L"Press (A) to go to Game Play, Press (B) to go to Main Menu";
	m_StatusText = L"Now you are at Lobby";
}

void TextManager::SetupTextForOptionState()
{
	m_NotificationText = L"Press (A) to Access Storage,Press (B) to go to Main Menu";
	m_StatusText = L"Now you are at Option";
	SetupTextForInformation();
}

void TextManager::SetupTextForInGameState()
{
	m_NotificationText = L"Press (Right Shoulder & Left Shoulder) at the same time to go to Result, Press (A) to kill Alien Enemy";
	m_StatusText = L"Now you are at Game Play";
}

void TextManager::SetupTextForResultState()
{
	m_NotificationText = L"Press (A) to go to Lobby, Press (Y) to go to Leaderboards";
	m_StatusText = L"Now you are at Result";
}

void TextManager::SetupTextForErrorState()
{
	m_NotificationText = L"Restart the app";
	m_StatusText = L"Now you are at Error State, Restart the App";
}

void TextManager::SetupTextForNoUserLogginState()
{
	m_NotificationText = L"Restart the app";
	m_StatusText = L"No logged in User detected, Restart the App and login a user";
}

void TextManager::SetupTextForLeaderboard()
{
	m_NotificationText = L"Press (B) to back to Result, Press (A) to obtain Leaderboards";
	m_StatusText = L"Now you are at Leaderboards";
	SetupTextForInformation();
}

void TextManager::SetupTextForInformation()
{

}

void TextManager::Render(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics)
{
	RenderHeader(context,viewport,useFastSemantics);
	RenderNotifications(context, viewport, useFastSemantics);
	RenderStages(context, viewport, useFastSemantics);
}

