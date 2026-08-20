#pragma once

#include <cmath>
#include <raylib.h>

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

//-------------------------------------------------------------------------

enum class UpdateScheme : uint8_t
{
    Explicit,
    Implicit, // Just pressure... As it is a pain in the ass...
    Leapfrog,
    RK2
    // Mabey RK4?
};

//-------------------------------------------------------------------------

struct PhysicsSettings
{
    //-------------------------------------------------------------------------

    const uint32_t TargetFPS{ 60 };
    const float    FrameTime{ 1.0f / TargetFPS };
    float          RestTime;
    float          InvTimestepMultiplier{ 2.0f };
    float          SlowMultiplier{ 1.0f };
    bool           Paused{ true };

    //-------------------------------------------------------------------------

    const float  SimulationResolution{ 10.0f }; // X and Y are same scale...
    uint32_t     ParticleCount{ 0 };
    UpdateScheme Scheme{ UpdateScheme::Leapfrog };
    int32_t      ImplicitMaxIterations{ 5 }; // ImGui doesn't take uint

    //-------------------------------------------------------------------------

    float TargetDensity{ ParticleCount / SimulationResolution / SimulationResolution / 1.3f };
    float SmoothingRadius{ 2 * std::sqrt( SimulationResolution * SimulationResolution / ( ParticleCount ) ) };

    bool ApplyPressureForce{ true };
    bool ApplyViscocity{ false };
    bool ApplyMouseForce{ false };

    // This is directly linked to the speed of sound in the fluid.
    // c^2 = PressureMultiplier, which is due to the way we calculate the pressure.
    // see: https://en.wikipedia.org/wiki/Speed_of_sound

    float PressureMultiplier{ 10000.0f / ParticleCount };
    float Viscocity{ 0.01f }; // Might need tweaking.

    bool  ApplyGravity{ false };
    float GravityMultiplier{ 0.0f };

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

    bool  DrawSpeedColor{ false };
    float MaxSpeed{ 1.0f };

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

    [[deprecated( "Use the Init{SimulationInstance} methods instead." )]] void Restart() noexcept;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    void Update( float timestep ) noexcept;

    void SetScheme( UpdateScheme scheme ) noexcept;
    void ExplicitUpdate( float timestep ) noexcept;
    void ImplicitUpdate( float timestep ) noexcept;
    void LeapfrogUpdate( float timestep ) noexcept;
    void RK2Update( float timestep ) noexcept;

    void UpdateDensities() noexcept; // Currently uses an explicit method, with the current pos, and density.
    void UpdatePressures() noexcept; // Just conversion from Densities to pressure by some method...
    void UpdatePressureGradiant() noexcept;

    void UpdateViscocity() noexcept; // Applies it directly to the velocity.

    void Render() noexcept;

    void DrawPhysicsOverlay() noexcept;
    void DrawDebugOverlay() noexcept;
    void DrawAirfoilOverlay() noexcept;
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

    Vector2 CalculateViscocityForce( Vector2 location ) noexcept;
    Vector2 CalculateViscocityForce( uint32_t index ) noexcept;

    void ApplyMouseAction( float timestep ) noexcept;

    //-------------------------------------------------------------------------

    // Boundary Conditions:
    void HandleBoundary() noexcept; // Reflection ish...

    //-------------------------------------------------------------------------

    // Debug stuff...
    void DrawDensity() noexcept;
    void DrawPressure() noexcept;

    //-------------------------------------------------------------------------

    // All defined in regards to the SmoothingRadius.
    int32_t m_MinCol; // The particle most to the left.
    int32_t m_MaxCol; // The particle most to the right.
    int32_t m_MinRow; // The particle most towards the top
    int32_t m_MaxRow; // The particle most to the bottom.

    uint32_t m_GridWidth;
    uint32_t m_GridHeight;
    uint32_t m_CellCount;

    void UpdateGridDims() noexcept;

    //-------------------------------------------------------------------------

    // To be able to scale the stuff, at runtime,
    // we need to compute the grid for each particle each frame.
    std::pair<int32_t, int32_t> GetCellCount() noexcept;
    void                        UpdateGridIndices() noexcept;
    int32_t                     GetGridIndex( Vector2 pos ) noexcept;
    std::vector<int32_t>        GetNeighbourCells( int32_t i ) noexcept;
    std::span<int32_t>          GetParticlesInCell( int32_t i ) noexcept;
    // Would be a nice API, to just do ForEachSurrounding(index, []() {update}) // Just for the future, if i have time.

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------

    PhysicsSettings m_PhysicsSettings{};
    DebugSettings   m_DebugSettings{};

    //-------------------------------------------------------------------------

    // Grid stuff...
    std::vector<int32_t> m_GridIndices;
    std::vector<int32_t> m_CellStart;       // Maps cell to the start of the particles in the cell in the sorted particles.
    std::vector<int32_t> m_SortedParticles; // Maps to the actual position.

    // Particle info:
    //-------------------------------------------------------------------------

    std::vector<Vector2> m_Positions;
    std::vector<Vector2> m_Velocities;
    std::vector<Color>   m_ParticleColors; // Initialized manually.
    int32_t              m_GridAxisSize{ 0 };
    float                m_Masses{ 1.0f };

    // The variant values, calculated based on the particles.
    // Note: The position for the i'th value is m_Positions[i].
    std::vector<float>   m_Densities;
    std::vector<float>   m_Pressures;
    std::vector<Vector2> m_PressureGradiants;
    std::vector<Vector2> m_ViscocityForces;

    // Boundary info:
    //-------------------------------------------------------------------------

    // Currently this is just no normal flow at the lines defined by P1[i] -> P2[i]
    std::vector<Vector2> m_BorderP1;
    std::vector<Vector2> m_BorderP2;

    //-------------------------------------------------------------------------

    void InitSandbox() noexcept;
    void InitAirfoil() noexcept;
    void InitObstacle() noexcept;

    //-------------------------------------------------------------------------

    void UpdateAirfoil() noexcept;
    void AirfoilParticleTeleport() noexcept;

    std::vector<Vector2> m_TransformedAirfoilP1;
    std::vector<Vector2> m_TransformedAirfoilP2;

    float   m_AFScale{ 1.0f };
    float   m_AFRotation{ 0.0f };
    float   m_AFSpawningSpeed{ 3.0f };
    Vector2 m_AFOffset{ m_PhysicsSettings.SimulationResolution / 2.0f, m_PhysicsSettings.SimulationResolution / 2.0f };

    bool m_AFEnabled{ false };

    //-------------------------------------------------------------------------

    void ObstacleParticleTeleport() noexcept;

    std::vector<Vector2> m_ObstaclesP1;
    std::vector<Vector2> m_ObstaclesP2;

    bool m_Obstacles{ false };

    //-------------------------------------------------------------------------
};
