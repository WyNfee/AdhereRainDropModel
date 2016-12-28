//--------------------------------------------------------------------------------------
// NuiJointFilter.h
//
// This file contains Holt Double Exponential Smoothing filter for filtering Joints
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef XSF_NUIJOINTFILTER_H_INCLUDED
#define XSF_NUIJOINTFILTER_H_INCLUDED

#ifndef XSF_H_INCLUDED
#error  Please include SampleFramework.h before this file
#endif

#include <DirectXMath.h>
#include <queue>

#ifdef _XBOX_ONE

namespace XboxSampleFramework
{
    using namespace Windows::Kinect;

    typedef struct _NUI_TRANSFORM_SMOOTH_PARAMETERS
    {
        FLOAT   fSmoothing;             // [0..1], lower values closer to raw data
        FLOAT   fCorrection;            // [0..1], lower values slower to correct towards the raw data
        FLOAT   fPrediction;            // [0..n], the number of frames to predict into the future
        FLOAT   fJitterRadius;          // The radius in meters for jitter reduction
        FLOAT   fMaxDeviationRadius;    // The maximum radius in meters that filtered positions are allowed to deviate from raw data
    } NUI_TRANSFORM_SMOOTH_PARAMETERS;


    // Holt Double Exponential Smoothing filter
    class FilterDoubleExponentialData
    {
    public:
        XMVECTOR m_vRawPosition;
        XMVECTOR m_vFilteredPosition;
        XMVECTOR m_vTrend;
        DWORD    m_dwFrameCount;
    };

    class FilterDoubleExponential
    {
    public:
        FilterDoubleExponential() { Init(); }
        ~FilterDoubleExponential() { Shutdown(); }

        VOID Init( FLOAT fSmoothing = 0.25f, FLOAT fCorrection = 0.25f, FLOAT fPrediction = 0.25f, FLOAT fJitterRadius = 0.03f, FLOAT fMaxDeviationRadius = 0.05f )
        {
            m_pFilteredJoints = new XMVECTOR[ Body::JointCount ];
            m_pHistory = new FilterDoubleExponentialData[ Body::JointCount ];
            Reset( fSmoothing, fCorrection, fPrediction, fJitterRadius, fMaxDeviationRadius );
        }

        VOID Shutdown()
        {
            XSF_SAFE_DELETE_ARRAY( m_pFilteredJoints );
            XSF_SAFE_DELETE_ARRAY ( m_pHistory );
        }

        VOID Reset( FLOAT fSmoothing = 0.25f, FLOAT fCorrection = 0.25f, FLOAT fPrediction = 0.25f, FLOAT fJitterRadius = 0.03f, FLOAT fMaxDeviationRadius = 0.05f )
        {
            assert( m_pFilteredJoints );
            assert( m_pHistory );

            m_fMaxDeviationRadius = fMaxDeviationRadius; // Size of the max prediction radius Can snap back to noisy data when too high
            m_fSmoothing = fSmoothing;                   // How much smothing will occur.  Will lag when too high
            m_fCorrection = fCorrection;                 // How much to correct back from prediction.  Can make things springy
            m_fPrediction = fPrediction;                 // Amount of prediction into the future to use. Can over shoot when too high
            m_fJitterRadius = fJitterRadius;             // Size of the radius where jitter is removed. Can do too much smoothing when too high

            memset( m_pFilteredJoints, 0, sizeof( XMVECTOR ) * Body::JointCount );
            memset( m_pHistory, 0, sizeof( FilterDoubleExponentialData ) * Body::JointCount );
        }

        VOID Update( Body^ pBody );

        inline XMVECTOR* GetFilteredJoints() const { return m_pFilteredJoints; }

    private:
        XMVECTOR* m_pFilteredJoints;
        FilterDoubleExponentialData* m_pHistory;
        FLOAT m_fSmoothing;
        FLOAT m_fCorrection;
        FLOAT m_fPrediction;
        FLOAT m_fJitterRadius;
        FLOAT m_fMaxDeviationRadius;

        VOID Update( Body^ pBody, UINT i, NUI_TRANSFORM_SMOOTH_PARAMETERS smoothingParams );
    };
}

#endif

#endif