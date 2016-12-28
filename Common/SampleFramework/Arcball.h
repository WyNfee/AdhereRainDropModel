//--------------------------------------------------------------------------------------
// Arcball.h
//
// A simple arcball class which manipulates a rotation matrix using controller input
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_ARCBALL_H
#define XSF_ARCBALL_H

namespace XboxSampleFramework
{
    
    //--------------------------------------------------------------------------------------
    // Arcball
    // A simple arcball which manipulates a rotation matrix using controller input
    //--------------------------------------------------------------------------------------

    class Arcball
    {
    public:
        
        //--------------------------------------------------------------------------------------
        // Constructor
        //--------------------------------------------------------------------------------------

        Arcball()
        {
            Reset();
        }

        //--------------------------------------------------------------------------------------
        // Name: Reset
        // Desc: Set the rotation of the arcball to identity
        //--------------------------------------------------------------------------------------
        
        inline void Reset()
        {
            m_currentRotation = XMQuaternionIdentity();
        }
       
        //--------------------------------------------------------------------------------------
        // Name: GetRotationMatrix
        // Desc: Get the rotation matrix of the arcball
        //--------------------------------------------------------------------------------------

        inline XMMATRIX GetRotationMatrix() const
        {
            return XMMatrixRotationQuaternion( m_currentRotation );
        }

        //--------------------------------------------------------------------------------------
        // Name: GetRotationQuaternion
        // Desc: Get the rotation quaternion of the arcball
        //--------------------------------------------------------------------------------------
        
        inline XMVECTOR GetRotationQuaternion() const
        {
            return m_currentRotation;
        }

        //--------------------------------------------------------------------------------------
        // Name: SetRotationQuaternion
        // Desc: Set the rotation quaternion of the arcball
        //--------------------------------------------------------------------------------------

        inline void SetRotationQuaternion( const XMVECTOR& q )
        {
            m_currentRotation = q;
        }

        //--------------------------------------------------------------------------------------
        // Name: Rotate
        // Desc: Rotate the arcball as if it is a trackball
        //
        //       Example invocation: 
        //          m_arcballWorld.Rotate( -input->NormalizedLeftThumbstickX * scale, -input->NormalizedLeftThumbstickY * scale );
        //
        //       where m_arcballWorld is an instance of this class and scale controls how fast 
        //       the rotation is
        //--------------------------------------------------------------------------------------

        void Rotate(float x, float y)
        {
            float z = 0.0f;
            float mag = x * x + y * y;
            if( mag > 1.0f )
            {
                float scale = 1.0f / sqrtf( mag );
                x *= scale;
                y *= scale;
            }
            else
            {
                z = sqrtf( 1.0f - mag );
            }

            const XMVECTOR from = XMVectorSet(0, 0, 1, 0);
            XMVECTOR to = XMVectorSet(x, y, z, 0);
            XMVECTOR part = XMVector3Cross(from, to);
            XMVECTOR dot = XMVector3Dot(from, to);
            part = XMVectorSetW(part, XMVectorGetX(dot));
            
            m_currentRotation = XMQuaternionMultiply( m_currentRotation, part );
        }

    protected:
        XMVECTOR m_currentRotation;
    };

}   // namespace XboxSampleFramework

#endif