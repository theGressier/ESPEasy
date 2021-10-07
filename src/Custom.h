#ifndef ESPEASY_CUSTOM_H
#define ESPEASY_CUSTOM_H

#include <Arduino.h>

#ifdef BUILD_GIT
# undef BUILD_GIT
#endif // ifdef BUILD_GIT
#define BUILD_GIT           "[PG] "  __DATE__ " "  __TIME__

#ifndef USES_P245
  #define USES_P245  // Hoben
#endif

#define PLUGIN_BUILD_NORMAL
#define BUILD_NO_DEBUG

//#undef	DEFAULT_NAME
//#define DEFAULT_NAME		"MyEspEasyDevice"			// Enter your device friendly name
//
//#undef	DEFAULT_SSID
//#define DEFAULT_SSID		"MyHomeSSID"			   // Enter your network SSID
//
//#undef	DEFAULT_KEY
//#define DEFAULT_KEY			"MySuperSecretPassword"		// Enter your network WPA key
//
//#undef	DEFAULT_AP_KEY
//#define DEFAULT_AP_KEY		"MyOwnConfigPassword"		// Enter network WPA key for AP (config) mode

#endif