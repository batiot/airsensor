
/*
  **********************************************************************
         Connect to wifi and push airquality to firebase
  **********************************************************************
*/
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "paulvha_SCD30.h"

//secret store in config.h
const char *ssid = WIFI_SSID;
const char *password =  WIFI_PASSWORD;
const char *firebaseEmail = FIREBASE_EMAIL;
const char *firebasePassword = FIREBASE_PASSWORD;


const String gIdentityBaseUrl( "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" );
const String gIdentityUrlWithApiKey = gIdentityBaseUrl +  FIREBASE_API_KEY;
const String httpsScheme("https://");
const String firebaseAironeBaseUrl(".firebaseio.com/airone.json?auth=");
const String firebaseAironeUrl = httpsScheme +FIREBASE_PROJECT + firebaseAironeBaseUrl;


int co2;
int temp, hum;
float temperatureOffset = 0;

//////////////////////////////////////////////////////////////////////////
// set SCD30 driver debug level (only NEEDED case of errors)            //
// Requires serial monitor (remove DTR-jumper before starting monitor)  //
// 0 : no messages                                                      //
// 1 : request sending and receiving                                    //
// 2 : request sending and receiving + show protocol errors             //
//////////////////////////////////////////////////////////////////////////
#define scd_debug 0
//////////////////////////////////////////////////////////////////////////
//                SELECT THE WIRE INTERFACE                             //
//////////////////////////////////////////////////////////////////////////
#define SCD30WIRE Wire

ESP8266WiFiMulti WiFiMulti;
SCD30 airSensor;

void setup() {
  SCD30WIRE.begin();

  Serial.begin(115200);
  Serial.println("SCD30 Example 1");

  airSensor.setDebug(scd_debug);
  //This will cause readings to occur every two seconds
  if (! airSensor.begin(SCD30WIRE))
  {
    Serial.println(F("The SCD30 did not respond. Please check wiring."));
    while (1);
  }

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }


  //airSensor.setAutoSelfCalibration(0);          // stop ASC as that is set automatically during airSensor.begin()
    
  if (!airSensor.setTemperatureOffset(temperatureOffset)) {
    Serial.println(F("Could not set temperature offset"));
  }
  
  airSensor.setAmbientPressure(1018); //Current ambient pressure in mBar: 700 to 1200

  // Change number of seconds between measurements: 2 to 1800 (30 minutes)
  // setting to 4 will cause 3 dots before reading, due to call delay(1000)
  airSensor.setMeasurementInterval(60);


  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    //client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();
    HTTPClient https;
    https.useHTTP10(true);

    if (airSensor.dataAvailable())
    {
      co2 = airSensor.getCO2();
      temp = round(airSensor.getTemperature());
      hum = round(airSensor.getHumidity());

      StaticJsonDocument<200> bodyDoc;
      // StaticJsonObject allocates memory on the stack, it can be
      // replaced by DynamicJsonDocument which allocates in the heap.
      //
      // DynamicJsonDocument  doc(200);
      // Add values in the document
      //
      JsonObject timestamp  = bodyDoc.createNestedObject("timestamp");
      timestamp[".sv"] = "timestamp";
      bodyDoc["co2"] = co2;
      bodyDoc["temp"] = temp;
      bodyDoc["hum"] = hum;
      String body;
      serializeJson(bodyDoc, body);
      Serial.println(body);

      //https.begin(*client, "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key="+apiKey); //HTTP
      https.begin(*client, gIdentityUrlWithApiKey); //HTTP
      https.addHeader("Content-Type", "application/json");
      StaticJsonDocument<200> authBodyDoc;
      authBodyDoc["email"] = firebaseEmail;
      authBodyDoc["password"] = firebasePassword;
      authBodyDoc["returnSecureToken"] = "true";
      String authBody;
      serializeJson(authBodyDoc, authBody);
      int httpCode = https.POST(authBody);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST signInWithPassword... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK && co2!=0) {
          DynamicJsonDocument doc(2048);
          deserializeJson(doc, https.getStream());
          https.begin(*client, firebaseAironeUrl + doc["idToken"].as<String>()); //HTTP
          https.addHeader("Content-Type", "application/json");

          int httpCode = https.POST(body);
          if (httpCode > 0) { // httpCode will be negative on error
            Serial.printf("[HTTP] POST airone... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK) {
              String payload = https.getString();
              //Serial.println(payload);
            }
          } else {
            Serial.printf("[HTTP] POST airone... failed, code:%s error: %s\n", httpCode, https.errorToString(httpCode).c_str());
          }
        }

      } else {
        Serial.printf("[HTTP] POST signInWithPassword... failed, code:%s error: %s\n", httpCode, https.errorToString(httpCode).c_str());
      }

      https.end();
    }
  }
}
