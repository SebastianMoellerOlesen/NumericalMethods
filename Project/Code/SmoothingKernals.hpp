#pragma once

#include "raylib.h"

//-------------------------------------------------------------------------

// The integral is equal to one
constexpr inline float SimpleSmoothingKernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    float smoothingValue        = ( radius - distance ) * ( radius - distance );
    float normalizationConstant = ( PI * radius * radius * radius * radius ) / 6;
    return smoothingValue / normalizationConstant;
}

constexpr inline float SimpleSmoothinKernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    float smoothingValue        = 2 * distance - 2 * radius;
    float normalizationConstant = ( PI * radius * radius * radius * radius ) / 6;
    return smoothingValue / normalizationConstant;
}

constexpr inline float SimpleSmoothingKernalLaplace2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    float smoothingValue        = 2;
    float normalizationConstant = ( PI * radius * radius * radius * radius ) / 6;
    return smoothingValue / normalizationConstant;
}

//-------------------------------------------------------------------------
