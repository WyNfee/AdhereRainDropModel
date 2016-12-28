#include "pch.h"

class TextManager
{
public:
	TextManager(XSF::D3DDevice* device);
	void Initialize();
	void UpdateDisplayText(TitleState state);
	void Render(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics);

private:
	bool IsStateChanged();

private:
	void RenderHeader(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics);
	void RenderNotifications(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics);
	void RenderStages(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics);
	void RenderInformation(XSF::D3DDeviceContext* context, const D3D11_VIEWPORT* viewport, BOOL useFastSemantics);

private:
	void SetupTextForStartState();
	void SetupTextForMainmenuState();
	void SetupTextForOptionState();
	void SetupTextForLobbyState();
	void SetupTextForInGameState();
	void SetupTextForResultState();
	void SetupTextForErrorState();
	void SetupTextForNoUserLogginState();
	void SetupTextForLeaderboard();
	void SetupTextForInformation();

private:
	XSF::BitmapFont   m_Fonts;
	WCHAR* m_NotificationTitle;
	WCHAR* m_NotificationText;
	WCHAR* m_StatusTitle;
	WCHAR* m_StatusText;
	WCHAR* m_TitleName;
	WCHAR* m_InfoText;
	TitleState m_PreviousState;
	TitleState m_CurrentState;
};
