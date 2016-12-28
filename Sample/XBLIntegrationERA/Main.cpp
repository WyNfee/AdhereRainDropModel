#include "pch.h"
#include "Main.h"


void Sample::Initialize()
{
    XSF::D3DDevice* const pDev = GetDevice();

	m_TextManager = new TextManager(pDev);
	m_TextManager->Initialize();

	m_GameStateManager = new GameStateManager();

}

void Sample::Update(float /*timeTotal*/, float /*timeDelta*/ )
{
    const XSF::GamepadReading& input = GetGamepadReading();

	m_GameStateManager->Update(input);
	m_TextManager->UpdateDisplayText(m_GameStateManager->GetGameState());
	
}

void Sample::Render()
{
    XSF::D3DDeviceContext* const pCtx = GetImmediateContext();

	m_TextManager->Render(pCtx, &GetBackbuffer().GetViewport(), SampleSettings().FastSemanticsEnabled());
}


XSF_DECLARE_ENTRY_POINT( Sample );
