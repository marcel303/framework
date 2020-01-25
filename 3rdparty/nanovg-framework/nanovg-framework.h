#pragma once

struct NVGcontext;

NVGcontext * nvgCreateFramework(int flags);
void nvgDeleteFramework(NVGcontext * ctx);
