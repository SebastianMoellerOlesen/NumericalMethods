#pragma once

#include <raylib.h>

#include <cstdint>
#include <vector>

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------

class Simulation
{

public:

    //-------------------------------------------------------------------------

    // The width and height are in physical pixels.
    // The size in logical pixels depends on the DPI of the display.
    Simulation( uint32_t height, uint32_t width, uint32_t ballCount, uint32_t targetFPS, float ballRadius );
    ~Simulation() = default;

    //-------------------------------------------------------------------------

    void Run() noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    void Update() noexcept;
    void Render() const noexcept;

    //-------------------------------------------------------------------------

    void HandleBorderCollision() noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    std::vector<Vector2> m_Positions;
    std::vector<Vector2> m_Velocities;
    std::vector<Color>   m_Colors;
    float                m_Radius;

    //-------------------------------------------------------------------------

    float m_RenderWidth;
    float m_RenderHeight;

    //-------------------------------------------------------------------------

    std::vector<Vector2> m_BorderP1;
    std::vector<Vector2> m_BorderP2;

    //-------------------------------------------------------------------------

    // This is our delta_t
    float m_DeltaTime;
};
