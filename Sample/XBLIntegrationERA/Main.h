#pragma once
#include "pch.h"
#include "TextManager.h"
#include "GameStateManager.h"

class Sample : public SampleFramework
{
public:
	virtual void Initialize() override;
	virtual void Update(float timeTotal, float timeDelta) override;
	virtual void Render() override;

private:
	TextManager* m_TextManager;
	GameStateManager* m_GameStateManager;
};
