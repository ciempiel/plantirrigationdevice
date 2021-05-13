#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include "EEPROM.h"

#define MAX_SSID 32
#define MAX_PASSWORD 32

#define EEPROM_ARDRESS 0x0000

struct NetworkParams {
    char ssid[MAX_SSID + 1];
    char password[MAX_PASSWORD + 1];
    uint32_t ownIp;
    uint32_t gateway;
    uint32_t subnet;
};

IPAddress ownIP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

ESP8266WebServer webServer(80);

bool validateString(char* dest, JsonObject& json, const char* name, int maxLength) {
  if (json.containsKey(name)) {
    const char* value = json[name];
    if (strlen(value) < maxLength) {
      strcpy(dest, value);
      return true;
    }
  }
  return false;
}

bool validateIp(uint32_t* dest, JsonObject& json, const char* name) {
  IPAddress address;
  
  if (json.containsKey(name)) {
    const char* value = json[name];
    if (address.fromString(value)) {
      *dest = address;
      return true;
    }
  }
  return false;
}

bool validateParams(JsonObject& json, NetworkParams& params) {
  NetworkParams newParmas;
  bool errors;

  errors = !validateString(newParmas.ssid, json, "ssid", MAX_SSID);
  errors = !validateString(newParmas.password, json, "password", MAX_PASSWORD);
  errors = !validateIp(&newParmas.ownIp, json, "ownIp");
  errors = !validateIp(&newParmas.gateway, json, "gateway");
  errors = !validateIp(&newParmas.subnet, json, "subnet");

  if(!errors) {
    memcpy(&params, &newParmas, sizeof(NetworkParams));
  }
  return !errors;
}

void storeNetworkParams(NetworkParams& params) {
  EEPROM.put(EEPROM_ARDRESS, params);
}

void readNetworkParams(NetworkParams& params) {
  EEPROM.get(EEPROM_ARDRESS, params);
}

void setNetwork() {
    String postBody = webServer.arg("plain");
    Serial.println(postBody);

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (!error) {
      JsonObject json = doc.as<JsonObject>();
      NetworkParams params;

      if (validateParams(json, params)) {
        storeNetworkParams(params);

        Serial.print(F("Network params stored!"));
        webServer.send(201, F("text/html"), "Network params stored!");
      } else {
        Serial.print(F("Invalid params"));
        webServer.send(400, F("text/html"), "Invalid params!");
      }
    } else {
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
        String msg = error.c_str();
        webServer.send(400, F("text/html"), "Error in parsin json body! <br>" + msg);
    }
}

void getNetwork() {
  DynamicJsonDocument doc(512);
  NetworkParams params;
  IPAddress address;
  String buf;

  readNetworkParams(params);
  
  doc["ssid"] = params.ssid;
  doc["password"] = params.password;
  address = params.ownIp;
  doc["ownIp"] = address.toString();
  address = params.gateway;
  doc["gateway"] = address.toString();
  address = params.subnet;
  doc["subnet"] = address.toString();
  
  serializeJson(doc, buf);
  webServer.send(200, F("application/json"), buf);
  Serial.print(F("done."));
}

// Define routing
void restServerRouting() {
    webServer.on("/", HTTP_GET, []() {
        webServer.send(200, F("text/html"),
            F("Plant irrigation server"));
    });
    webServer.on(F("/network"), HTTP_POST, setNetwork);
    webServer.on(F("/network"), HTTP_GET, getNetwork);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  EEPROM.begin(512);
  Serial.print("EEPROM initialized.");

  Serial.print("Setting soft-AP ... ");
  WiFi.softAPConfig(ownIP, gateway, subnet);
  boolean result = WiFi.softAP("PlantServer_01", "12345678");
  if(result == true)
  {
    Serial.println("Ready");

    restServerRouting();

    webServer.begin();
  }
  else
  {
    Serial.println("Failed!");
  }
}

void loop()
{
  webServer.handleClient();
}
