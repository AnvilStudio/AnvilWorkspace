#pragma once

#include "../../Core/Macros.h"
#include "../UMacros.h"

#include <exception>

#ifdef PLATFORM_WIN32
#include <Windows.h>
#include <DbgHelp.h>
#endif

namespace anv
{
	namespace crash
	{
		class CrashHandler
		{
		public:
#ifdef PLATFORM_WIN32
			inline static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* _exc)
			{
				ANV_LOG_ERROR("[CRASH DETECTED]: %08X (%s), Address: 0x%p", 
					_exc->ExceptionRecord->ExceptionCode,
					get_exc_desc(_exc->ExceptionRecord->ExceptionCode),
					_exc->ExceptionRecord->ExceptionAddress);
				return EXCEPTION_EXECUTE_HANDLER;
			}
#endif
			inline static void Init()
			{
#ifdef PLATFORM_WIN32
				SetUnhandledExceptionFilter(ExceptionHandler);
#endif
			}

			inline static const char* get_exc_desc(DWORD exceptionCode) {
#ifdef PLATFORM_WIN32
				switch (exceptionCode) {
				case EXCEPTION_ACCESS_VIOLATION: return "Access Violation";
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array Bounds Exceeded";
				case EXCEPTION_DATATYPE_MISALIGNMENT: return "Data Misalignment";
				case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float Divide by Zero";
					// Add other cases as needed
				default: return "Unknown Exception";
				}
#endif
			}

			inline static void prnt_call_stack()
			{
#ifdef PLATFORM_WIN32
				const int maxFrames = 128;
				void* frames[maxFrames];
				HANDLE process = GetCurrentProcess();
				SymInitialize(process, NULL, TRUE);

				WORD frameCount = CaptureStackBackTrace(0, maxFrames, frames, NULL);
				
				// Resolve symbols for the captured frames
				SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256);
				symbol->MaxNameLen = 255;
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

				ANV_LOG_INFO("=== STACK TRACE ===");
				for (int i = 0; i < frameCount; ++i)
				{
					if (SymFromAddr(process, (DWORD64)(frames[i]), 0, symbol))
					{
						ANV_LOG_INFO("\t0x%X   ...   %s ", symbol->Address, symbol->Name);
					}
					else
					{
						ANV_LOG_INFO("??? : %s", GetLastError());
					}
				}

				free(symbol);
#endif
			}
		};
	}
}