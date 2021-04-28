#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>


IPAddress ownIP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

ESP8266WebServer webServer(80);

// Define routing
void restServerRouting() {
    webServer.on("/", HTTP_GET, []() {
        webServer.send(200, F("text/html"),
            F("Plant irrigation server"));
    });
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

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
