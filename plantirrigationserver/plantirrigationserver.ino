#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>]
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "AppParams.h"

#define AP_INDICATOR_LED_PIN 5 // D1
#define CLEAR_NETWORK_PIN 4 // D2

IPAddress ownIP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

ESP8266WebServer webServer(80);
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

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

void setNetwork() {
    String postBody = webServer.arg("plain");
    Serial.println(postBody);

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (!error) {
      JsonObject json = doc.as<JsonObject>();
      NetworkParams params;

      if (validateParams(json, params)) {
        AppParams.storeNetworkParams(params);

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

  AppParams.readNetworkParams(params);
  
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

void clearNetwork() {
  AppParams.clearNetworkParams();
  webServer.send(200, F("text/html"), "Network params deleted!");
}

// Define routing
void restServerRouting() {
    webServer.on("/", HTTP_GET, []() {
        webServer.send(200, F("text/html"),
            F("Plant irrigation server"));
    });
    webServer.on(F("/network"), HTTP_POST, setNetwork);
    webServer.on(F("/network"), HTTP_GET, getNetwork);
    webServer.on(F("/network"), HTTP_DELETE, clearNetwork);
}


void setupWiFi(NetworkParams& params) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(params.ssid, params.password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(params.ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());



  restServerRouting();
  webServer.begin();
  
  timeClient.begin();
}

void setupAP() {
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

void setup() {
  Serial.begin(115200);
  delay(500);

  AppParams.init();
  Serial.println();

  pinMode(AP_INDICATOR_LED_PIN, OUTPUT);
  pinMode(CLEAR_NETWORK_PIN, INPUT);

  delay(500);

  for (int i=0; i<100; i++) {
    delay(10);
    if (digitalRead(CLEAR_NETWORK_PIN) == HIGH) {
      clearNetwork();

      Serial.println("Network params cleared");
      break;
    }
  }
  
  digitalWrite(AP_INDICATOR_LED_PIN, HIGH);
  
  NetworkParams params;
  
  if (AppParams.readNetworkParams(params)) {
    Serial.println("WiFi mode");
    
    setupWiFi(params);
  } else {
    Serial.println("AP mode");
    
    setupAP();
  }
}

void loop()
{
  static bool printTime = true;
  
  webServer.handleClient();

  timeClient.update();

  if (printTime) {
    Serial.println(timeClient.getFormattedTime());
    printTime = false;
  }
}
