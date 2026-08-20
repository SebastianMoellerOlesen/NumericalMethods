#pragma once

#include "raylib.h"
#include <vector>

//-------------------------------------------------------------------------

static inline std::vector<Vector2> AirfoilP1{
    { -0.500f, 0.000f },
    { -0.476f, 0.026f },
    { -0.405f, 0.046f },
    { -0.294f, 0.058f },
    { -0.155f, 0.060f },
    { 0.000f, 0.053f },
    { 0.155f, 0.041f },
    { 0.294f, 0.027f },
    { 0.405f, 0.014f },
    { 0.476f, 0.005f },
    { 0.500f, -0.001f }, // This is 0.001 for a small fix. If it's not there, there are some issues for the particles.
    { 0.476f, -0.005f },
    { 0.405f, -0.014f },
    { 0.294f, -0.027f },
    { 0.155f, -0.041f },
    { 0.000f, -0.053f },
    { -0.155f, -0.060f },
    { -0.294f, -0.058f },
    { -0.405f, -0.046f },
    { -0.476f, -0.026f },
};

static inline std::vector<Vector2> AirfoilP2{
    { -0.476f, 0.026f },
    { -0.405f, 0.046f },
    { -0.294f, 0.058f },
    { -0.155f, 0.060f },
    { 0.000f, 0.053f },
    { 0.155f, 0.041f },
    { 0.294f, 0.027f },
    { 0.405f, 0.014f },
    { 0.476f, 0.005f },
    { 0.500f, 0.001f }, // This is 0.001 for a small fix. If it's not there, there are some issues for the particles.
    { 0.476f, -0.005f },
    { 0.405f, -0.014f },
    { 0.294f, -0.027f },
    { 0.155f, -0.041f },
    { 0.000f, -0.053f },
    { -0.155f, -0.060f },
    { -0.294f, -0.058f },
    { -0.405f, -0.046f },
    { -0.476f, -0.026f },
    { -0.500f, 0.000f },
};

//-------------------------------------------------------------------------

static inline std::vector<Vector2> ObstacleP1{
    { 7.0f, 4.0f },
    { 10.0f, 5.5f },
    { 6.0f, 9.0f },
};

static inline std::vector<Vector2> ObstacleP2{
    { 0.0f, 2.5f },
    { 3.0f, 7.0f },
    { 0.0f, 8.0f },
};

// static inline std::vector<Vector2> ObstacleP1{
//     { 0.0f, 3.0f },
//     { 4.0f, 3.0f },
// };

// static inline std::vector<Vector2> ObstacleP2{
//     { 4.0f, 3.0f },
//     { 5.0f, 2.0f },
// };
