#pragma once

void animationTestInit();
bool animationTestIsActive();
void animationTestToggleIsActive();
void animationTestChangeAnim(int direction, int x, int y);
void animationTestTick(float dt);
void animationTestDraw();

void blastEffectTestToggleIsActive();
void blastEffectTestTick(float dt);
void blastEffectTestDraw();

void gifCaptureToggleIsActive(bool cancel);
void gifCaptureComplete(bool cancel);
void gifCaptureTick(float dt);
void gifCaptureTick_PostRender();
