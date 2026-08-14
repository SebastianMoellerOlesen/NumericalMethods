#pragma once

#include <raylib.h>
#include <raymath.h>

//-------------------------------------------------------------------------

inline Vector2 GetMousePosScaled()
{
    return GetMousePosition() * GetWindowScaleDPI();
}

//-------------------------------------------------------------------------
