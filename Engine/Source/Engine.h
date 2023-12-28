#pragma once

#ifdef _DEBUG
#define EG_CONFIRM(x) \
if(!(x))\
{\
__debugbreak();\
}
#else
#define EG_CONFIRM(x) x
#endif

