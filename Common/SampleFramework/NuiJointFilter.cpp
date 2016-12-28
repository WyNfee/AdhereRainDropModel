//--------------------------------------------------------------------------------------
// NuiJointFilter.cpp
//
// This file contains Holt Double Exponential Smoothing filter for filtering Joints
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NuiJointFilter.h"

namespace XboxSampleFramework
{
    //-------------------------------------------------------------------------------------
    // Name: Lerp()
    // Desc: Linear interpolation between two floats
    //-------------------------------------------------------------------------------------
    inline FLOAT Lerp( FLOAT f1, FLOAT f2, FLOAT fBlend )
    {
        return f1 + ( f2 - f1 ) * fBlend;
    }
 
    //--------------------------------------------------------------------------------------
    // if joint is 0 it is not valid.
    //--------------------------------------------------------------------------------------
    inline BOOL JointPositionIsValid( XMVECTOR vJointPosition )
    {
        return ( XMVectorGetX( vJointPosition ) != 0.0f ||
                 XMVectorGetY( vJointPosition ) != 0.0f ||
                 XMVectorGetZ( vJointPosition ) != 0.0f );
    }

    //--------------------------------------------------------------------------------------
    // Implementation of a Holt Double Exponential Smoothing filter. The double exponential
    // smooths the curve and predicts.  There is also noise jitter removal. And maximum
    // prediction bounds.  The paramaters are commented in the init function.
    //--------------------------------------------------------------------------------------
    VOID FilterDoubleExponential::Update( Body^ pBody )
    {
#ifdef PROFILE
        PIXBeginEvent( 0, L"FilterUpdate" );
#endif

        XSF_ASSERT( pBody );

        // Check for divide by zero. Use an epsilon of a 10th of a millimeter
        m_fJitterRadius = XMMax( 0.0001f, m_fJitterRadius );

        NUI_TRANSFORM_SMOOTH_PARAMETERS SmoothingParams;

        for ( INT i = 0; i < Body::JointCount; i++ )
        {
            SmoothingParams.fSmoothing      = m_fSmoothing;
            SmoothingParams.fCorrection     = m_fCorrection;
            SmoothingParams.fPrediction     = m_fPrediction;
            SmoothingParams.fJitterRadius   = m_fJitterRadius;
            SmoothingParams.fMaxDeviationRadius = m_fMaxDeviationRadius;

            // If inferred, we smooth a bit more by using a bigger jitter radius
            Joint joint = pBody->Joints->Lookup( static_cast<JointType>( i ) );
            if ( joint.TrackingState == TrackingState::Inferred )
            {
                SmoothingParams.fJitterRadius       *= 2.0f;
                SmoothingParams.fMaxDeviationRadius *= 2.0f;
            }

            Update( pBody, i, SmoothingParams );
        }

#ifdef PROFILE
        PIXEndEvent();
#endif
    }

    VOID FilterDoubleExponential::Update( Body^ pBody, UINT i, NUI_TRANSFORM_SMOOTH_PARAMETERS smoothingParams )
    {
        XMVECTOR vPrevRawPosition;
        XMVECTOR vPrevFilteredPosition;
        XMVECTOR vPrevTrend;
        XMVECTOR vRawPosition;
        XMVECTOR vFilteredPosition;
        XMVECTOR vPredictedPosition;
        XMVECTOR vDiff;
        XMVECTOR vTrend;
        XMVECTOR vLength;
        FLOAT fDiff;
        BOOL bJointIsValid;

        const Joint joint = pBody->Joints->Lookup( static_cast<JointType>( i ) );

        vRawPosition            = XMVectorSet( joint.Position.X, joint.Position.Y, joint.Position.Z, 0.0f );
        vPrevFilteredPosition   = m_pHistory[i].m_vFilteredPosition;
        vPrevTrend              = m_pHistory[i].m_vTrend;
        vPrevRawPosition        = m_pHistory[i].m_vRawPosition;
        bJointIsValid           = JointPositionIsValid( vRawPosition );

        // If joint is invalid, reset the filter
        if ( !bJointIsValid )
        {
            m_pHistory[i].m_dwFrameCount = 0;
        }

        // Initial start values
        if ( m_pHistory[i].m_dwFrameCount == 0 )
        {
            vFilteredPosition = vRawPosition;
            vTrend = XMVectorZero();
            m_pHistory[i].m_dwFrameCount++;
        }
        else if ( m_pHistory[i].m_dwFrameCount == 1 )
        {
            vFilteredPosition = XMVectorScale( XMVectorAdd( vRawPosition, vPrevRawPosition ), 0.5f );
            vDiff = XMVectorSubtract( vFilteredPosition, vPrevFilteredPosition );
            vTrend = XMVectorAdd( XMVectorScale( vDiff, smoothingParams.fCorrection ), XMVectorScale( vPrevTrend, 1.0f - smoothingParams.fCorrection ) );
            m_pHistory[i].m_dwFrameCount++;
        }
        else
        {              
            // First apply jitter filter
            vDiff = XMVectorSubtract( vRawPosition, vPrevFilteredPosition );
            vLength = XMVector3Length( vDiff );
            fDiff = fabs( XMVectorGetX( vLength ) );

            if ( fDiff <= smoothingParams.fJitterRadius )
            {
                vFilteredPosition = XMVectorAdd( XMVectorScale( vRawPosition, fDiff/smoothingParams.fJitterRadius ),
                                                XMVectorScale( vPrevFilteredPosition, 1.0f - fDiff/smoothingParams.fJitterRadius ) );
            }
            else
            {
                vFilteredPosition = vRawPosition;
            }

            // Now the double exponential smoothing filter
            vFilteredPosition = XMVectorAdd( XMVectorScale( vFilteredPosition, 1.0f - smoothingParams.fSmoothing ),
                                            XMVectorScale( XMVectorAdd( vPrevFilteredPosition, vPrevTrend ), smoothingParams.fSmoothing ) );


            vDiff = XMVectorSubtract( vFilteredPosition, vPrevFilteredPosition );
            vTrend = XMVectorAdd( XMVectorScale( vDiff, smoothingParams.fCorrection ), XMVectorScale( vPrevTrend, 1.0f - smoothingParams.fCorrection ) );
        }      

        // Predict into the future to reduce latency
        vPredictedPosition = XMVectorAdd( vFilteredPosition, XMVectorScale( vTrend, smoothingParams.fPrediction ) );

        // Check that we are not too far away from raw data
        vDiff = XMVectorSubtract( vPredictedPosition, vRawPosition );
        vLength = XMVector3Length( vDiff );
        fDiff = fabs( XMVectorGetX( vLength ) );

        if ( fDiff > smoothingParams.fMaxDeviationRadius )
        {
            vPredictedPosition = XMVectorAdd( XMVectorScale( vPredictedPosition, smoothingParams.fMaxDeviationRadius/fDiff ),
                                             XMVectorScale( vRawPosition, 1.0f - smoothingParams.fMaxDeviationRadius/fDiff ) );
        }

        // Save the data from this frame
        m_pHistory[i].m_vRawPosition      = vRawPosition;
        m_pHistory[i].m_vFilteredPosition = vFilteredPosition;
        m_pHistory[i].m_vTrend            = vTrend;
        
        // Output the data
        m_pFilteredJoints[i] = vPredictedPosition;
        m_pFilteredJoints[i] = XMVectorSetW( m_pFilteredJoints[i], 1.0f );
    }
}
