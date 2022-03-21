#pragma once

#define MAJOR_VERSION_STR "1"
#define MAJOR_VERSION_INT 1

#define SUB_VERSION_STR "0"
#define SUB_VERSION_INT 0

#define RELEASE_NUMBER_STR "0"
#define RELEASE_NUMBER_INT 0

#define BUILD_NUMBER_STR "0"
#define BUILD_NUMBER_INT 0

// Version with build number (example "1.0.3.342")
#define FULL_VERSION_STR MAJOR_VERSION_STR "." SUB_VERSION_STR "." RELEASE_NUMBER_STR "." BUILD_NUMBER_STR

// Version without build number (example "1.0.3")
#define VERSION_STR MAJOR_VERSION_STR "." SUB_VERSION_STR "." RELEASE_NUMBER_STR

#define stringOriginalFilename	"paukn.vst3"
#if SMTG_PLATFORM_64
#define stringFileDescription	"paukn VST3 (64Bit)"
#else
#define stringFileDescription	"paukn VST3-SDK"
#endif
#define stringCompanyWeb		""
#define stringCompanyEmail		""
#define stringCompanyName		"Auracle"
#define stringLegalCopyright	""
#define stringLegalTrademarks	""
