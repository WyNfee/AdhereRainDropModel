//--------------------------------------------------------------------------------------
// FlyCamera.h
//
// Class to provide first-person style free-look.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <pch.h>
#include "FlyCamera.h"

using namespace XboxSampleFramework;

float WrapHalfPi(float x);


FlyCamera::FlyCamera(void)
	: m_VelocityAttenuation(20)
	, m_Heading(0)
	, m_HeadingVelocity(0)
	, m_HeadingVelocityAttenuation(60)
	, m_Pitch(0)
	, m_PitchVelocity(0)
	, m_PitchVelocityAttenuation(60)
{
	m_Position = XMVectorZero();
	m_Velocity = XMVectorZero();
	m_Transform = XMMatrixIdentity();
	m_ViewMatrix = XMMatrixIdentity();

	Update(0);
}

FlyCamera::~FlyCamera(void)
{
}

void FlyCamera::AddVelocity(DirectX::FXMVECTOR v) 
{ 
	m_Velocity = m_Velocity + v; 
}

void FlyCamera::SetHeading(float h)
{
	m_Heading = WrapHalfPi(h);
}

void FlyCamera::SetPitch(float p)
{
	m_Pitch = Clamp(p, -XM_PI * 0.49f, XM_PI * 0.49f);
}

const XMMATRIX& FlyCamera::Update(float delta)
{
	m_Position += m_Velocity * delta;
	m_Velocity *= XMMax(0.0f, 1 - (delta * m_VelocityAttenuation));

	m_Pitch += m_PitchVelocity * delta;
	m_PitchVelocity *= XMMax(0.0f, 1 - (delta * m_PitchVelocityAttenuation));

	m_Heading += m_HeadingVelocity * delta;
	m_HeadingVelocity *= XMMax(0.0f, 1 - (delta * m_HeadingVelocityAttenuation));

	m_Heading = WrapHalfPi(m_Heading);
	m_Pitch = Clamp(m_Pitch, -XM_PI * 0.49f, XM_PI * 0.49f);

	XMMATRIX MH = XMMatrixRotationY(m_Heading);
	XMMATRIX MP = XMMatrixRotationX(m_Pitch);

	m_Transform = XMMatrixMultiply(MP, MH);
	m_Transform.r[3] = XMVectorSelect(g_XMOne, m_Position, g_XMSelect1110.v);
	
	XMVECTOR determinant = XMMatrixDeterminant(m_Transform);
	m_ViewMatrix = XMMatrixInverse(&determinant, m_Transform);

	return m_ViewMatrix;
}


// Helper functions

float WrapHalfPi(float x)
{
	return ModF(x + XM_PI, XM_2PI) - XM_PI;
}
