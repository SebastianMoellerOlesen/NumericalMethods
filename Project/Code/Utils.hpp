#pragma once

#include <random>
#include <raylib.h>
#include <raymath.h>

//-------------------------------------------------------------------------

inline Vector2 GetMousePosScaled()
{
    return GetMousePosition() * GetWindowScaleDPI();
}

//-------------------------------------------------------------------------

inline float RandomFloat( float min = -1.0f, float max = 1.0f )
{
    // Create the device once per frame, as it is expensive...
    static thread_local std::mt19937 gen{ std::random_device{}() };

    // Generate the distribution.
    std::uniform_real_distribution<float> dist( min, max );

    // Sample it using the generator.
    return dist( gen );
}

//-------------------------------------------------------------------------

inline Vector2 GetRandomDir()
{
    Vector2 vec{ RandomFloat(), RandomFloat() };

    // ULTRA UNLIKELY to be (0,0), so i won't bother checking...
    return Vector2Normalize( vec );
}

//-------------------------------------------------------------------------
