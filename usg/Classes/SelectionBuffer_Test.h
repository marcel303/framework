#pragma once

#include "SelectionBuffer.h"

// draws the selection buffer to the currently bound OpenGL texture

void SelectionBuffer_ToTexture(SelectionBuffer* buffer, int offsetX, int offsetY, int textureSx, int textureSy);
