////////////////////////////////////////////////////////////////
//                                                            //
// Macros.h is used mainly for Compiler/OS and hardware       //
// specific detection.                                        //
//                                                            //
////////////////////////////////////////////////////////////////

#pragma once

#define ANV_ENGINE_VERSION "ver. dev2026.0.1"

/// OS detection
# ifdef _WIN32
	# ifdef _WIN64
		# define PLATFORM_WIN64 1 // Windows 64 bit 
	#else
		# error "32bit applications are not supported!"
		# define PLATFORM_WIN64 // Windows 32 bit
	# endif
# endif


# ifdef __APPLE__
	# define PLATFORM_APPLE 1 // Apple platform (MacOS, iOS, etc.)
	# ifndef PLATFORM_MACOS
		# define PLATFORM_MACOS 1 // MacOS
	#endif
# endif

#if defined(PLATFORM_MACOS) && defined(PLATFORM_APPLE_VK)

    #define ANV_API_VER "Vulkan 1.4"

#elif defined(PLATFORM_MACOS)

    #define ANV_API_VER "MacOS Metal"

#elif defined(PLATFORM_WIN64)

    #define ANV_API_VER "Vulkan 1.4"

#else

    #error "Unsupported platform or graphics API configuration"

#endif


