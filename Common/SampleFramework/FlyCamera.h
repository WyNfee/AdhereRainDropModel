//--------------------------------------------------------------------------------------
// FlyCamera.h
//
// Class to provide first-person style free-look.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_FLY_CAMERA_H_INCLUDED
#define XSF_FLY_CAMERA_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

namespace XboxSampleFramework
{
	class FlyCamera
	{
	public:
		FlyCamera();
		~FlyCamera();

		void SetPosition(FXMVECTOR p) { m_Position = p; }
		XMVECTOR GetPosition() const { return m_Position; }

		void AddVelocity(FXMVECTOR v);
		void SetVelocity(FXMVECTOR v) { m_Velocity = v; }
		XMVECTOR GetVelocity() const { return m_Velocity; }

		void SetVelocityAttenuation(float va) { m_VelocityAttenuation = va; }
		float GetVelocityAttenuation() const { return m_VelocityAttenuation; }

		void SetHeading(float h);
		float GetHeading() const { return m_Heading; }

		void AddHeadingVelocity(float hv) { m_HeadingVelocity += hv; }
		void SetHeadingVelocity(float hv) { m_HeadingVelocity = hv; }
		float GetHeadingVelocity() const { return m_HeadingVelocity; }

		void SetHeadingVelocityAttenuation(float hva) { m_HeadingVelocityAttenuation = hva; }
		float GetHeadingVelocityAttenuation() const { return m_HeadingVelocityAttenuation; }

		void SetPitch(float p);
		float GetPitch() const { return m_Pitch; }

		void AddPitchVelocity(float pv) { m_PitchVelocity += pv; }
		void SetPitchVelocity(float pv) { m_PitchVelocity = pv; }
		float GetPitchVelocity() const { return m_PitchVelocity; }

		void SetPitchVelocityAttenuation(float pva) { m_PitchVelocityAttenuation = pva; }
		float GetPitchVelocityAttenuation() const { return m_PitchVelocityAttenuation; }

		const XMMATRIX& Update(float deltaTime);
		const XMMATRIX& GetTransform() const { return m_Transform; }
		const XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }

	private:

		XMMATRIX m_Transform;
		XMMATRIX m_ViewMatrix;
		XMVECTOR m_Position;
		XMVECTOR m_Velocity;
		float m_VelocityAttenuation;
		float m_Heading;
		float m_HeadingVelocity;
		float m_HeadingVelocityAttenuation;
		float m_Pitch;
		float m_PitchVelocity;
		float m_PitchVelocityAttenuation;
	};
}       // XboxSampleFramework

#endif

