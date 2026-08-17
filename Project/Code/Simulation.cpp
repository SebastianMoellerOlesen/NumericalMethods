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
    SetTargetFPS( m_PhysicsSettings.TargetFPS ); // Debatable if it should be in the physics settings...

    BeginDrawing();
    EndDrawing();

    // Fixes mouse scaling for HighDPI displays?
    // At least for my config. Milage may vary
    SetMouseScale( 1.0f, 1.0f );

    //-------------------------------------------------------------------------

    rlImGuiSetup( true );

    //-------------------------------------------------------------------------

    m_DebugSettings.RenderResolution = size;

    //-------------------------------------------------------------------------

    // Assign the points for the BC
    m_BorderP1.insert( m_BorderP1.begin(), { { 0, 0 }, { m_PhysicsSettings.SimulationResolution, 0 }, { m_PhysicsSettings.SimulationResolution, m_PhysicsSettings.SimulationResolution }, { 0, m_PhysicsSettings.SimulationResolution } } );
    m_BorderP2.insert( m_BorderP2.begin(), { { m_PhysicsSettings.SimulationResolution, 0 }, { m_PhysicsSettings.SimulationResolution, m_PhysicsSettings.SimulationResolution }, { 0, m_PhysicsSettings.SimulationResolution }, { 0, 0 } } );

    //-------------------------------------------------------------------------

    m_Masses = 1.0f;

    //-------------------------------------------------------------------------

    // Generate a random initial position.
    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
    {
        // Position
        float x = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        float y = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        m_Positions.push_back( Vector2( x, y ) / m_DebugSettings.RenderResolution * m_PhysicsSettings.SimulationResolution );

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
    m_Densities.resize( m_PhysicsSettings.ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressureGradiant();

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

Vector2 Simulation::WorldSpaceToScreenSpace( Vector2 WS ) noexcept
{
    return Vector2Scale( WS, m_DebugSettings.RenderResolution / m_PhysicsSettings.SimulationResolution );
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
    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
    {
        // Position
        float x        = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        float y        = GetRandomValue( 0.0f, m_DebugSettings.RenderResolution );
        m_Positions[i] = Vector2( x, y ) / m_DebugSettings.RenderResolution * m_PhysicsSettings.SimulationResolution;

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
    m_Densities.resize( m_PhysicsSettings.ParticleCount );
    UpdateDensities();

    m_Pressures.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressures();

    m_PressureGradiants.resize( m_PhysicsSettings.ParticleCount );
    UpdatePressureGradiant();
}

//-------------------------------------------------------------------------

void Simulation::SetScheme( UpdateScheme scheme ) noexcept { m_PhysicsSettings.Scheme = scheme; }

void Simulation::Update() noexcept
{

    if ( m_PhysicsSettings.Paused )
    {
        return;
    }

    //-------------------------------------------------------------------------

    switch ( m_PhysicsSettings.Scheme )
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
        m_Positions[i] += m_Velocities[i] * m_PhysicsSettings.DeltaTime;
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
        m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * m_PhysicsSettings.DeltaTime;
        m_Positions[i]  += m_Velocities[i] * m_PhysicsSettings.DeltaTime;
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
        m_Velocities[i] += m_PressureGradiants[i] / m_Densities[i] * m_PhysicsSettings.DeltaTime;
        m_Positions[i]  += m_Velocities[i] * m_PhysicsSettings.DeltaTime;
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

    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
    {
        float distance   = Vector2Length( location - m_Positions[i] );
        float influence  = SimpleSmoothingKernal2D( m_PhysicsSettings.SmoothingRadius, distance );
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

    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
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

        float influence  = SimpleSmoothingKernal2D( m_PhysicsSettings.SmoothingRadius, distance );
        gradient        += Vector2Normalize( difference ) * m_Masses * influence * m_Pressures[i] / m_Densities[i];
    }

    return gradient;
}

Vector2 Simulation::CalculatePressureGradiant( uint32_t index ) noexcept
{
    //-------------------------------------------------------------------------

    Vector2 gradient = Vector2Zero();

    //-------------------------------------------------------------------------

    for ( uint32_t i = 0; i < m_PhysicsSettings.ParticleCount; i++ )
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

        float influence       = SimpleSmoothinKernalDerivative2D( m_PhysicsSettings.SmoothingRadius, distance );
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

    if ( m_DebugSettings.Draw )
    {
        for ( uint32_t i = 0; i < m_BorderP1.size(); i++ )
        {
            DrawLineEx( WorldSpaceToScreenSpace( m_BorderP1[i] ), WorldSpaceToScreenSpace( m_BorderP2[i] ), 3.0f, BLACK );
        }

        //-------------------------------------------------------------------------

        for ( uint32_t i = 0; i < m_Positions.size(); i++ )
        {
            DrawCircleV( WorldSpaceToScreenSpace( m_Positions[i] ), m_DebugSettings.ParticleDrawRadius, m_ParticleColors[i] );
        }
    }

    //-------------------------------------------------------------------------

    DrawPhysicsOverlay();
    DrawDebugOverlay();

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
    if ( ImGui::Button( "Restart" ) ) { Restart(); }

    //-------------------------------------------------------------------------

    ImGui::Separator();

    //-------------------------------------------------------------------------

    ImGui::Text( "Scheme" );

    if ( ImGui::Button( " Explicit " ) ) { SetScheme( UpdateScheme::Explicit ); }
    ImGui::SameLine();
    if ( ImGui::Button( " Implicit " ) ) { SetScheme( UpdateScheme::Implicit ); }

    //-------------------------------------------------------------------------

    ImGui::Separator();

    //-------------------------------------------------------------------------

    ImGui::Text( "Parameters" );

    ImGui::InputFloat( "Smoothing radius", &m_PhysicsSettings.SmoothingRadius, 0.001f, 0.1f );
    ImGui::InputFloat( "Pressure Multiplier", &m_PhysicsSettings.PressureMultiplier, 0.1f, 1.0f );
    ImGui::InputFloat( "Target Density", &m_PhysicsSettings.TargetDensity, 0.1f, 1.0f );
    ImGui::End();
}

void Simulation::DrawDebugOverlay() noexcept
{

    ImGui::Begin( "Debug Settings" );

    ImGui::Text( "Ball drawing" );
    ImGui::Checkbox( " Draw Balls ", &m_DebugSettings.Draw );
    if ( m_DebugSettings.Draw ) { ImGui::SliderFloat( "Ball radius", &m_DebugSettings.ParticleDrawRadius, 0.0f, 10.0f ); }

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

//-------------------------------------------------------------------------
