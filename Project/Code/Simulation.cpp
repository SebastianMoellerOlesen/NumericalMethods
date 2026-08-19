#include "Simulation.hpp"
#include "SmoothingKernals.hpp"
#include "Utils.hpp"

#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>
#include <rlgl.h>
#include <imgui_impl_raylib.h>

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <span>
#include <unistd.h>
#include <ranges>
#include <utility>
#include <vector>

//-------------------------------------------------------------------------

Simulation::Simulation( uint32_t size )
{

    //-------------------------------------------------------------------------

    SetConfigFlags( FLAG_WINDOW_HIGHDPI );
    InitWindow( size, size, "Simulation" );
    SetTargetFPS( m_PhysicsSettings.TargetFPS ); // Debatable if it should be in the physics settings...

    BeginDrawing();
    EndDrawing();

    // Fixes mouse scaling for HighDPI displays?
    // At least for my config. Milage may vary
    SetMouseScale( 1.0f, 1.0f );

    //-------------------------------------------------------------------------

    rlImGuiSetup( true ); // Darkmode: true

    //-------------------------------------------------------------------------

    m_DebugSettings.RenderResolution = size;

    //-------------------------------------------------------------------------

    // Create the debug texture:
    m_DebugSettings.DebugPixels.resize( m_DebugSettings.DebugFieldResolution * m_DebugSettings.DebugFieldResolution );

    Image debugImg = {
        .data    = m_DebugSettings.DebugPixels.data(),
        .width   = static_cast<int>( m_DebugSettings.DebugFieldResolution ),
        .height  = static_cast<int>( m_DebugSettings.DebugFieldResolution ),
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

    m_DebugSettings.DebugTexture = LoadTextureFromImage( debugImg );
    SetTextureFilter( m_DebugSettings.DebugTexture, TEXTURE_FILTER_BILINEAR );
}

Simulation::~Simulation()
{
    UnloadTexture( m_DebugSettings.DebugTexture );
    rlImGuiShutdown();
    CloseWindow();
}

//-------------------------------------------------------------------------

Vector2 Simulation::WorldSpaceToScreenSpace( Vector2 WS ) noexcept
{
    return Vector2Scale( WS, m_DebugSettings.RenderResolution / m_PhysicsSettings.SimulationResolution );
}

//-------------------------------------------------------------------------

void Simulation::UpdateGridDims() noexcept
{
    float min_x = std::min_element( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                                    []( const Vector2& a, const Vector2& b ) { return a.x < b.x; } )
                      ->x;

    //-------------------------------------------------------------------------

    float min_y = std::min_element( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                                    []( const Vector2& a, const Vector2& b ) { return a.y < b.y; } )
                      ->y;

    //-------------------------------------------------------------------------

    float max_x = std::max_element( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                                    []( const Vector2& a, const Vector2& b ) { return a.x < b.x; } )
                      ->x;

    //-------------------------------------------------------------------------

    float max_y = std::max_element( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                                    []( const Vector2& a, const Vector2& b ) { return a.y < b.y; } )
                      ->y;

    //-------------------------------------------------------------------------

    float radius = m_PhysicsSettings.SmoothingRadius;

    m_MinCol = min_x / radius;
    m_MaxCol = max_x / radius;
    m_MinRow = min_y / radius;
    m_MaxRow = max_y / radius;

    m_GridWidth  = m_MaxCol - m_MinCol + 1;
    m_GridHeight = m_MaxRow - m_MinRow + 1;
    m_CellCount  = m_GridWidth * m_GridHeight;
}

int32_t Simulation::GetGridIndex( Vector2 pos ) noexcept
{
    pos /= m_PhysicsSettings.SmoothingRadius;

    // Old version.
    // int32_t x  = pos.x; // Should floor, if bugs, mabey this is rounding...
    // int32_t y  = pos.y; // -| |-

    int32_t x = pos.x - m_MinCol;
    int32_t y = pos.y - m_MinRow;

    // return x + y * m_GridAxisSize;
    return x + y * m_GridWidth;
}

//-------------------------------------------------------------------------

void Simulation::UpdateGridIndices() noexcept
{

    //-------------------------------------------------------------------------

    // Old version.
    // m_GridAxisSize = m_PhysicsSettings.SimulationResolution / m_PhysicsSettings.SmoothingRadius;
    // ++m_GridAxisSize; // We increment by one, to account for any potential flooring. Having this be one larger than necessary won't matter.

    UpdateGridDims();

    //-------------------------------------------------------------------------

    // Get the index for each particle...
    std::for_each( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(), [this]( const Vector2& pos ) {
        size_t i         = &pos - m_Positions.data();
        m_GridIndices[i] = GetGridIndex( pos );
    } );

    //-------------------------------------------------------------------------

    // Old version
    // uint32_t cellCount = m_GridAxisSize * m_GridAxisSize;

    uint32_t cellCount = m_CellCount;

    //-------------------------------------------------------------------------

    // This should be fine for the new version.
    // We do however have a massive problem, if some particle goes -> inf...
    // As we will need to allocate a MASSIVE amount of storage...
    // Mabey do some check somewhere, (if len(pos) > threshold) { delete particle, or reset it... }

    // Reset the indexing.
    // We use cellCount + 1, so we can easily acces the indices that belong to a cell, by just taking all the indices between [cellindex] -> [cellindex + 1]
    m_CellStart.assign( cellCount + 1, 0 );

    // Generate the starting index for each cell.
    for ( int32_t cell : m_GridIndices ) { m_CellStart[cell + 1]++; }                                                           // How many particles are in each cell.
    for ( int32_t cellIndex = 0; cellIndex < cellCount; cellIndex++ ) { m_CellStart[cellIndex + 1] += m_CellStart[cellIndex]; } // numpy.cumsum equivalent.

    //-------------------------------------------------------------------------

    // Now we sort the particles.
    m_SortedParticles.resize( m_GridIndices.size() );

    // Track where we should place the next particle, that belongs to a specific cell.
    std::vector<int32_t> NextIndex = m_CellStart;

    // For each particle
    for ( size_t i = 0; i < m_GridIndices.size(); i++ )
    {
        // Get the cell for the particle.
        int32_t cell = m_GridIndices[i];

        // Place it into the Next index, for the cell it belongs to.
        int32_t& sortedIndex           = NextIndex[cell];
        m_SortedParticles[sortedIndex] = i;
        sortedIndex++; // Increment the index, so its ready for the next particle.
    }
}

//-------------------------------------------------------------------------

std::vector<int32_t> Simulation::GetNeighbourCells( int32_t i ) noexcept
{

    //-------------------------------------------------------------------------

    // Old
    // int32_t col = i % m_GridAxisSize;
    // int32_t row = i / m_GridAxisSize;

    int32_t col = i % m_GridWidth;
    int32_t row = i / m_GridWidth;

    //-------------------------------------------------------------------------

    std::vector<int32_t> neighbours;
    neighbours.reserve( 9 ); // At best, we can have 9 cells, including the center.

    //-------------------------------------------------------------------------

    for ( int32_t dy = -1; dy <= 1; dy++ )
    {
        for ( int32_t dx = -1; dx <= 1; dx++ )
        {
            // Get the location of the cell.
            int32_t c = col + dx;
            int32_t r = row + dy;

            // Check if the location is within bound.
            // Note this needs to be reworked, if implementing something like ghost cells.
            if ( c >= 0 && c < m_GridWidth && r >= 0 && r < m_GridHeight )
            {
                neighbours.push_back( c + r * m_GridWidth );
            }
        }
    }

    //-------------------------------------------------------------------------

    return neighbours;

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

std::span<int32_t> Simulation::GetParticlesInCell( int32_t i ) noexcept
{
    // Bounds checking
    if ( i < 0 || i >= m_CellStart.size() - 1 )
    {
        return {};
    }
    return { m_SortedParticles.data() + m_CellStart[i], static_cast<size_t>( m_CellStart[i + 1] - m_CellStart[i] ) };
}

//-------------------------------------------------------------------------

void Simulation::Run() noexcept
{

    while ( !WindowShouldClose() )
    {

        //-------------------------------------------------------------------------

        Render();

        //-------------------------------------------------------------------------

        // Avoid Updating inf times, when trying to change the radius...
        if ( m_PhysicsSettings.SmoothingRadius == 0 )
        {
            continue;
        }

        //-------------------------------------------------------------------------

        float slowdown = m_PhysicsSettings.SlowMultiplier < 1e-3f ? 1.0f : m_PhysicsSettings.SlowMultiplier;

        m_PhysicsSettings.RestTime += m_PhysicsSettings.FrameTime / slowdown;
        float timestep              = m_PhysicsSettings.SmoothingRadius / std::sqrt( m_PhysicsSettings.PressureMultiplier ) / m_PhysicsSettings.InvTimestepMultiplier;
        timestep                    = timestep > m_PhysicsSettings.FrameTime ? m_PhysicsSettings.FrameTime : timestep;

        //-------------------------------------------------------------------------

        while ( m_PhysicsSettings.RestTime >= timestep )
        {
            Update( timestep );
            m_PhysicsSettings.RestTime -= timestep;
        }

        //-------------------------------------------------------------------------
    }
}

void Simulation::InitSandbox( /* Change this to take a bool, on whether or not to reset settings */ ) noexcept
{
    //-------------------------------------------------------------------------

    m_PhysicsSettings.ApplyPressureForce = false;
    m_PhysicsSettings.ApplyViscocity     = false;
    m_PhysicsSettings.ApplyGravity       = false;
    m_PhysicsSettings.ApplyMouseForce    = false;

    m_PhysicsSettings.SlowMultiplier = 1.0f;

    //-------------------------------------------------------------------------

    m_Masses                                = 1.0f;
    m_PhysicsSettings.Paused                = true;
    m_PhysicsSettings.ParticleCount         = 2000;
    m_PhysicsSettings.SmoothingRadius       = 0.35;
    m_PhysicsSettings.InvTimestepMultiplier = 2.0f;
    m_PhysicsSettings.GravityMultiplier     = 5.0f;
    m_PhysicsSettings.TargetDensity         = 15.0f;
    m_PhysicsSettings.PressureMultiplier    = 100.0f;
    m_PhysicsSettings.Viscocity             = 0.01f;
    m_PhysicsSettings.Scheme                = UpdateScheme::Implicit;

    //-------------------------------------------------------------------------

    m_DebugSettings.Draw               = true;
    m_DebugSettings.ParticleDrawRadius = 5.0f;

    m_DebugSettings.Field            = DebugField::Density;
    m_DebugSettings.DebugFieldMiddle = 10.0f;
    m_DebugSettings.DebugFieldMax    = 30.0f;

    //-------------------------------------------------------------------------

    m_AFEnabled = false;

    //-------------------------------------------------------------------------

    uint32_t particleCount = m_PhysicsSettings.ParticleCount;
    m_Positions.resize( particleCount );
    m_Velocities.resize( particleCount );
    m_ParticleColors.assign( particleCount, BLUE );

    //-------------------------------------------------------------------------

    // For the sandbox, we initialilze the particles randomly, without velocity.
    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
    {
        // Position
        float x        = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        float y        = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        m_Positions[i] = Vector2( x, y ) / m_DebugSettings.RenderResolution * m_PhysicsSettings.SimulationResolution;

        //-------------------------------------------------------------------------

        float dx = 0.0f;
        float dy = 0.0f;

        m_Velocities[i] = Vector2( dx, dy );

        //-------------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------

    UpdateGridIndices();

    //-------------------------------------------------------------------------

    // Initialize the fields.
    m_Densities.resize( m_PhysicsSettings.ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressureGradiant();

    // Avoid the density only being drawn, when we unpause...
    DrawDensity(); // <- Doesn't work for some reason... Or the field is very weak?? WFT... It works after unpausing...

    //-------------------------------------------------------------------------

    // Update the borders, so they are as they should be:
    m_BorderP1.resize( 0 );
    m_BorderP2.resize( 0 );
    m_BorderP1.insert( m_BorderP1.begin(), { { 0, 0 }, { m_PhysicsSettings.SimulationResolution, 0 }, { m_PhysicsSettings.SimulationResolution, m_PhysicsSettings.SimulationResolution }, { 0, m_PhysicsSettings.SimulationResolution } } );
    m_BorderP2.insert( m_BorderP2.begin(), { { m_PhysicsSettings.SimulationResolution, 0 }, { m_PhysicsSettings.SimulationResolution, m_PhysicsSettings.SimulationResolution }, { 0, m_PhysicsSettings.SimulationResolution }, { 0, 0 } } );

    //-------------------------------------------------------------------------
}

void Simulation::InitAirfoil( /* Change this to take a bool, on whether or not to reset settings */ ) noexcept
{
    //-------------------------------------------------------------------------

    m_PhysicsSettings.ApplyPressureForce = true;
    m_PhysicsSettings.ApplyViscocity     = true;
    m_PhysicsSettings.ApplyGravity       = false;
    m_PhysicsSettings.ApplyMouseForce    = false;

    m_PhysicsSettings.SlowMultiplier = 5.0f;
    currentKernal                    = Kernal::Spiky3;

    //-------------------------------------------------------------------------

    m_Masses                                = 1.0f;
    m_PhysicsSettings.Paused                = true;
    m_PhysicsSettings.ParticleCount         = 5000;
    m_PhysicsSettings.SmoothingRadius       = 0.45f;
    m_PhysicsSettings.InvTimestepMultiplier = 4.0f; // Large here to avoid them phasing through the airfoil...
    m_PhysicsSettings.GravityMultiplier     = 0.0f;
    m_PhysicsSettings.TargetDensity         = 10.0f;
    m_PhysicsSettings.PressureMultiplier    = 1000.0f;
    m_PhysicsSettings.Viscocity             = 0.01f;
    m_PhysicsSettings.Scheme                = UpdateScheme::Implicit;

    //-------------------------------------------------------------------------

    m_DebugSettings.Draw               = true;
    m_DebugSettings.ParticleDrawRadius = 5.0f;

    m_DebugSettings.Field            = DebugField::None;
    m_DebugSettings.DebugFieldMiddle = 30.0f;
    m_DebugSettings.DebugFieldMax    = 70.0f;

    //-------------------------------------------------------------------------

    m_AFEnabled  = true;
    m_AFScale    = 2.0f;
    m_AFRotation = 4.7f;
    m_AFOffset   = { 0.0f, 4.4f };
    UpdateAirfoil();

    //-------------------------------------------------------------------------

    uint32_t particleCount = m_PhysicsSettings.ParticleCount;
    m_Positions.resize( particleCount );
    m_Velocities.resize( particleCount );
    m_ParticleColors.assign( particleCount, BLUE );

    //-------------------------------------------------------------------------

    // For the sandbox, we initialilze the particles randomly, without velocity.
    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
    {
        // Position
        float x        = RandomFloat( -1.0f, m_PhysicsSettings.SimulationResolution + 5.0f );
        float y        = RandomFloat( 0.0f, m_PhysicsSettings.SimulationResolution );
        m_Positions[i] = Vector2( x, y );

        //-------------------------------------------------------------------------

        float dx = m_AFSpawningSpeed;
        float dy = 0.0f;

        m_Velocities[i] = Vector2( dx, dy );

        //-------------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------

    m_GridIndices.resize( m_PhysicsSettings.ParticleCount );
    UpdateGridIndices();

    //-------------------------------------------------------------------------

    // Initialize the fields.
    m_Densities.resize( m_PhysicsSettings.ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressureGradiant();

    // Avoid the density only being drawn, when we unpause...
    DrawDensity();

    //-------------------------------------------------------------------------

    // Update the borders, so they are as they should be:
    m_BorderP1.resize( 0 );
    m_BorderP2.resize( 0 );
    m_BorderP1.insert( m_BorderP1.begin(), { { 0, 0 }, { m_PhysicsSettings.SimulationResolution, m_PhysicsSettings.SimulationResolution }, { -3, m_PhysicsSettings.SimulationResolution } } );
    m_BorderP2.insert( m_BorderP2.begin(), { { m_PhysicsSettings.SimulationResolution, 0 }, { 0, m_PhysicsSettings.SimulationResolution }, { -3, 0 } } );

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

void Simulation::Update( float timestep ) noexcept
{

    if ( m_PhysicsSettings.Paused )
    {
        return;
    }

    //-------------------------------------------------------------------------

    switch ( m_PhysicsSettings.Scheme )
    {
        case UpdateScheme::Explicit:
            ExplicitUpdate( timestep );
            break;

        case UpdateScheme::Implicit:
            ImplicitUpdate( timestep );
            break;
    }
}

void Simulation::ImplicitUpdate( float timestep ) noexcept
{

    //-------------------------------------------------------------------------

    // Save the old position so we can restore it after calculating the forces.
    // It might also be viable to create another vector in the class, to hold forwardPos...
    std::vector<Vector2> pos = m_Positions;

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        // Precict positions.
        m_Positions[i] += m_Velocities[i] * timestep;
    }

    HandleBoundary();

    //-------------------------------------------------------------------------

    // Update velocity implicitly.
    // Todo: Change the UpdateMethods to apply if applicable...
    // Mostly the PressureGradiant...

    UpdateGridIndices();
    UpdateDensities();
    UpdatePressures();

    UpdatePressureGradiant(); // Update to ApplyPressureGradiant()... Should be straight forward...

    //-------------------------------------------------------------------------

    if ( m_PhysicsSettings.ApplyPressureForce )
    {
        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * timestep;
        }
    }

    if ( m_PhysicsSettings.ApplyGravity )
    {
        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            m_Velocities[i] += Vector2( 0.0f, 1.0f ) * m_PhysicsSettings.GravityMultiplier * timestep;
        }
    }

    if ( ( IsMouseButtonDown( MOUSE_LEFT_BUTTON ) || IsMouseButtonDown( MOUSE_RIGHT_BUTTON ) ) && m_PhysicsSettings.ApplyMouseForce )
    {
        float   multiplier = IsMouseButtonDown( MOUSE_LEFT_BUTTON ) ? 1.0f : -1.0f;
        Vector2 worldPos   = GetMousePosition() / m_DebugSettings.RenderResolution * m_PhysicsSettings.SimulationResolution;

        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            Vector2 difference = worldPos - m_Positions[i];
            float   distance   = Vector2Length( difference );
            float   weight     = CurrentSmoothingKernal2D( 2.0f, distance );

            m_Velocities[i] += difference / distance * weight * multiplier * m_PhysicsSettings.PressureMultiplier / m_Densities[i];
        }
    }

    if ( m_PhysicsSettings.ApplyViscocity )
    {
        // We use operator splitting, to compute the Viscocity.
        // v^*(n+1) = v^(n) + dt * F(p^(n+1))
        // v^(n+1) = dt * F(v^*(n+1))
        //
        // We can mabey apply the Strang Splitting here.
        // Since the ApplyViscocity update the velocities directly, would it cause an issue?
        // Just think about how the Strang splitting actually works...
        // But for now, i'll keep it simple.
        ApplyViscocity( timestep );
    }

    // Restore the old pos, so we can update it using the new vel.
    m_Positions = std::move( pos );

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        m_Positions[i] += m_Velocities[i] * timestep;
    }

    //-------------------------------------------------------------------------

    // Make sure the balls stay inside the box.
    // Acts like a reflective BC i guess...
    HandleBoundary();
}

void Simulation::ExplicitUpdate( float timestep ) noexcept
{

    //-------------------------------------------------------------------------

    UpdateGridIndices();
    UpdateDensities();
    UpdatePressures();
    UpdatePressureGradiant();

    //-------------------------------------------------------------------------

    if ( m_PhysicsSettings.ApplyViscocity )
    {
        // The update method here is completely explicit.
        // Currently we have pos^(n), and velocity^(n).
        // The force from the other methods are also already calculated.
        ApplyViscocity( timestep );
    }

    if ( m_PhysicsSettings.ApplyPressureForce )
    {
        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * timestep;
        }
    }

    if ( m_PhysicsSettings.ApplyGravity )
    {
        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            m_Velocities[i] += Vector2( 0.0f, 1.0f ) * m_PhysicsSettings.GravityMultiplier * timestep;
        }
    }

    if ( ( IsMouseButtonDown( MOUSE_LEFT_BUTTON ) || IsMouseButtonDown( MOUSE_RIGHT_BUTTON ) ) && m_PhysicsSettings.ApplyMouseForce )
    {
        float   multiplier = IsMouseButtonDown( MOUSE_LEFT_BUTTON ) ? 1.0f : -1.0f;
        Vector2 worldPos   = GetMousePosition() / m_DebugSettings.RenderResolution * m_PhysicsSettings.SimulationResolution;

        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            Vector2 difference = worldPos - m_Positions[i];
            float   distance   = Vector2Length( difference );
            float   weight     = CurrentSmoothingKernal2D( 2.0f, distance );

            m_Velocities[i] += difference / distance * weight * multiplier * m_PhysicsSettings.PressureMultiplier / m_Densities[i];
        }
    }

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        m_Positions[i] += m_Velocities[i] * timestep;
    }

    //-------------------------------------------------------------------------

    // Make sure the balls stay inside the box.
    // Acts like a reflective BC i guess...
    HandleBoundary();
}

//-------------------------------------------------------------------------

void Simulation::HandleBoundary() noexcept
{
    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {

        Vector2& pos = m_Positions[i];
        Vector2& vel = m_Velocities[i];

        for ( uint32_t k = 0; k < m_BorderP1.size(); k++ )
        {
            Vector2 P1 = m_BorderP1[k];
            Vector2 P2 = m_BorderP2[k];

            Vector2 normal = Vector2Normalize( Vector2Rotate( P2 - P1, PI / 2 ) );
            float   offset = Vector2DotProduct( normal, P1 - m_Positions[i] );

            // If we are outside our box:
            // Or inside, if the points are counter clockwise...
            if ( offset > 0 )
            {
                pos += normal * offset * 2;                           // Reflect the travel
                vel -= normal * Vector2DotProduct( vel, normal ) * 2; // Reflect the particle.
            }
        }

        if ( m_AFEnabled )
        {
            // If we use the same method we did for the walls it won't work.
            // That only works, if we want to be on the inside...
            // So we need to find the edge we are closest to, and only use that as a comparison.

            float    shortestDistance = INFINITY;
            uint32_t shortestIdx      = 0;

            for ( uint32_t k = 0; k < m_TransformedAirfoilP1.size(); k++ )
            {
                Vector2 P1 = m_TransformedAirfoilP1[k];
                Vector2 P2 = m_TransformedAirfoilP2[k];

                // We need to find the point on the edge, that is closest to the particle.
                // We can't do as we did before, as we treat those as infinite lines.
                Vector2 edge         = P2 - P1;
                Vector2 closestPoint = P1 + edge * Clamp( Vector2DotProduct( pos - P1, edge ) / Vector2DotProduct( edge, edge ), 0.0f, 1.0f );

                float distance = Vector2Distance( pos, closestPoint );

                if ( distance < shortestDistance )
                {
                    shortestDistance = distance;
                    shortestIdx      = k;
                }
            }

            // Now we can do the response...
            // This is the same as for the line now...
            Vector2 P1 = m_TransformedAirfoilP1[shortestIdx];
            Vector2 P2 = m_TransformedAirfoilP2[shortestIdx];

            Vector2 normal = Vector2Normalize( Vector2Rotate( P2 - P1, PI / 2 ) );
            float   offset = Vector2DotProduct( normal, P1 - m_Positions[i] );

            // If we are outside our box:
            // Or inside, if the points are counter clockwise...
            if ( offset > 0 )
            {
                pos += normal * offset;
                vel -= normal * Vector2DotProduct( vel, normal );
            }
        }
    }

    if ( m_AFEnabled )
    {
        AirfoilParticleTeleport();
    }
}

//-------------------------------------------------------------------------

float Simulation::CalculateDensity( Vector2 location ) noexcept
{

    int32_t index            = GetGridIndex( location );
    auto    cells            = GetNeighbourCells( index );
    auto    neighbourIndices = cells | std::views::transform( [this]( int32_t cell ) { return GetParticlesInCell( cell ); } ) | std::views::join;

    float density = 0;
    for ( int32_t i : neighbourIndices )
    {
        float distance   = Vector2Length( location - m_Positions[i] );
        float influence  = CurrentSmoothingKernal2D( m_PhysicsSettings.SmoothingRadius, distance );
        density         += m_Masses * influence;
    }

    return density;
}

//-------------------------------------------------------------------------

void Simulation::UpdateDensities() noexcept
{

    //-------------------------------------------------------------------------

    // Explicit update.
    // Density^(n+1) = F(Density^(n), Position^(n))
    // Mabey, we should use some other method for better stability.

    //-------------------------------------------------------------------------

    std::vector<float> newDensities;
    newDensities.resize( m_PhysicsSettings.ParticleCount );

    //-------------------------------------------------------------------------

    std::for_each( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                   [this, &newDensities]( const Vector2& pos ) {
                       size_t i        = &pos - m_Positions.data();
                       newDensities[i] = this->CalculateDensity( pos );
                   } );

    //-------------------------------------------------------------------------

    // Mabey this is not the best approach.
    // We might wan't to store both the old and the new for some reason.
    // It should be easy to implement though...
    m_Densities = std::move( newDensities );

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

float Simulation::CalculatePressure( Vector2 location ) noexcept
{
    float difference = CalculateDensity( location ) - m_PhysicsSettings.TargetDensity;
    return difference * m_PhysicsSettings.PressureMultiplier;
}

float Simulation::CalculatePressure( uint32_t index ) noexcept
{
    float difference = m_Densities[index] - m_PhysicsSettings.TargetDensity;
    return difference * m_PhysicsSettings.PressureMultiplier;
}

//-------------------------------------------------------------------------

void Simulation::UpdatePressures() noexcept
{
    // No need to update them all at once, since we don't use the pressures, but the densities to calculate the pressure.
    std::for_each( std::execution::par_unseq, m_Pressures.begin(), m_Pressures.end(),
                   [this]( float& pressure ) {
                       size_t i = &pressure - m_Pressures.data();
                       pressure = CalculatePressure( i );
                   } );
}

//-------------------------------------------------------------------------

void Simulation::UpdatePressureGradiant() noexcept
{
    std::for_each( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(), [this]( const Vector2& pos ) {
        size_t i                     = &pos - m_Positions.data();
        this->m_PressureGradiants[i] = this->CalculatePressureGradiant( i );
    } );
}

//-------------------------------------------------------------------------

Vector2 Simulation ::CalculatePressureGradiant( Vector2 location ) noexcept
{

    //-------------------------------------------------------------------------

    int32_t index            = GetGridIndex( location );
    auto    cells            = GetNeighbourCells( index );
    auto    neighbourIndices = cells | std::views::transform( [this]( int32_t cell ) { return GetParticlesInCell( cell ); } ) | std::views::join;

    //-------------------------------------------------------------------------

    Vector2 gradient = Vector2Zero();

    //-------------------------------------------------------------------------

    for ( int32_t i : neighbourIndices )
    {

        Vector2 difference = location - m_Positions[i];
        float   distance   = Vector2Length( difference );

        // As it is currently, we apply a gradient from the particle itself.
        // It should take the index instead of the location.
        // It should still react to the pressure gradient from other particles, with potentially the same position...
        if ( distance == 0 )
        {
            // The GetRandomDir() returns a normalized vector, so no need to normalize.
            difference = GetRandomDir();
        }

        float influence  = CurrentSmoothingKernalDerivative2D( m_PhysicsSettings.SmoothingRadius, distance );
        gradient        += Vector2Normalize( difference ) * m_Masses * influence * m_Pressures[i] / m_Densities[i];
    }

    return gradient;
}

Vector2 Simulation::CalculatePressureGradiant( uint32_t index ) noexcept
{
    //-------------------------------------------------------------------------

    int32_t cell             = GetGridIndex( m_Positions[index] );
    auto    cells            = GetNeighbourCells( cell );
    auto    neighbourIndices = cells | std::views::transform( [this]( int32_t cell ) { return GetParticlesInCell( cell ); } ) | std::views::join;

    //-------------------------------------------------------------------------

    Vector2 gradient = Vector2Zero();

    //-------------------------------------------------------------------------

    for ( int32_t i : neighbourIndices )
    {

        //-------------------------------------------------------------------------

        if ( i == index )
        {
            continue;
        }

        //-------------------------------------------------------------------------

        Vector2 difference = m_Positions[index] - m_Positions[i];
        float   distance   = Vector2Length( difference );

        //-------------------------------------------------------------------------

        if ( distance == 0 )
        {
            // The GetRandomDir() returns a normalized vector, so no need to normalize.
            difference = GetRandomDir();
        }

        //-------------------------------------------------------------------------

        float influence        = CurrentSmoothingKernalDerivative2D( m_PhysicsSettings.SmoothingRadius, distance );
        float averagePressure  = ( m_Pressures[i] + m_Pressures[index] ) * 0.5f;
        gradient              -= Vector2Normalize( difference ) * m_Masses * influence * averagePressure / m_Densities[i];

        //-------------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------

    return gradient;

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

Vector2 Simulation::CalculateViscocityForce( uint32_t index ) noexcept
{

    int32_t cell             = GetGridIndex( m_Positions[index] );
    auto    cells            = GetNeighbourCells( cell );
    auto    neighbourIndices = cells | std::views::transform( [this]( int32_t cell ) { return GetParticlesInCell( cell ); } ) | std::views::join;

    //-------------------------------------------------------------------------

    Vector2 force = Vector2Zero();

    //-------------------------------------------------------------------------

    for ( int32_t i : neighbourIndices )
    {
        if ( i == index )
        {
            continue;
        }

        //-------------------------------------------------------------------------

        Vector2 difference = m_Positions[index] - m_Positions[i];
        float   distance   = Vector2Length( difference );
        float   influence  = CurrentSmoothingKernalLaplacian2D( m_PhysicsSettings.SmoothingRadius, distance );

        //-------------------------------------------------------------------------

        // Other minus this
        Vector2 deltaV  = m_Velocities[i] - m_Velocities[index];
        force          += deltaV * m_PhysicsSettings.Viscocity * m_Masses * influence;

        //-------------------------------------------------------------------------
    }

    return force;
}

void Simulation::ApplyViscocity( float timestep ) noexcept
{
    std::for_each( std::execution::par_unseq, m_SortedParticles.begin(), m_SortedParticles.end(), [this, &timestep]( const int32_t& idx ) {
        Vector2 force      = CalculateViscocityForce( idx );
        m_Velocities[idx] += force * timestep / m_Densities[idx];
    } );
}

//-------------------------------------------------------------------------

void Simulation::UpdateAirfoil() noexcept
{
    m_TransformedAirfoilP1.resize( AirfoilP1.size() );
    m_TransformedAirfoilP2.resize( AirfoilP2.size() );

    for ( uint32_t i = 0; i < AirfoilP1.size(); i++ )
    {
        Vector2 transformedP1 = Vector2Add( m_AFOffset, Vector2Rotate( Vector2Scale( AirfoilP1[i], m_AFScale ), m_AFRotation ) );
        Vector2 transformedP2 = Vector2Add( m_AFOffset, Vector2Rotate( Vector2Scale( AirfoilP2[i], m_AFScale ), m_AFRotation ) );

        m_TransformedAirfoilP1[i] = transformedP1;
        m_TransformedAirfoilP2[i] = transformedP2;
    }
}

void Simulation::AirfoilParticleTeleport() noexcept
{

    int32_t colorHere  = GetGridIndex( { m_AFOffset.x - 0.5f, m_PhysicsSettings.SimulationResolution / 2.0f } );
    int32_t colorHere2 = GetGridIndex( { m_AFOffset.x - 0.5f, m_PhysicsSettings.SimulationResolution / 2.0f - 1.0f } );

    std::for_each( std::execution::par_unseq, m_SortedParticles.begin(), m_SortedParticles.end(), [this, &colorHere, &colorHere2]( int32_t index ) {
        Vector2& pos = m_Positions[index];
        if ( pos.x > m_PhysicsSettings.SimulationResolution + 5 )
        {
            pos.x                 = -1.0f;
            m_Velocities[index].x = m_AFSpawningSpeed;
            // m_ParticleColors[index] = BLUE;
        }

        int32_t particleIdx = GetGridIndex( pos );

        if ( colorHere == particleIdx )
        {
            m_ParticleColors[index] = PINK;
        }
        if ( colorHere2 == particleIdx )
        {
            m_ParticleColors[index] = LIME;
        }

        if ( pos.x > m_AFOffset.x - 0.75f && pos.x < m_AFOffset.x - 0.5f )
        {
            m_ParticleColors[index] = BLUE;
        }
    } );
}

//-------------------------------------------------------------------------

void Simulation::Render() noexcept
{

    //-------------------------------------------------------------------------

    if ( m_AFEnabled )
    {
        UpdateAirfoil();
    }

    BeginDrawing();
    rlImGuiBegin();

    //-------------------------------------------------------------------------

    ClearBackground( DARKGRAY );

    switch ( m_DebugSettings.Field )
    {
        case DebugField::None:
            break;

        case DebugField::Density:
            DrawDensity();
            break;

        case DebugField::Pressure:
            DrawPressure();
            break;
    }

    //-------------------------------------------------------------------------

    if ( m_AFEnabled )
    {
        for ( uint32_t i = 0; i < m_TransformedAirfoilP1.size(); i++ ) { DrawLineEx( WorldSpaceToScreenSpace( m_TransformedAirfoilP1[i] ), WorldSpaceToScreenSpace( m_TransformedAirfoilP2[i] ), 3.0f, BLACK ); }
    }

    if ( m_DebugSettings.Draw )
    {
        for ( uint32_t i = 0; i < m_BorderP1.size(); i++ )
        {
            DrawLineEx( WorldSpaceToScreenSpace( m_BorderP1[i] ), WorldSpaceToScreenSpace( m_BorderP2[i] ), 6.0f, BLACK );
        }

        //-------------------------------------------------------------------------

        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            Color circleColor = m_ParticleColors[i];
            if ( m_DebugSettings.DrawSpeedColor ) { circleColor = ColorLerp( BLUE, RED, Vector2Length( m_Velocities[i] ) / m_DebugSettings.MaxSpeed ); }
            DrawCircleV( WorldSpaceToScreenSpace( m_Positions[i] ), m_DebugSettings.ParticleDrawRadius + 1.0f, BLACK );
            DrawCircleV( WorldSpaceToScreenSpace( m_Positions[i] ), m_DebugSettings.ParticleDrawRadius, circleColor );
        }
    }

    //-------------------------------------------------------------------------

    DrawPhysicsOverlay();
    DrawDebugOverlay();

    if ( m_AFEnabled )
    {
        // Enabled for specific presets.
        DrawAirfoilOverlay();
    }

    //-------------------------------------------------------------------------

    rlImGuiEnd();
    EndDrawing();
}

//-------------------------------------------------------------------------

void Simulation::DrawDensity() noexcept
{

    // How does each pixel location map to the world?
    float cellToWorld = m_PhysicsSettings.SimulationResolution / m_DebugSettings.DebugFieldResolution;

    std::for_each( std::execution::par_unseq, m_DebugSettings.DebugPixels.begin(), m_DebugSettings.DebugPixels.end(),
                   [this, cellToWorld]( Color& pixel ) {
                       //-------------------------------------------------------------------------

                       // Get the row and column for the current idx.
                       size_t i   = &pixel - this->m_DebugSettings.DebugPixels.data();
                       size_t col = i % this->m_DebugSettings.DebugFieldResolution;
                       size_t row = i / this->m_DebugSettings.DebugFieldResolution;

                       //-------------------------------------------------------------------------

                       Vector2 world   = { ( col + 0.5f ) * cellToWorld, ( row + 0.5f ) * cellToWorld };
                       float   density = this->CalculateDensity( world );

                       //-------------------------------------------------------------------------

                       // Map the density to the color.
                       float t = ( density - this->m_DebugSettings.DebugFieldMiddle ) / ( this->m_DebugSettings.DebugFieldMax - this->m_DebugSettings.DebugFieldMiddle );
                       pixel   = ColorLerp( this->m_DebugSettings.DebugMiddleColor, this->m_DebugSettings.DebugMaxColor, t );

                       //-------------------------------------------------------------------------
                   } );

    // Update the texture data, and draw it.
    UpdateTexture( m_DebugSettings.DebugTexture, m_DebugSettings.DebugPixels.data() );

    DrawTexturePro( m_DebugSettings.DebugTexture, { 0, 0, static_cast<float>( m_DebugSettings.DebugFieldResolution ), static_cast<float>( m_DebugSettings.DebugFieldResolution ) },
                    { 0, 0, static_cast<float>( m_DebugSettings.RenderResolution ), static_cast<float>( m_DebugSettings.RenderResolution ) }, { 0, 0 }, 0, WHITE );
}

void Simulation::DrawPressure() noexcept
{

    // How does each pixel location map to the world?
    float cellToWorld = m_PhysicsSettings.SimulationResolution / m_DebugSettings.DebugFieldResolution;

    std::for_each( std::execution::par_unseq, m_DebugSettings.DebugPixels.begin(), m_DebugSettings.DebugPixels.end(),
                   [this, cellToWorld]( Color& pixel ) {
                       //-------------------------------------------------------------------------

                       // Get the row and column for the current idx.
                       size_t i   = &pixel - this->m_DebugSettings.DebugPixels.data();
                       size_t col = i % this->m_DebugSettings.DebugFieldResolution;
                       size_t row = i / this->m_DebugSettings.DebugFieldResolution;

                       //-------------------------------------------------------------------------

                       Vector2 world    = { ( col + 0.5f ) * cellToWorld, ( row + 0.5f ) * cellToWorld };
                       float   pressure = this->CalculatePressure( world );

                       //-------------------------------------------------------------------------

                       // Map the density to the color.

                       if ( pressure < m_DebugSettings.DebugFieldMiddle )
                       {
                           float t = ( pressure - this->m_DebugSettings.DebugFieldMin ) / ( this->m_DebugSettings.DebugFieldMiddle - this->m_DebugSettings.DebugFieldMin );
                           pixel   = ColorLerp( this->m_DebugSettings.DebugMinColor, this->m_DebugSettings.DebugMiddleColor, t );
                       }
                       else
                       {
                           float t = ( pressure - this->m_DebugSettings.DebugFieldMiddle ) / ( this->m_DebugSettings.DebugFieldMax - this->m_DebugSettings.DebugFieldMiddle );
                           pixel   = ColorLerp( this->m_DebugSettings.DebugMiddleColor, this->m_DebugSettings.DebugMaxColor, t );
                       }

                       //-------------------------------------------------------------------------
                   } );

    // Update the texture data, and draw it.
    UpdateTexture( m_DebugSettings.DebugTexture, m_DebugSettings.DebugPixels.data() );

    DrawTexturePro( m_DebugSettings.DebugTexture, { 0, 0, static_cast<float>( m_DebugSettings.DebugFieldResolution ), static_cast<float>( m_DebugSettings.DebugFieldResolution ) },
                    { 0, 0, static_cast<float>( m_DebugSettings.RenderResolution ), static_cast<float>( m_DebugSettings.RenderResolution ) }, { 0, 0 }, 0, WHITE );
}

//-------------------------------------------------------------------------

// It is assumed that rlImGuiBegin() and rlImGuiEnd() are called outside this scope.;
void Simulation::DrawPhysicsOverlay() noexcept
{

    //-------------------------------------------------------------------------

    ImGui::Begin( " Physics Settings " );

    //-------------------------------------------------------------------------

    ImGui::Text( "%.1f FPS", static_cast<double>( GetFPS() ) );
    ImGui::Checkbox( "Pause", &m_PhysicsSettings.Paused );
    ImGui::SameLine();
    ImGui::InputFloat( "Slowdown", &m_PhysicsSettings.SlowMultiplier );
    if ( ImGui::Button( "StartSandbox" ) ) { InitSandbox(); }
    ImGui::SameLine();
    if ( ImGui::Button( "StartAirfoil" ) ) { InitAirfoil(); }
    ImGui::SliderFloat( " InvTimestepMultiplier ", &m_PhysicsSettings.InvTimestepMultiplier, 1.0f, 10.0f );

    //-------------------------------------------------------------------------

    ImGui::Separator();

    //-------------------------------------------------------------------------

    ImGui::Text( "Scheme" );
    if ( ImGui::Button( " Explicit " ) ) { m_PhysicsSettings.Scheme = UpdateScheme::Explicit; }
    ImGui::SameLine();
    if ( ImGui::Button( " Implicit " ) ) { m_PhysicsSettings.Scheme = UpdateScheme::Implicit; }

    ImGui::Text( "Kernal" );
    if ( ImGui::Button( "Spiky_1" ) ) { currentKernal = Kernal::Spiky1; }
    ImGui::SameLine();
    if ( ImGui::Button( "Spiky_2" ) ) { currentKernal = Kernal::Spiky2; }
    ImGui::SameLine();
    if ( ImGui::Button( "Spiky_3" ) ) { currentKernal = Kernal::Spiky3; }
    {
        //-------------------------------------------------------------------------

        ImGui::Separator();
    }

    //-------------------------------------------------------------------------

    ImGui::Text( "Parameters" );
    ImGui::InputFloat( "Smoothing radius", &m_PhysicsSettings.SmoothingRadius, 0.001f, 0.1f );
    ImGui::InputFloat( "Target Density", &m_PhysicsSettings.TargetDensity, 0.1f, 1.0f );
    ImGui::Separator();

    //-------------------------------------------------------------------------

    ImGui::Checkbox( " Pressure Force ", &m_PhysicsSettings.ApplyPressureForce );
    ImGui::SameLine();
    ImGui::Checkbox( " Viscocity ", &m_PhysicsSettings.ApplyViscocity );
    ImGui::SameLine();
    ImGui::Checkbox( " Gravity ", &m_PhysicsSettings.ApplyGravity );
    ImGui::SameLine();
    ImGui::Checkbox( " Mouse ", &m_PhysicsSettings.ApplyMouseForce );

    if ( m_PhysicsSettings.ApplyPressureForce ) { ImGui::InputFloat( "Pressure Multiplier", &m_PhysicsSettings.PressureMultiplier, 0.1f, 1.0f ); }
    if ( m_PhysicsSettings.ApplyViscocity ) { ImGui::InputFloat( "Viscocity", &m_PhysicsSettings.Viscocity, 0.01f, 0.1f ); }
    if ( m_PhysicsSettings.ApplyGravity ) { ImGui::InputFloat( "Gravity Multiplier", &m_PhysicsSettings.GravityMultiplier, 0.01f, 0.1f ); }

    //-------------------------------------------------------------------------

    ImGui::End();
}

void Simulation::DrawDebugOverlay() noexcept
{

    ImGui::Begin( "Debug Settings" );

    ImGui::Text( "Ball drawing" );
    ImGui::Checkbox( " Draw Balls ", &m_DebugSettings.Draw );
    ImGui::SameLine();
    ImGui::Checkbox( " Speed = Colors ", &m_DebugSettings.DrawSpeedColor );
    if ( m_DebugSettings.Draw ) { ImGui::SliderFloat( "Ball radius", &m_DebugSettings.ParticleDrawRadius, 0.0f, 10.0f ); }
    if ( m_DebugSettings.DrawSpeedColor ) { ImGui::SliderFloat( "Speed color cap", &m_DebugSettings.MaxSpeed, 0.0f, 10.0f ); }

    ImGui::Separator();
    ImGui::Text( "Debug Texture Mode" );
    if ( ImGui::Button( " None " ) ) { m_DebugSettings.Field = DebugField::None; }
    ImGui::SameLine();
    if ( ImGui::Button( " Density " ) ) { m_DebugSettings.Field = DebugField::Density; }
    ImGui::SameLine();
    if ( ImGui::Button( " Pressure " ) ) { m_DebugSettings.Field = DebugField::Pressure; }

    if ( m_DebugSettings.Field == DebugField::Density )
    {
        ImGui::SliderFloat( "Debug field max", &m_DebugSettings.DebugFieldMax, 0.0f, 100.0f );
        ImGui::SliderFloat( "Debug field min", &m_DebugSettings.DebugFieldMiddle, 0.0f, m_DebugSettings.DebugFieldMax );
    }
    else if ( m_DebugSettings.Field == DebugField::Pressure )
    {
        ImGui::SliderFloat( "Debug field max", &m_DebugSettings.DebugFieldMax, 0.0f, 100.0f );
        ImGui::SliderFloat( "Debug field middle", &m_DebugSettings.DebugFieldMiddle, m_DebugSettings.DebugFieldMin, m_DebugSettings.DebugFieldMax );
        ImGui::SliderFloat( "Debug field min", &m_DebugSettings.DebugFieldMin, -100.0f, 0.0f );
    }

    ImGui::End();
}

void Simulation::DrawAirfoilOverlay() noexcept
{
    ImGui::Begin( "Airfoil" );
    ImGui::Checkbox( " Enable Airfoil ", &m_AFEnabled );
    if ( m_AFEnabled )
    {
        ImGui::SliderFloat( " Offset-X ", &m_AFOffset.x, 0.0f, m_PhysicsSettings.SimulationResolution );
        ImGui::SliderFloat( " Offset-Y ", &m_AFOffset.y, 0.0f, m_PhysicsSettings.SimulationResolution );
        ImGui::SliderFloat( " Scale ", &m_AFScale, 0.0f, 10.0f );
        ImGui::SliderFloat( " Rotation ", &m_AFRotation, 0.0f, 2 * PI );
    }
    ImGui::End();
}

//-------------------------------------------------------------------------
