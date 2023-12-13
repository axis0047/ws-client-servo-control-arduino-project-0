#include <ESP8266WiFi.h>
//#include <WebSocketClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Servo.h>


#define USE_SERIAL Serial

#define INTERRUPT_BUTTON 2

#define SERVO0 15
#define SERVO1 4
#define SERVO2 12
#define SERVO3 14


//
//boolean SERVO0_status = false;
//boolean SERVO1_status = false;
//boolean SERVO2_status = false;
//boolean SERVO3_status = false;

StaticJsonDocument<200> doc;


long currentMillis = 0;

long period = 60000;


const char* ssid     = "SANJEEWA";
const char* password = "680850402";
char path[] = "/";
//IPAddress host(20,246,228,95);/
char host[] = "20.253.48.86";
char wsHost[] = "20.253.48.86";

const char* ssidAP = "NodeMCUPostTest";
const char* passwordAP = "12345678";

boolean websocketOn = false;

IPAddress local_ip(192,168,2,1);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

volatile boolean server_mode = false;

const String postForms = "<html>\
  <head>\
    <title>Access Point Credentials</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Enter WiFi Access Point Credentials</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      <lable>Wifi Ssid</lable>\
      <input type=\"text\" name=\"ssid\" value=\"\"><br>\
      <lable>Wifi Password</lable>\
      <input type=\"password\" name=\"pw\" value=\"\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

void handleRoot() {
  server.send(200, "text/html", postForms);
}

void handleForm() {
  if (server.method() != HTTP_POST) {
//    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
//    digitalWrite(led, 0);
  } else {
//    digitalWrite(led, 1);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) { 
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
      if(server.argName(i) == "ssid"){
//        credentialsArr[0] = server.arg(i);
          ssid = server.arg(i).c_str();
        }
       if(server.argName(i) == "pw"){
//        credentialsArr[1] = server.arg(i);
          password = server.arg(i).c_str();
        }
      }
    Serial.print(message);
    server.send(200, "text/plain", message);
//    digitalWrite(led, 0);
    WiFi.softAPdisconnect(true);
    server_mode = false;
    delay(100);
    Serial.println("HTTP Server Stopped");
  }
}

void handleNotFound() {
//  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
//  digitalWrite(led, 0);
}

ICACHE_RAM_ATTR void setServerMode() {
 if(server_mode == false){
    server_mode = true;
    Serial.println("server mode on");
  }else{
    server_mode = false;
    Serial.println("server mode on");
    if(WiFi.softAPIP() == local_ip){
          WiFi.softAPdisconnect(true);
          Serial.println("HTTP Server Stopped");
      } 
  }
}

int unit_id = 69420;
String initial_data = "{\"realyId\":\"2\"}";

WebSocketsClient webSocket;


WiFiClient client;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      webSocket.sendTXT(initial_data);
    }
      break;
    case WStype_TEXT:{
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      
      DeserializationError error = deserializeJson(doc, payload);

      serializeJsonPretty(doc, Serial);

      if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
      } else {
     
        for (JsonPair kv : doc.as<JsonObject>()) {
          const char* key = kv.key().c_str();
          bool value = kv.value().as<bool>();

          Serial.print("Key: ");
          Serial.printf("hello%shello",key);
          Serial.print(", Value: ");
          Serial.println(value);

          if(strcmp(key, "0") == 1){
            digitalWrite(SERVO0, value);
            Serial.println("Switched 0");
          }else if(strcmp(key, "1") == 1){
            digitalWrite(SERVO1, value);
            Serial.println("Switched 1");
          }else if(strcmp(key, "2") == 1){
            digitalWrite(SERVO2, value);
            Serial.println("Switched 2");
          }else{
            digitalWrite(SERVO3, value);
            Serial.println("Switched 3");
          }
        }
      }
      break;
    }
    case WStype_BIN:

      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
        case WStype_PING:
            // pong will be send automatically
            USE_SERIAL.printf("[WSc] get ping\n");
            break;
        case WStype_PONG:
            // answer to a ping we send
            USE_SERIAL.printf("[WSc] get pong\n");
            break;
    }
}

void setup() {
  
  Serial.begin(9600);
//  Serial.setDebugOutput(true);
  delay(10);

  for(int t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  pinMode(INTERRUPT_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_BUTTON), setServerMode, CHANGE);
  
  server.on("/", handleRoot);

  server.on("/postform/", handleForm);

  server.onNotFound(handleNotFound);

  pinMode(SERVO0, OUTPUT);
  pinMode(SERVO1, OUTPUT);
  pinMode(SERVO2, OUTPUT);
  pinMode(SERVO3, OUTPUT);

  digitalWrite(SERVO0, LOW);
  digitalWrite(SERVO1, LOW);
  digitalWrite(SERVO2, LOW);
  digitalWrite(SERVO3, LOW);

}

void loop() {

  if(server_mode){

    websocketOn = false;

    if(WiFi.softAPIP() != local_ip){
      
        WiFi.softAP(ssidAP, passwordAP);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        delay(100);
        server.begin();
        Serial.println("HTTP server started");
        
      }else{
        server.handleClient();
      }

//    Serial.println("Server Mode");
//    delay(5000);
//    server_mode = false;
    
    }else{

       //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){

      delay(10);

      if(websocketOn){
        
        webSocket.loop();
        
        }else{
          
        webSocket.begin(wsHost,4000, "/relayswitch");
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(5000);

        webSocket.enableHeartbeat(15000, 3000, 2);

        websocketOn = true;
        
        }      
    }
    else {
      websocketOn = false;
//      Serial.println("WiFi Disconnected");
      WiFi.begin(ssid, password);
      Serial.println("Connecting...");
      currentMillis = millis();
      while(millis() - currentMillis < period && WiFi.status() != WL_CONNECTED) { 
        delay(500);
        Serial.print(".");
      }
      if(WiFi.status() == WL_CONNECTED){
        Serial.println("");
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());
      }else{
        Serial.println("Retrying...");
      }
    }
    }
 
}