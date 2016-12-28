//--------------------------------------------------------------------------------------
// OrbitCamera.h
//
// A camera that's useful for 3D object inspection.
//
// Xbox Advanced Technology Group (ATG).
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_ORBIT_CAMERA_H_INCLUDED
#define XSF_ORBIT_CAMERA_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

namespace XboxSampleFramework
{
	class OrbitCamera
	{
	public:
		OrbitCamera();
		~OrbitCamera();

        // Move the focus position (but keep the camera where it is)
        void SetFocusPosition( _In_ FXMVECTOR focusPos ) { m_FocusPosition = focusPos; }
        XMVECTOR GetFocusPosition() const { return m_FocusPosition; }

        // Sets the focus position to the center of a bounding box.
        // The box is determined of the min/max of the supplied points.
        // The distance from the center is determined by the size of the
        // box.
        // This also updates the minimum distance so the camera cannot
        // intersect the box (but leaves the max distance unaffected.)
        void FocusOnBoundingBox(
            _In_reads_(numCorners) const XMFLOAT3* pBoxCorners,
            _In_ UINT numCorners );

        // Add rotational velocity
		void SetHeading( _In_ float h );
		float GetHeading() const { return m_Heading; }

		void AddHeadingVelocity( _In_ float hv ) { m_HeadingVelocity += hv; }
		void SetHeadingVelocity( _In_ float hv ) { m_HeadingVelocity = hv; }
		float GetHeadingVelocity() const { return m_HeadingVelocity; }

		void SetHeadingVelocityAttenuation( _In_ float hva ) { m_HeadingVelocityAttenuation = hva; }
		float GetHeadingVelocityAttenuation() const { return m_HeadingVelocityAttenuation; }

		void SetPitch( _In_ float p );
		float GetPitch() const { return m_Pitch; }

		void AddPitchVelocity( _In_ float pv ) { m_PitchVelocity += pv; }
		void SetPitchVelocity( _In_ float pv ) { m_PitchVelocity = pv; }
		float GetPitchVelocity() const { return m_PitchVelocity; }

		void SetPitchVelocityAttenuation( _In_ float pva ) { m_PitchVelocityAttenuation = pva; }
		float GetPitchVelocityAttenuation() const { return m_PitchVelocityAttenuation; }

        // Zoom in and out from the center (negative = towards, positive = towards)
        void SetDolly( _In_ float dolly );
        float GetDolly() const { return m_Dolly; }

		void AddDollyVelocity( _In_ float dolly );
		void SetDollyVelocity( _In_ float dolly ) { m_DollyVelocity = dolly; }
		float GetDollyVelocity() const { return m_DollyVelocity; }

		void SetDollyVelocityAttenuation( _In_ float va ) { m_DollyVelocityAttenuation = va; }
		float GetDollyVelocityAttenuation() const { return m_DollyVelocityAttenuation; }

        // Setting either of these will not recompute the matrix until the next Update.
        void SetDollyDistanceLimits( _In_ float minDist , _In_ float maxDist );
        float GetMaxDollyDistance() const { return m_DistanceMax; }
        float GetMinDollyDistance() const { return m_DistanceMin; }

        // Matrix generation/accessors
		const XMMATRIX& Update( _In_ float deltaTime );
		const XMMATRIX& GetTransform() const { return m_Transform; }
		const XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }
        XMVECTOR GetEyePosition() const { return m_EyePosition; }

	private:

		XMMATRIX m_Transform;
		XMMATRIX m_ViewMatrix;
		XMVECTOR m_FocusPosition;
        XMVECTOR m_EyePosition;
        float m_Dolly;
		float m_DollyVelocity;
		float m_DollyVelocityAttenuation;
        float m_DistanceMin;
        float m_DistanceMax;
		float m_Heading;
		float m_HeadingVelocity;
		float m_HeadingVelocityAttenuation;
		float m_Pitch;
		float m_PitchVelocity;
		float m_PitchVelocityAttenuation;
	};
}       // XboxSampleFramework

#endif

