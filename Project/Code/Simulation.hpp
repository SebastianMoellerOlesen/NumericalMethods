#pragma once

#include <raylib.h>

#include <cstdint>
#include <vector>

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

    // Note: We might just want to keep track of the densities for now.
    // Then we could add methods to convert from density to others, as long as that is possible.

    // When current, the data in the class is what is refered to.
    void UpdateDensities() noexcept;       // Currently uses an explicit method, with the current pos, and density.
    void UpdateDensityGradient() noexcept; // Currently uses an explicit method, with the current pos, and density.

    //-------------------------------------------------------------------------

    float   CalculateDensity( Vector2 location ) const noexcept;
    Vector2 CalculateDensityGradiant( Vector2 location ) const noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    uint32_t m_ParticleCount;
    float    m_DrawRadius;

    //-------------------------------------------------------------------------

    std::vector<Vector2> m_Positions;
    std::vector<Vector2> m_Velocities;
    std::vector<float>   m_Densities;
    std::vector<Vector2> m_DensityGradients;
    std::vector<float>   m_Masses;    // Const, could be varied.
    std::vector<float>   m_Pressures; // Const, could be varied.
    std::vector<Color>   m_Colors;

    //-------------------------------------------------------------------------

    float m_InfluenceRadius{ 30.0f }; // How far out is it smoothed?

    //-------------------------------------------------------------------------

    // Vector2 m_DPIScaling;
    float m_RenderWidth;
    float m_RenderHeight;

    //-------------------------------------------------------------------------

    std::vector<Vector2> m_BorderP1;
    std::vector<Vector2> m_BorderP2;

    //-------------------------------------------------------------------------

    // This is our delta_t
    float m_DeltaTime;
};
