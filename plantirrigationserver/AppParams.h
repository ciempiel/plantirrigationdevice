#ifndef APP_PARAMS_h
#define APP_PARAMS_h

#include <stdint.h>

#define MAX_SSID 32
#define MAX_PASSWORD 32

struct NetworkParams {
  char ssid[MAX_SSID + 1];
  char password[MAX_PASSWORD + 1];
  uint32_t ownIp;
  uint32_t gateway;
  uint32_t subnet;
};

class AppParamsClass {
  
public:
  void init();
  bool storeNetworkParams(NetworkParams& params);
  bool readNetworkParams(NetworkParams& params);
  bool clearNetworkParams();
  
};

extern AppParamsClass AppParams;

#endif
