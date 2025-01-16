#pragma once

extern thread_local __int32 LThreadId;

================================================================================
// CoreTLS.cpp 파일 내용
================================================================================

#include "pch.h"
#include "CoreTLS.h"

thread_local __int32 LThreadId = 0;