#include "P099_data_struct.h"

#ifdef USES_P099
#include "../../ESPEasyNetwork.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/Scheduler.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/SystemVariables.h"

#include "../../ESPEasy_fdwdecl.h"

#include <XPT2046_Touchscreen.h>

P099_data_struct::P099_data_struct() : touchscreen(nullptr) {}

P099_data_struct::~P099_data_struct() {
  reset();
}

/**
 * Proper reset and cleanup.
 */
void P099_data_struct::reset() {
  if (touchscreen != nullptr) {
    delete touchscreen;
    touchscreen = nullptr;
  }
}

/**
 * Initialize data and set up the touchscreen.
 */
bool P099_data_struct::init(taskIndex_t taskIndex,
                 uint8_t  _cs,
                 uint8_t  _rotation,
                 uint8_t  _z_treshold,
                 bool     _send_xy,
                 bool     _send_z,
                 bool     _useCalibration,
                 uint16_t _ts_x_res,
                 uint16_t _ts_y_res) {
  reset();

  address_ts_cs  = _cs;
  z_treshold     = _z_treshold;
  rotation       = _rotation;
  send_xy        = _send_xy;
  send_z         = _send_z;
  useCalibration = _useCalibration;
  ts_x_res       = _ts_x_res;
  ts_y_res       = _ts_y_res;

  touchscreen = new (std::nothrow) XPT2046_Touchscreen(address_ts_cs);
  if (touchscreen != nullptr) {
    touchscreen->setRotation(rotation);
    touchscreen->begin();
    loadTouchObjects(taskIndex);
  } else {
    return false;
  }
  return isInitialized();
}

/**
 * Properly initialized? then true
 */
bool P099_data_struct::isInitialized() const {
  return touchscreen != nullptr;
}

/**
 * Load the touch objects from the settings, and initialize then properly where needed.
 */
void P099_data_struct::loadTouchObjects(taskIndex_t taskIndex) {
#ifdef PLUGIN_099_DEBUG
  String log = F("loadTouchObjects size Cal: ");
  log += String(sizeof(Calibration));
  log += F(" T.obj: ");
  log += String(sizeof(TouchObjects));
  addLog(LOG_LEVEL_INFO, log);
#endif // PLUGIN_099_DEBUG
  LoadCustomTaskSettings(taskIndex, (uint8_t *)&(Calibration), sizeof(Calibration) + sizeof(TouchObjects));

  for (int i = 0; i < P099_MaxObjectCount; i++) {
    TouchObjects[i].objectname[P099_MaxObjectNameLength - 1] = 0; // Terminate in case of uninitialized data
    SurfaceAreas[i] = 0;
  }
}

/**
 * Check if the screen is touched.
 */
bool P099_data_struct::touched() {
  if (isInitialized()) {
    return touchscreen->touched();
  } 
  return false;
}

/**
 * Read the raw data if the touchscreen is initialized.
 */
void P099_data_struct::readData(uint16_t *x, uint16_t *y, uint8_t *z) {
  if (isInitialized()) {
    touchscreen->readData(x, y, z);
  }
}

/**
 * Only set rotation if the touchscreen is initialized.
 */
void P099_data_struct::setRotation(uint8_t n) {
  if (isInitialized()) {
    touchscreen->setRotation(n);
  }
}

/**
 * Determine if calibration is enabled and usable.
 */
bool P099_data_struct::isCalibrationActive() {
  return    useCalibration
         && Calibration.top_left.x > 0
         && Calibration.top_left.y > 0
         && Calibration.bottom_right.x > 0
         && Calibration.bottom_right.y > 0; // Enabled and all values != 0 => Active
}

/**
 * Check within the list of defined objects if we touched one of them.
 * The smallest matching surface is selected if multiple objects overlap.
 * Returns state, and sets selectedObjectName to the best matching object
 */
bool P099_data_struct::isValidAndTouchedTouchObject(uint16_t x, uint16_t y, String &selectedObjectName, uint8_t checkObjectCount) {
  uint32_t lastObjectArea = 0;
  bool     selected = false;
  for (uint8_t objectNr = 0; objectNr < checkObjectCount; objectNr++) {
    String objectName = String(TouchObjects[objectNr].objectname);
    if ( objectName.length() > 0
      && objectName.substring(0,1 ) != F("_")         // Ignore if name starts with an underscore
      && TouchObjects[objectNr].bottom_right.x > 0
      && TouchObjects[objectNr].bottom_right.y > 0) { // Not initial could be valid
      if (SurfaceAreas[objectNr] == 0) {
        SurfaceAreas[objectNr] = (TouchObjects[objectNr].bottom_right.x - TouchObjects[objectNr].top_left.x) * (TouchObjects[objectNr].bottom_right.y - TouchObjects[objectNr].top_left.y);
      }
      if ( TouchObjects[objectNr].top_left.x <= x
        && TouchObjects[objectNr].top_left.y <= y
        && TouchObjects[objectNr].bottom_right.x >= x
        && TouchObjects[objectNr].bottom_right.y >= y
        && (lastObjectArea == 0 
          || SurfaceAreas[objectNr] < lastObjectArea)) { // Select smallest area that fits the coordinates
        selectedObjectName = objectName;
        lastObjectArea = SurfaceAreas[objectNr];
        selected = true;
      }
#ifdef PLUGIN_099_DEBUG
      String log = F("P099 Check touched: obj: ");
      log += objectName;
      log += ',';
      log += TouchObjects[objectNr].top_left.x;
      log += ',';
      log += TouchObjects[objectNr].top_left.y;
      log += ',';
      log += TouchObjects[objectNr].bottom_right.x;
      log += ',';
      log += TouchObjects[objectNr].bottom_right.y;
      log += F(" surface:");
      log += SurfaceAreas[objectNr];
      log += F(" x,y:");
      log += x;
      log += ',';
      log += y;
      log += F(" sel:");
      log += selectedObjectName;
      addLog(LOG_LEVEL_INFO, log);
#endif // PLUGIN_099_DEBUG
    }
  }
  return selected;
}

/**
 * scale the provided raw coordinates to screen-resolution coordinates if calibration is enabled/configured
 */
void P099_data_struct::scaleRawToCalibrated(uint16_t &x, uint16_t &y) {
  if (isCalibrationActive()) {
    uint16_t _x = x - Calibration.top_left.x;
    if (_x <= 0) {
      x = 0;
    } else {
      float x_fact = (Calibration.bottom_right.x - Calibration.top_left.x) / ts_x_res;
      x = int((_x * 1.0f) / x_fact);
    }
    uint16_t _y = y - Calibration.top_left.y;
    if (_y <= 0) {
      y = 0;
    } else {
      float y_fact = (Calibration.bottom_right.y - Calibration.top_left.y) / ts_y_res;
      y = int((_y * 1.0f) / y_fact);
    }
  }
}

#endif  // ifdef USES_P099