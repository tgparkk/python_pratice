#pragma once

extern thread_local __int32 LThreadId;

================================================================================
// CoreTLS.cpp file content
================================================================================

#include "pch.h"
#include "CoreTLS.h"

thread_local __int32 LThreadId = 0;