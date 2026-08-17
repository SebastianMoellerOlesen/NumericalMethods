#include "Simulation.hpp"
#include "SmoothingKernals.hpp"
#include "Utils.hpp"

#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
#include <rlImGui.h>
#include <rlgl.h>
#include <imgui_impl_raylib.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <unistd.h>
#include <utility>
#include <vector>

//-------------------------------------------------------------------------

Simulation::Simulation( uint32_t size )
{

    //-------------------------------------------------------------------------

    SetConfigFlags( FLAG_WINDOW_HIGHDPI );
    InitWindow( size, size, "Simulation" );
    SetTargetFPS( m_TargetFPS );

    BeginDrawing();
    EndDrawing();

    // Fixes mouse scaling for HighDPI displays?
    // At least for my config. Milage may vary
    SetMouseScale( 1.0f, 1.0f );

    //-------------------------------------------------------------------------

    rlImGuiSetup( true );

    //-------------------------------------------------------------------------

    m_RenderResolution = size;

    //-------------------------------------------------------------------------

    // Assign the points for the BC
    m_BorderP1.insert( m_BorderP1.begin(), { { 0, 0 }, { m_SimulationResolution, 0 }, { m_SimulationResolution, m_SimulationResolution }, { 0, m_SimulationResolution } } );
    m_BorderP2.insert( m_BorderP2.begin(), { { m_SimulationResolution, 0 }, { m_SimulationResolution, m_SimulationResolution }, { 0, m_SimulationResolution }, { 0, 0 } } );

    //-------------------------------------------------------------------------

    m_Masses = 1.0f;

    //-------------------------------------------------------------------------

    // Generate a random initial position.
    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
    {
        // Position
        float x = GetRandomValue( 0.0f, m_RenderResolution );
        float y = GetRandomValue( 0.0f, m_RenderResolution );
        m_Positions.push_back( Vector2( x, y ) / m_RenderResolution * m_SimulationResolution );

        //-------------------------------------------------------------------------

        float dx = 0.0f;
        float dy = 0.0f;

        // Set a random velocity:
        // float max = 5.0f;
        // dx        = GetRandomValue( -max, max );
        // dy        = GetRandomValue( -max, max );;

        m_Velocities.push_back( Vector2( dx, dy ) );

        //-------------------------------------------------------------------------

        m_ParticleColors.push_back( BLACK );

        //-------------------------------------------------------------------------
    }

    // Initialize the fields.
    m_Densities.resize( m_ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_ParticleCount );
    UpdatePressureGradiant();

    // Create the debug texture:
    m_DebugPixels.resize( m_DebugFieldResolution * m_DebugFieldResolution );

    Image debugImg = {
        .data    = m_DebugPixels.data(),
        .width   = static_cast<int>( m_DebugFieldResolution ),
        .height  = static_cast<int>( m_DebugFieldResolution ),
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

    m_DebugTexture = LoadTextureFromImage( debugImg );
    SetTextureFilter( m_DebugTexture, TEXTURE_FILTER_BILINEAR );
}

Simulation::~Simulation()
{
    UnloadTexture( m_DebugTexture );
    rlImGuiShutdown();
    CloseWindow();
}

Vector2 Simulation::WorldSpaceToScreenSpace( Vector2 WS ) noexcept
{
    return Vector2Scale( WS, m_RenderResolution / m_SimulationResolution );
}

//-------------------------------------------------------------------------

void Simulation::Run() noexcept
{
    while ( !WindowShouldClose() )
    {
        Update();
        Render();
    }
}

void Simulation::Restart() noexcept
{
    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
    {
        // Position
        float x        = GetRandomValue( 0.0f, m_RenderResolution );
        float y        = GetRandomValue( 0.0f, m_RenderResolution );
        m_Positions[i] = Vector2( x, y ) / m_RenderResolution * m_SimulationResolution;

        //-------------------------------------------------------------------------

        float dx = 0.0f;
        float dy = 0.0f;

        // Set a random velocity:
        // float max = 5.0f;
        // dx        = GetRandomValue( -max, max );
        // dy        = GetRandomValue( -max, max );;

        m_Velocities[i] = Vector2( dx, dy );

        //-------------------------------------------------------------------------
    }

    // Initialize the fields.
    m_Densities.resize( m_ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_ParticleCount );
    UpdatePressureGradiant();
}

//-------------------------------------------------------------------------

void Simulation::SetScheme( UpdateScheme scheme ) noexcept { m_UpdateScheme = scheme; }

void Simulation::Update() noexcept
{

    if ( m_Paused )
    {
        return;
    }

    //-------------------------------------------------------------------------

    switch ( m_UpdateScheme )
    {
        case UpdateScheme::Explicit:
            ExplicitUpdate();
            break;

        case UpdateScheme::Implicit:
            ImplicitUpdate();
            break;
    }
}

void Simulation::ImplicitUpdate() noexcept
{

    //-------------------------------------------------------------------------

    // Save the old position so we can restore it after calculating the forces.
    // It might also be viable to create another vector in the class, to hold forwardPos...
    std::vector<Vector2> pos = m_Positions;

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        m_Positions[i] += m_Velocities[i] * m_DeltaTime;
    }

    //-------------------------------------------------------------------------

    // Update velocity implicitly.
    UpdateDensities();
    UpdatePressures();
    UpdatePressureGradiant();

    //-------------------------------------------------------------------------

    // Restore the old pos, so we can update it using the new vel.
    m_Positions = std::move( pos );

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * m_DeltaTime;
        m_Positions[i]  += m_Velocities[i] * m_DeltaTime;
    }

    //-------------------------------------------------------------------------

    // Make sure the balls stay inside the box.
    // Acts like a reflective BC i guess...
    HandleBorderCollision();
}

void Simulation::ExplicitUpdate() noexcept
{

    //-------------------------------------------------------------------------

    UpdateDensities();
    UpdatePressures();
    UpdatePressureGradiant();

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * m_DeltaTime;
        m_Positions[i]  += m_Velocities[i] * m_DeltaTime;
    }

    //-------------------------------------------------------------------------

    // Make sure the balls stay inside the box.
    // Acts like a reflective BC i guess...
    HandleBorderCollision();
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

            Vector2 normal = Vector2Normalize( Vector2Rotate( P2 - P1, PI / 2 ) );
            float   offset = Vector2DotProduct( normal, P1 - m_Positions[i] );

            // If we are outside our box:
            if ( offset > 0 )
            {
                pos += normal * offset;                               // Push into the inside of the box.
                vel -= normal * Vector2DotProduct( vel, normal ) * 2; // Reflect the particle.
            }
        }
    }
}

//-------------------------------------------------------------------------

float Simulation::CalculateDensity( Vector2 location ) noexcept
{
    float density = 0;

    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
    {
        float distance   = Vector2Length( location - m_Positions[i] );
        float influence  = SimpleSmoothingKernal2D( m_SmoothingRadius, distance );
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
    newDensities.resize( m_ParticleCount );

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
    float difference = CalculateDensity( location ) - m_TargetDensity;
    return difference * m_PressureMultiplier;
}

float Simulation::CalculatePressure( uint32_t index ) noexcept
{
    float difference = m_Densities[index] - m_TargetDensity;
    return difference * m_PressureMultiplier;
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

void Simulation::UpdatePressureGradiant() noexcept
{
    std::for_each( std::execution::par_unseq, m_Positions.begin(), m_Positions.end(), [this]( const Vector2& pos ) {
        size_t i                     = &pos - m_Positions.data();
        this->m_PressureGradiants[i] = this->CalculatePressureGradiant( i );
    } );
}

Vector2 Simulation ::CalculatePressureGradiant( Vector2 location ) noexcept
{
    Vector2 gradient = Vector2Zero();

    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
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

        float influence  = SimpleSmoothingKernal2D( m_SmoothingRadius, distance );
        gradient        += Vector2Normalize( difference ) * m_Masses * influence * m_Pressures[i] / m_Densities[i];
    }

    return gradient;
}

Vector2 Simulation::CalculatePressureGradiant( uint32_t index ) noexcept
{
    //-------------------------------------------------------------------------

    Vector2 gradient = Vector2Zero();

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_ParticleCount; i++ )
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

        float influence       = SimpleSmoothinKernalDerivative2D( m_SmoothingRadius, distance );
        float averageDensity  = ( m_Densities[i] + m_Densities[index] ) * 0.5f;
        gradient             -= Vector2Normalize( difference ) * m_Masses * influence * m_Pressures[i] / averageDensity;

        //-------------------------------------------------------------------------
    }

    //-------------------------------------------------------------------------

    return gradient;

    //-------------------------------------------------------------------------
}

//-------------------------------------------------------------------------

void Simulation::Render() noexcept
{

    //-------------------------------------------------------------------------

    BeginDrawing();
    rlImGuiBegin();

    //-------------------------------------------------------------------------

    ClearBackground( RAYWHITE );

    // DrawDensity();
    // DrawPressure();

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_BorderP1.size(); i++ )
    {
        DrawLineEx( WorldSpaceToScreenSpace( m_BorderP1[i] ), WorldSpaceToScreenSpace( m_BorderP2[i] ), 3.0f, BLACK );
    }

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_Positions.size(); i++ )
    {
        DrawCircleV( WorldSpaceToScreenSpace( m_Positions[i] ), m_ParticleDrawRadius, m_ParticleColors[i] );
    }

    //-------------------------------------------------------------------------

    ImGui::Begin( "Parameters" );
    ImGui::Checkbox( "Pause simulation", &m_Paused );

    if ( ImGui::Button( " Restart Simulation " ) )
    {
        Restart();
    }

    ImGui::Separator();

    ImGui::Text( "Integration Scheme:" );
    if ( ImGui::Button( " Explicit " ) )
    {
        SetScheme( UpdateScheme::Explicit );
    }
    ImGui::SameLine();
    if ( ImGui::Button( " Implicit " ) )
    {
        SetScheme( UpdateScheme::Implicit );
    }

    ImGui::Separator();
    ImGui::SliderFloat( "Smoothing radius", &m_SmoothingRadius, 0.01f, 2.0f );
    ImGui::SliderFloat( "Pressure Multiplier", &m_PressureMultiplier, 0.0f, 10.0f );
    ImGui::SliderFloat( "Target Density", &m_TargetDensity, 0.0f, 20.0f );
    ImGui::Separator();
    ImGui::Text( "%.1f FPS", static_cast<double>( GetFPS() ) );
    ImGui::End();

    //-------------------------------------------------------------------------

    rlImGuiEnd();
    EndDrawing();
}

//-------------------------------------------------------------------------

void Simulation::DrawDensity() noexcept
{
    ImGui::Begin( " Debug Params " );

    ImGui::Separator();
    ImGui::SliderFloat( "Debug field max", &m_DebugFieldMax, 0.0f, 100.0f );
    ImGui::SliderFloat( "Debug field min", &m_DebugFieldMiddle, 0.0f, m_DebugFieldMax );

    ImGui::End();

    // How does each pixel location map to the world?
    float cellToWorld = m_SimulationResolution / m_DebugFieldResolution;

    std::for_each( std::execution::par_unseq, m_DebugPixels.begin(), m_DebugPixels.end(),
                   [this, cellToWorld]( Color& pixel ) {
                       //-------------------------------------------------------------------------

                       // Get the row and column for the current idx.
                       size_t i   = &pixel - this->m_DebugPixels.data();
                       size_t col = i % this->m_DebugFieldResolution;
                       size_t row = i / this->m_DebugFieldResolution;

                       //-------------------------------------------------------------------------

                       Vector2 world   = { ( col + 0.5f ) * cellToWorld, ( row + 0.5f ) * cellToWorld };
                       float   density = this->CalculateDensity( world );

                       //-------------------------------------------------------------------------

                       // Map the density to the color.
                       float t = ( density - this->m_DebugFieldMiddle ) / ( this->m_DebugFieldMax - this->m_DebugFieldMiddle );
                       pixel   = ColorLerp( this->m_DebugMiddleColor, this->m_DebugMaxColor, t );

                       //-------------------------------------------------------------------------
                   } );

    // Update the texture data, and draw it.
    UpdateTexture( m_DebugTexture, m_DebugPixels.data() );

    DrawTexturePro( m_DebugTexture, { 0, 0, static_cast<float>( m_DebugFieldResolution ), static_cast<float>( m_DebugFieldResolution ) },
                    { 0, 0, static_cast<float>( m_RenderResolution ), static_cast<float>( m_RenderResolution ) }, { 0, 0 }, 0, WHITE );
}

void Simulation::DrawPressure() noexcept
{

    ImGui::Begin( " Debug Params " );

    ImGui::Separator();
    ImGui::SliderFloat( "Debug field max", &m_DebugFieldMax, 0.0f, 50.0f );
    ImGui::SliderFloat( "Debug field min", &m_DebugFieldMin, -50.0f, 0.0f );
    ImGui::SliderFloat( "Debug field middle", &m_DebugFieldMiddle, m_DebugFieldMin, m_DebugFieldMax );

    ImGui::End();

    // How does each pixel location map to the world?
    float cellToWorld = m_SimulationResolution / m_DebugFieldResolution;

    std::for_each( std::execution::par_unseq, m_DebugPixels.begin(), m_DebugPixels.end(),
                   [this, cellToWorld]( Color& pixel ) {
                       //-------------------------------------------------------------------------

                       // Get the row and column for the current idx.
                       size_t i   = &pixel - this->m_DebugPixels.data();
                       size_t col = i % this->m_DebugFieldResolution;
                       size_t row = i / this->m_DebugFieldResolution;

                       //-------------------------------------------------------------------------

                       Vector2 world    = { ( col + 0.5f ) * cellToWorld, ( row + 0.5f ) * cellToWorld };
                       float   pressure = this->CalculatePressure( world );

                       //-------------------------------------------------------------------------

                       // Map the density to the color.

                       if ( pressure < m_DebugFieldMiddle )
                       {
                           float t = ( pressure - this->m_DebugFieldMin ) / ( this->m_DebugFieldMiddle - this->m_DebugFieldMin );
                           pixel   = ColorLerp( this->m_DebugMinColor, this->m_DebugMiddleColor, t );
                       }
                       else
                       {
                           float t = ( pressure - this->m_DebugFieldMiddle ) / ( this->m_DebugFieldMax - this->m_DebugFieldMiddle );
                           pixel   = ColorLerp( this->m_DebugMiddleColor, this->m_DebugMaxColor, t );
                       }

                       //-------------------------------------------------------------------------
                   } );

    // Update the texture data, and draw it.
    UpdateTexture( m_DebugTexture, m_DebugPixels.data() );

    DrawTexturePro( m_DebugTexture, { 0, 0, static_cast<float>( m_DebugFieldResolution ), static_cast<float>( m_DebugFieldResolution ) },
                    { 0, 0, static_cast<float>( m_RenderResolution ), static_cast<float>( m_RenderResolution ) }, { 0, 0 }, 0, WHITE );
}
