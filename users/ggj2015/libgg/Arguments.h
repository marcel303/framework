#pragma once

#define ARGS_CHECKPARAM(n) { if (i + n >= argc) throw ExceptionVA("missing parameter"); }
