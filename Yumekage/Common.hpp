#pragma once
#pragma region CompilerWarningAndErrorFixes
// Disable warning for "Banned API ExAllocatePool*" | Reason: Replacements are possibly not available in older versions of windows.
#pragma warning( disable : 28751 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4201 )
#pragma error( disable : 2486 )
#pragma endregion

#define _VERBOSE_ // Comment this line out to disable debug logging completely.
//#define _FILEVERBOSE_ // Uncomment this line to show file path, line number, and function name for debug logging.

#include <ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntimage.h>
#include <intrin.h>

#include "ia32.hpp"
#include "Misc/DynamicArray.hpp"

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

// Basic macros and definitions.
#pragma region Definitions
#define NTHEADER( Base ) PIMAGE_NT_HEADERS64( uint64_t( Base ) + PIMAGE_DOS_HEADER( Base )->e_lfanew )
#define _IMPORT_ extern "C" __declspec( dllimport )
#define GetBuildNumber( ) SharedUserData->NtBuildNumber

#define WIN10_BN_1709 16299
#define WIN10_BN_1803 17134
#define WIN10_BN_1809 17763
#define WIN10_BN_1903 18362
#define WIN10_BN_1909 18363
#define WIN10_BN_20H1 19041
#define WIN10_BN_20H2 19042
#define WIN10_BN_21H1 19043
#define WIN10_BN_21H2 19044
#define WIN10_BN_22H2 19045
#define WIN11_BN_21H2 22000
#define WIN11_BN_22H2 22621
#pragma endregion

#pragma region DebugLogging
#ifdef _VERBOSE_
#	ifdef _FILEVERBOSE_
#		define DBG( Fmt, ... )																							\
			{																											\
				DbgPrintEx( 0, 0, "[Yumekage:CORE <%s:%d %s>] "##Fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__ );	\
			}
#	else
#		define DBG( Fmt, ... )												\
			{																\
				DbgPrintEx( 0, 0, "[Yumekage:CORE] "##Fmt, __VA_ARGS__ );	\
			}
#	endif
#else
#	define DBG( ... )
#endif

#define PANIC( BugcheckCode, Fmt, ... )												\
	{																				\
		DbgPrintEx( 0, 0, "\n[Yumekage:PANIC] !!! "##Fmt##" !!!\n", __VA_ARGS__ );	\
		BugcheckCode ? KeBugCheck( BugcheckCode ) : __debugbreak( );				\
	}
#pragma endregion

#pragma region GlobalVars
extern DynamicArray<void*> MemoryAllocations;
#pragma endregion