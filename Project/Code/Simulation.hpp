#pragma once

#include <cmath>
#include <raylib.h>

#include <cstdint>
#include <vector>

//-------------------------------------------------------------------------

enum class UpdateScheme : uint8_t
{
    Explicit = 0,
    Implicit = 1,
    // Mabey RK4?
};

//-------------------------------------------------------------------------

struct PhysicsSettings
{
    //-------------------------------------------------------------------------

    uint32_t TargetFPS{ 60 };
    float    DeltaTime{ 1.0f / TargetFPS };
    bool     Paused{ true };

    //-------------------------------------------------------------------------

    float        SimulationResolution{ 10.0f }; // X and Y are same scale...
    uint32_t     ParticleCount{ 2000 };
    UpdateScheme Scheme{ UpdateScheme::Implicit };

    //-------------------------------------------------------------------------

    float TargetDensity{ ParticleCount / SimulationResolution / SimulationResolution / 1.3f };
    float SmoothingRadius{ 2 * std::sqrt( SimulationResolution * SimulationResolution / ( ParticleCount ) ) };
    float PressureMultiplier{ 10000.0f / ParticleCount };

    //-------------------------------------------------------------------------
};

enum class DebugField
{
    None = 0,
    Density,
    Pressure,
};

// Since the drawing of the particles, aren't actually neccesary, it is considered to be part of the debug...
struct DebugSettings
{

    //-------------------------------------------------------------------------

    uint32_t   RenderResolution;
    float      ParticleDrawRadius{ 5.0f };
    bool       Draw{ true };
    DebugField Field{ DebugField::None };

    //-------------------------------------------------------------------------

    // This is the size of a texture, we generate to project the different fields for debug.
    // A larger value doesn't matter a lot, at we just want a general sense whether or not out field looks right.
    uint32_t           DebugFieldResolution{ 100 };
    std::vector<Color> DebugPixels;
    Texture2D          DebugTexture;

    //-------------------------------------------------------------------------

    // Some settings for how to color it.
    float DebugFieldMin{ -20.0f };
    float DebugFieldMiddle{ 0.0f };
    float DebugFieldMax{ 20.0f };

    Color DebugMinColor{ BLUE };
    Color DebugMiddleColor{ WHITE };
    Color DebugMaxColor{ RED };

    //-------------------------------------------------------------------------
};

//-------------------------------------------------------------------------

class Simulation
{

public:

    //-------------------------------------------------------------------------

    Simulation( uint32_t size );
    ~Simulation();

    //-------------------------------------------------------------------------

    void Run() noexcept;
    void Restart() noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    void Update() noexcept;

    void SetScheme( UpdateScheme scheme ) noexcept;
    void ExplicitUpdate() noexcept;
    void ImplicitUpdate() noexcept;

    void UpdateDensities() noexcept; // Currently uses an explicit method, with the current pos, and density.
    void UpdatePressures() noexcept; // Just conversion from Densities to pressure by some method...
    void UpdatePressureGradiant() noexcept;

    void ApplyForces() noexcept;

    void Render() noexcept;

    void DrawPhysicsOverlay() noexcept;
    void DrawDebugOverlay() noexcept;

    //-------------------------------------------------------------------------

    // Utilities:
    Vector2 WorldSpaceToScreenSpace( Vector2 WS ) noexcept;
    float   WorldSpaceToSCreenSpace( float WS ) noexcept;

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

    PhysicsSettings m_PhysicsSettings{};
    DebugSettings   m_DebugSettings{};

    // Particle info:
    //-------------------------------------------------------------------------

    std::vector<Vector2> m_Positions;
    std::vector<Vector2> m_Velocities;
    std::vector<Color>   m_ParticleColors; // Initialized manually.
    float                m_Masses{ 1.0f };

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
