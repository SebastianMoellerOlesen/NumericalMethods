#pragma once

#include <raylib.h>

#include <cstdint>
#include <vector>

//-------------------------------------------------------------------------

class Simulation
{

public:

    //-------------------------------------------------------------------------

    Simulation( uint32_t size );
    ~Simulation();

    //-------------------------------------------------------------------------

    void Run() noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    void Update() noexcept;
    void UpdateDensities() noexcept; // Currently uses an explicit method, with the current pos, and density.
    void UpdatePressures() noexcept; // Just conversion from Densities to pressure by some method...
    void UpdatePressureGradiant() noexcept;

    void Render() noexcept;

    //-------------------------------------------------------------------------

    // Utilities:
    Vector2 WorldSpaceToScreenSpace( Vector2 WS ) noexcept;
    float   WorldSpaceToSCreenSpace( float WS ) noexcept;

    Vector2 GetRandomDir();
    Vector2 GetRandomFloat( float min, float max ) noexcept;

    float CalculateDensity( Vector2 location ) noexcept; // Includes all particles.
    float CalculateDensity( uint32_t index ) noexcept;   // Uses the position for the local index. This one includes all particles, include the particle itself.

    float CalculatePressure( Vector2 location ) noexcept;
    float CalculatePressure( uint32_t index ) noexcept;

    Vector2 CalculatePressureGradiant( Vector2 location ) noexcept; // Includes all particles, and if pos1 == pos2 generates a random direction.
    Vector2 CalculatePressureGradiant( uint32_t index ) noexcept;   // Skips the gradient from the particle itself, as it should be 0.

    //-------------------------------------------------------------------------

    // Boundary Conditions:
    void HandleBorderCollision() noexcept; // Reflection ish...

    //-------------------------------------------------------------------------

    // Debug stuff...
    void DrawDensity() noexcept;
    void DrawPressure() noexcept;

    //-------------------------------------------------------------------------

private:

    // Simulation settings:
    //-------------------------------------------------------------------------

    // Delta time stuff
    uint32_t m_TargetFPS{ 60 };
    float    m_DeltaTime{ 1.0f / m_TargetFPS };

    // Simulation size stuff
    float    m_SimulationResolution{ 10.0f }; // X and Y are same scale...
    uint32_t m_ParticleCount{ 1000 };

    // Particle param stuff
    float m_Masses{ 1.0f };
    float m_TargetDensity{ 10.0f };
    float m_SmoothingRadius{ 0.7f };
    float m_PressureMultiplier{ 1.0f };

    bool m_Paused{ true };

    // Window settings:
    //-------------------------------------------------------------------------

    // Both x and y are equal.
    uint32_t m_RenderResolution;

    // Render settings:
    //-------------------------------------------------------------------------

    float              m_ParticleDrawRadius{ 5.0f };
    std::vector<Color> m_ParticleColors; // Initialized manually.

    // Debug settings:
    //-------------------------------------------------------------------------

    // This is the size of a texture, we generate to project the different fields for debug.
    // A larger value doesn't matter a lot, at we just want a general sense whether or not out field looks right.
    uint32_t           m_DebugFieldResolution{ 200 };
    std::vector<Color> m_DebugPixels;
    Texture2D          m_DebugTexture;

    // Some settings for how to color it.
    float m_DebugFieldMin{ -10.0f };
    float m_DebugFieldMiddle{ 0.0f };
    float m_DebugFieldMax{ 10.0f };

    Color m_DebugMinColor{ BLUE };
    Color m_DebugMiddleColor{ WHITE };
    Color m_DebugMaxColor{ RED };

    // Particle info:
    //-------------------------------------------------------------------------

    std::vector<Vector2> m_Positions;
    std::vector<Vector2> m_Velocities;

    // The variant values, calculated based on the particles.
    // Note: The position for the i'th value is m_Positions[i].
    std::vector<float>   m_Densities;
    std::vector<float>   m_Pressures;
    std::vector<Vector2> m_PressureGradiants;

    // Boundary info:
    //-------------------------------------------------------------------------

    // Currently this is just no normal flow at the lines defined by P1[i] -> P2[i]
    std::vector<Vector2> m_BorderP1;
    std::vector<Vector2> m_BorderP2;

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
};
