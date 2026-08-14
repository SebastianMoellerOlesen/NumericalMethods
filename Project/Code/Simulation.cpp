#include "Simulation.hpp"
#include "SmoothingKernals.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <cstddef>
#include <execution>
#include <raylib.h>
#include <raymath.h>

#include <cstdint>
#include <utility>
#include <vector>

//-------------------------------------------------------------------------

Simulation::Simulation( uint32_t width, uint32_t height, uint32_t ballCount, uint32_t targetFPS, float ballRadius ) : m_DrawRadius( ballRadius ), m_ParticleCount( ballCount )
{

    //-------------------------------------------------------------------------

    SetConfigFlags( FLAG_WINDOW_HIGHDPI );
    InitWindow( width, height, "Simulation" );
    SetTargetFPS( targetFPS );

    //-------------------------------------------------------------------------

    m_DeltaTime = 1.0f / targetFPS;

    //-------------------------------------------------------------------------

    // Due to DPI Scaling, we need to get the size of the window in logical pixels.
    // Note: For some reason the scaling doesn't work, but the balls are still placed correctly.
    // This might be due to how the FLAG_WINDOW_HIGHDPI works, but not sure...
    // m_DPIScaling   = GetWindowScaleDPI();
    m_RenderWidth  = width;  // * m_DPIScaling.x;
    m_RenderHeight = height; // * m_DPIScaling.y;

    //-------------------------------------------------------------------------

    // Assign the points for the border.
    m_BorderP1.insert( m_BorderP1.begin(), { { 0, 0 }, { m_RenderWidth, 0 }, { m_RenderWidth, m_RenderHeight }, { 0, m_RenderHeight } } );
    m_BorderP2.insert( m_BorderP2.begin(), { { m_RenderWidth, 0 }, { m_RenderWidth, m_RenderHeight }, { 0, m_RenderHeight }, { 0, 0 } } );

    //-------------------------------------------------------------------------

    // Generate a random initial position.
    for ( uint32_t i = 0; i < ballCount; i++ )
    {

        // Position
        float x = GetRandomValue( 0.0f, m_RenderWidth );
        float y = GetRandomValue( 0.0f, m_RenderHeight );
        // x = i % 100;
        // y = float( i ) / 10;

        m_Positions.push_back( Vector2( x, y ) );

        //-------------------------------------------------------------------------

        // Velocity
        float max = 5.0f;
        float dx  = GetRandomValue( -max, max );
        float dy  = GetRandomValue( -max, max );

        dx = 0;
        dy = 0;

        m_Velocities.push_back( Vector2( dx, dy ) );

        //-------------------------------------------------------------------------

        m_DensityGradients.resize( ballCount );
        m_Densities.resize( ballCount );

        //-------------------------------------------------------------------------

        // Note: This might be usefull later.
        m_Colors.push_back( BLACK );
        m_Masses.push_back( 1.0f );
    }

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

void Simulation::Run() noexcept
{
    while ( !WindowShouldClose() )
    {
        PollInputEvents();

        Update();
        Render();
    }

    CloseWindow();
}

//-------------------------------------------------------------------------

void Simulation::Update() noexcept
{

    // Explicit.
    // When we have forces, this should mabey change.
    //-------------------------------------------------------------------------
    // Note: This is tested to me around 2x faster if flattening the vectors into regular floats.
    // Each iteration takes less than 1 micro second for 1000 particles, so might not be worth it.
    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        float density    = m_Densities[i] == 0 ? 1 : m_Densities[i];
        m_Velocities[i] += m_DensityGradients[i] * m_DeltaTime * 500 / density;
        m_Velocities[i] += Vector2( 0, 1 ) * m_DeltaTime;
        m_Positions[i]  += m_Velocities[i] * m_DeltaTime;
    }

    // Make sure the balls stay inside the box.
    HandleBorderCollision();

    UpdateDensities();
    UpdateDensityGradient();
}

//-------------------------------------------------------------------------

void Simulation::HandleBorderCollision() noexcept
{
    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {

        Vector2& pos = m_Positions[i];
        Vector2& vel = m_Velocities[i];

        for ( uint32_t k = 0; k < m_BorderP1.size(); k++ )
        {
            Vector2 P1 = m_BorderP1[k];
            Vector2 P2 = m_BorderP2[k];

            if ( CheckCollisionCircleLine( m_Positions[i], m_DrawRadius, P1, P2 ) )
            {
                Vector2 normalLine  = Vector2Normalize( Vector2Rotate( P2 - P1, PI / 2 ) );
                vel                -= Vector2Scale( normalLine, 2 * Vector2DotProduct( vel, normalLine ) );

                // Push out the circle:
                float indent  = Vector2DotProduct( P1 - pos, normalLine );
                pos          += Vector2Scale( normalLine, m_DrawRadius + indent );
            }
        }
    }
}

//-------------------------------------------------------------------------

float Simulation::CalculateDensity( Vector2 location ) const noexcept
{
    float density = 0;

    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
    {
        float distance   = Vector2Length( location - m_Positions[i] );
        float influence  = SimpleSmoothingKernal2D( m_InfluenceRadius, distance );
        density         += m_Masses[i] * influence;
    }

    return density;
}

//-------------------------------------------------------------------------

Vector2 Simulation::CalculateDensityGradiant( Vector2 location ) const noexcept
{
    Vector2 gradiant = Vector2Zero();

    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
    {
        Vector2 difference = location - m_Positions[i];

        float distance  = Vector2Length( difference );
        float influence = SimpleSmoothingKernal2D( m_InfluenceRadius, distance );

        gradiant += Vector2Normalize( difference ) * m_Masses[i] * influence;

        if ( IsMouseButtonDown( MOUSE_BUTTON_LEFT ) )
        {
            difference = location - GetMousePosScaled();
            distance   = Vector2Length( difference );

            float threshold = 200;
            if ( distance < threshold )
            {
                gradiant -= Vector2Normalize( difference ) * 1.0f * m_Masses[i] * influence * distance / threshold;
            }
        }
    }

    return gradiant;
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
    newDensities.reserve( m_ParticleCount );

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

void Simulation::UpdateDensityGradient() noexcept
{

    //-------------------------------------------------------------------------

    // Explicit update.
    // Density^(n+1) = F(Density^(n), Position^(n))
    // Mabey, we should use some other method for better stability.

    //-------------------------------------------------------------------------

    std::vector<Vector2> newGradiants;
    newGradiants.reserve( m_ParticleCount );

    //-------------------------------------------------------------------------

    std::for_each( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(),
                   [this, &newGradiants]( const Vector2& pos ) {
                       size_t i        = &pos - m_Positions.data();
                       newGradiants[i] = this->CalculateDensityGradiant( pos );
                   } );

    //-------------------------------------------------------------------------

    // Mabey this is not the best approach.
    // We might wan't to store both the old and the new for some reason.
    // It should be easy to implement though...
    m_DensityGradients = std::move( newGradiants );

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

void Simulation::Render() const noexcept
{

    //-------------------------------------------------------------------------

    BeginDrawing();

    //-------------------------------------------------------------------------

    ClearBackground( RAYWHITE );

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_BorderP1.size(); i++ )
    {
        DrawLineEx( m_BorderP1[i], m_BorderP2[i], 3.0f, BLACK );
    }

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        DrawCircleV( m_Positions[i], m_DrawRadius, m_Colors[i] );
    }

    //-------------------------------------------------------------------------

    EndDrawing();
}
