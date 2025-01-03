////////////////////////////////////////////////////////////////
//                                                            //
// Macros.h is used mainly for Compiler/OS and hardware       //
// specific detection.                                        //
//                                                            //
////////////////////////////////////////////////////////////////

#pragma once

/// OS detection
# ifdef _WIN32
	# ifdef _WIN64
		# define PLATFORM_WIN64 1 // Windows 64 bit 
	#else
		# error "32bit applications are not supported!"
		# define PLATFORM_WIN64 // Windows 32 bit
	# endif
# endif


// TODO (Alba): Impl Apple platform detection. (Follow a similar format of windows)
