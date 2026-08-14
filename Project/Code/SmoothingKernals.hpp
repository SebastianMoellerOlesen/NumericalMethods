#pragma once

#include "raylib.h"

//-------------------------------------------------------------------------

// The integral is equal to one
constexpr inline float SimpleSmoothingKernal2D( float radius, float distance )
{
    float smoothingValue        = radius * radius - distance * distance;
    float normalizationConstant = radius * radius * radius * PI / 3.0f;
    return smoothingValue > 0 ? smoothingValue / normalizationConstant : 0;
}

constexpr inline float SimpleSmoothinKernalDerivative2D( float radius, float distance )
{
    float normalizationConstant = radius * radius * radius / 3.0f;
    return distance > radius ? 0 : 2 * distance / normalizationConstant;
}

//-------------------------------------------------------------------------
