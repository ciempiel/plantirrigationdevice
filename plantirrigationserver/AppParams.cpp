#include "Arduino.h"
#include "EEPROM.h"
#include "AppParams.h"

#define NETWORK_PARAMS_EEPROM_ARDRESS 0x0000

void AppParamsClass::init() {
  EEPROM.begin(512);
  Serial.print("EEPROM initialized.");
}

bool AppParamsClass::storeNetworkParams(NetworkParams& params) {
  EEPROM.put(NETWORK_PARAMS_EEPROM_ARDRESS, params);
  EEPROM.commit();
  
  return true;
}

bool AppParamsClass::readNetworkParams(NetworkParams& params) {
  EEPROM.get(NETWORK_PARAMS_EEPROM_ARDRESS, params);
  
  // XXX it will be changed
  return params.ssid[0] != 0xff;
}

bool AppParamsClass::clearNetworkParams() {
  NetworkParams params;
  memset(&params, 0xff, sizeof(NetworkParams));
  storeNetworkParams(params);
  return true;
}

AppParamsClass AppParams;
