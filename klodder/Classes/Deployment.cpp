#include "Deployment.h"

namespace Deployment
{
#define String(name, value) const char* name = value;
#include "Deployment.inc"
#undef String
}
