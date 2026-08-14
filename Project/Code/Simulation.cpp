#include "Simulation.hpp"

#include <raylib.h>
#include <raymath.h>

#include <cstdint>

//-------------------------------------------------------------------------

Simulation::Simulation( uint32_t width, uint32_t height, uint32_t ballCount, uint32_t targetFPS, float ballRadius ) : m_Radius( ballRadius )
{

    //-------------------------------------------------------------------------

    SetConfigFlags( FLAG_WINDOW_HIGHDPI );
    InitWindow( width, height, "Simulation" );
    SetTargetFPS( targetFPS );

    //-------------------------------------------------------------------------

    m_DeltaTime = 1.0f / targetFPS;

    //-------------------------------------------------------------------------

    // Due to DPI Scaling, we need to get the size of the window in logical pixels.
    Vector2 DPIScaling = GetWindowScaleDPI();
    m_RenderWidth      = width * DPIScaling.x;
    m_RenderHeight     = height * DPIScaling.y;

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

        m_Positions.push_back( Vector2( x, y ) );

        //-------------------------------------------------------------------------

        // Velocity
        float max = 5.0f;
        float dx  = GetRandomValue( -max, max );
        float dy  = GetRandomValue( -max, max );

        m_Velocities.push_back( Vector2( dx, dy ) );

        //-------------------------------------------------------------------------

        // Colors
        // Note: This might be usefull later.
        m_Colors.push_back( BLACK );
    }

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

void Simulation::Run() noexcept
{
    while ( !WindowShouldClose() )
    {
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
        m_Positions[i] += m_Velocities[i] * m_DeltaTime;
    }

    // Make sure the balls stay inside the box.
    HandleBorderCollision();
}

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

            if ( CheckCollisionCircleLine( m_Positions[i], m_Radius, P1, P2 ) )
            {
                Vector2 normalLine  = Vector2Normalize( Vector2Rotate( P2 - P1, PI / 2 ) );
                vel                -= Vector2Scale( normalLine, 2 * Vector2DotProduct( vel, normalLine ) );

                // Push out the circle:
                float indent  = Vector2DotProduct( P1 - pos, normalLine );
                pos          += Vector2Scale( normalLine, m_Radius + indent );
            }
        }
    }
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
        DrawCircleV( m_Positions[i], m_Radius, m_Colors[i] );
    }

    //-------------------------------------------------------------------------

    EndDrawing();
}
