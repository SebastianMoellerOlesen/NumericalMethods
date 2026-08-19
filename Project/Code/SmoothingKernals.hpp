#pragma once

#include "raylib.h"
#include <cmath>

enum class Kernal
{
    Poly6, // Should be good for density...
    Spiky1,
    Spiky2,
    Spiky3,
};

// Todo, Change this to a param, so i can change it for the different updates...
static inline Kernal currentKernal = Kernal::Spiky2;

//-------------------------------------------------------------------------

constexpr inline float Poly6Kernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    // (r^2 - d ^2)^3
    float smoothingValue        = ( radius * radius - distance * distance ) * ( radius * radius - distance * distance ) * ( radius * radius - distance * distance );
    float normalizationConstant = ( PI * std::pow( radius, 8 ) ) / 4.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Poly6KernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = ( -2 * distance * ( radius * radius - distance * distance ) * ( radius * radius - distance * distance ) );
    float normalizationConstant = ( PI * std::pow( radius, 8 ) ) / 4.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Poly6KernalLaplacian2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = ( -2 * ( radius * radius - distance * distance ) * ( radius * radius - distance * distance ) ) + 4 * radius * radius * ( radius * radius - distance * distance );
    float normalizationConstant = ( PI * std::pow( radius, 8 ) ) / 4.0f;
    return smoothingValue / normalizationConstant;
}

//-------------------------------------------------------------------------

constexpr inline float SpikyKernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = radius - distance;
    float normalizationConstant = PI * radius * radius * radius / 3.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float SpikyKernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = -1.0f;
    float normalizationConstant = PI * radius * radius * radius / 3.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float SpikyKernalLaplacian2D( float radius, float distance ) noexcept
{
    return 0.0f;
}

//-------------------------------------------------------------------------

// The integral is equal to one
constexpr inline float Spiky2SmoothingKernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = ( radius - distance ) * ( radius - distance );
    float normalizationConstant = ( PI * radius * radius * radius * radius ) / 6;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Spiky2SmoothinKernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = 2 * distance - 2 * radius;
    float normalizationConstant = ( PI * radius * radius * radius * radius ) / 6;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Spiky2SmoothingKernalLaplace2D( float radius, float distance ) noexcept
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

constexpr inline float Spiky3Kernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = ( radius - distance ) * ( radius - distance ) * ( radius - distance );
    float normalizationConstant = ( PI * radius * radius * radius * radius * radius ) / 10.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Spiky3KernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = -3.0f * ( radius - distance ) * ( radius - distance );
    float normalizationConstant = ( PI * radius * radius * radius * radius * radius ) / 10.0f;
    return smoothingValue / normalizationConstant;
}

constexpr inline float Spiky3KernalLaplacian2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }
    float smoothingValue        = 6.0f * ( radius - distance );
    float normalizationConstant = ( PI * radius * radius * radius * radius * radius ) / 10.0f;
    return smoothingValue / normalizationConstant;
}

//-------------------------------------------------------------------------

constexpr inline float CurrentSmoothingKernal2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    switch ( currentKernal )
    {
        case Kernal::Poly6:
            return Poly6Kernal2D( radius, distance );
        case Kernal::Spiky1:
            return SpikyKernal2D( radius, distance );
        case Kernal::Spiky2:
            return Spiky2SmoothingKernal2D( radius, distance );
        case Kernal::Spiky3:
            return Spiky3Kernal2D( radius, distance );
        default: return 0.0f;
    }
}

constexpr inline float CurrentSmoothingKernalDerivative2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    switch ( currentKernal )
    {
        case Kernal::Poly6:
            return Poly6KernalDerivative2D( radius, distance );
        case Kernal::Spiky1:
            return SpikyKernalDerivative2D( radius, distance );
        case Kernal::Spiky2:
            return Spiky2SmoothinKernalDerivative2D( radius, distance );
        case Kernal::Spiky3:
            return Spiky3KernalDerivative2D( radius, distance );
        default: return 0.0f;
    }
}

constexpr inline float CurrentSmoothingKernalLaplacian2D( float radius, float distance ) noexcept
{
    if ( distance > radius )
    {
        return 0;
    }

    switch ( currentKernal )
    {
        case Kernal::Poly6:
            return Poly6KernalLaplacian2D( radius, distance );
        case Kernal::Spiky1:
            return SpikyKernalLaplacian2D( radius, distance );
        case Kernal::Spiky2:
            return Spiky2SmoothingKernalLaplace2D( radius, distance );
        case Kernal::Spiky3:
            return Spiky3KernalLaplacian2D( radius, distance );
        default: return 0.0f;
    }
}

//-------------------------------------------------------------------------
