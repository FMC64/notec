#pragma once

#include <setjmp.h>

#define unwind_capable jmp_buf _unwind_pos
#define catch(e) if (setjmp(e._unwind_pos) != 0)
#define throw longjmp(_unwind_pos, 1); __builtin_unreachable()