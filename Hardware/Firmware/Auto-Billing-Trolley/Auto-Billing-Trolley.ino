#include <Wire.h>
#include "src/LiquidCrystal_I2C/LiquidCrystal_I2C.h"  //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <FS.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

/*
Libraries Using
  EspSoftwareSerial - version 8.0.1 
  ESP8266WiFi       - version 1.0
  Hash              - version 1.0 
  WebSockets        - version 2.3.6
  ArduinoJson       - version 6.20.1 
  WiFiManager       - version 2.0.15-rc.1 
  ESP8266WebServer  - version 1.0 
  DNSServer         - version 1.1.1 
*/

// define the pins used for the software serial connection
#define RFID_Serial_TX 12
#define RFID_Serial_RX 13

// define the pins used for the remove item button
#define REMOVE_Item_Btn 14

//
#define DISPLAY_ADDR 0x3F

//
#define JSON_CONFIG_FILE "/config.json"
//
#define SSID "Auto Billing Trolley"
#define PASSWORD    "password"

// create a new instance of the EspSoftwareSerial library
EspSoftwareSerial::UART RFID_Serial;

//
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 16, 2);

//
WiFiManager wm;
//
WebSocketsClient webSocket;

// initialize variables used to store the card number
int count = 0;
char card_no[12];

// initialize variable to toggle between add and remove items mode
bool removeItem = true;

//
bool shouldSaveConfig = false;
char ws_server_val[21] = "192.168.x.x";
int ws_server_port_val = 1880;
char ws_server_path_val[41] = "/ws";

void setup() {
  // initialize the Serial Monitor
  Serial.begin(115200);

  Wire.begin();
  lcd.init();
  lcd.backlight();

  // set the remove item button pin as input with pull-up resistor enabled
  pinMode(REMOVE_Item_Btn, INPUT_PULLUP);

  pinMode(2, OUTPUT);

  // initialize the software serial connection for the RFID reader
  RFID_Serial.begin(9600, SWSERIAL_8N1, RFID_Serial_RX, RFID_Serial_TX, false);

  // check if the software serial connection was successfully initialized
  if (!RFID_Serial) {
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
    while (1) {
      delay(1000);
    }
  }

  bool forceConfig = false;  // Change to true when testing to force configuration every time we run

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  setupWiFiManager();

  // server address, port and URL
  webSocket.begin(ws_server_val, ws_server_port_val, ws_server_path_val);

  // event handler
  webSocket.onEvent(webSocketEvent);

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);


  Serial.print("WS server: ");
  Serial.println(ws_server_val);
  Serial.print("WS server port: ");
  Serial.println(ws_server_port_val);
  Serial.print("WS server path: ");
  Serial.println(ws_server_path_val);

  lcd.setCursor(0, 0);
  lcd.print("Testing...");
  delay(1000);
}

void loop() {
  webSocket.loop();

  // read the state of the remove item button
  bool removeItemBtnState = digitalRead(REMOVE_Item_Btn);

  // toggle between add and remove items mode when the remove item button is pressed
  if (!removeItemBtnState) {
    removeItem = !removeItem;
    delay(500);  // debounce delay
  }

  digitalWrite(2, removeItem);

  // check if data is available on the RFID reader's software serial connection
  if (RFID_Serial.available()) {
    count = 0;
    // read up to 12 characters from the software serial connection
    while (RFID_Serial.available() && count < 12) {
      card_no[count] = RFID_Serial.read();
      count++;
      delay(5);
    }
    // print the card number to the Serial Monitor with the appropriate message based on the current mode
    if (removeItem) {
      Serial.print("Add item: ");
    } else {
      Serial.print("Remove item: ");
    }
    Serial.println(card_no);
  }
}

void handleIncomingData(uint8_t *json) {
  DynamicJsonDocument doc(500);
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  String command = String(doc["command"]);
  Serial.println(command);

  if (command == "connected_ack") {
    // bool tool_timer_status    = doc["status"]["tool_timer"];
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected to url: %s\n", payload);
        // send message to server when Connected
        // webSocket.sendTXT(("{ \"command\": \"connected\", \"device\": \"" + String(device_name_val) + "\"}").c_str());
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      handleIncomingData(payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("[WSc] get pong\n");
      break;
  }
}


void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  // Create a JSON document
  StaticJsonDocument<512> json;
  json["ws_server"] = ws_server_val;
  json["ws_server_port"] = ws_server_port_val;
  json["ws_server_path"] = ws_server_path_val;

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}

// Load existing configuration file
bool loadConfigFile() {
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE)) {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          strcpy(ws_server_val, json["ws_server"]);
          ws_server_port_val = json["ws_server_port"].as<int>();
          strcpy(ws_server_path_val, json["ws_server_path"]);
          return true;
        } else {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  } else {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }
  return false;
}


// Callback notifying us of the need to save configuration
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Called when config mode launched
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}


void setupWiFiManager() {
  // wm.resetSettings(); // reset settings - wipe stored credentials for testing
  wm.setSaveConfigCallback(saveConfigCallback);  // Set config save notify callback
  wm.setAPCallback(configModeCallback);          // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setDebugOutput(false);                      //Disable debugging
  wm.setDarkMode(true);                          // dark theme

  WiFiManagerParameter ws_server("ws_server", "Websocket Server Address", ws_server_val, 20);
  wm.addParameter(&ws_server);

  char ws_server_port_val_a[16];
  WiFiManagerParameter ws_server_port("ws_server_port", "Websocket Server Port", itoa(ws_server_port_val, ws_server_port_val_a, 10), 5);
  wm.addParameter(&ws_server_port);

  WiFiManagerParameter ws_server_path("ws_server_path", "Websocket Server Path", ws_server_path_val, 40);
  wm.addParameter(&ws_server_path);

  String custom_js_string = "<script>document.getElementById('ws_server_port').type = 'number';</script>";
  WiFiManagerParameter custom_js(custom_js_string.c_str());
  wm.addParameter(&custom_js);

  bool res;
  String AP_NAME = SSID;
  res = wm.autoConnect(AP_NAME.c_str(), PASSWORD);  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  strncpy(ws_server_val, ws_server.getValue(), sizeof(ws_server_val));                 // Copy the string value
  ws_server_port_val = atoi(ws_server_port.getValue());                                //Convert the number value
  strncpy(ws_server_path_val, ws_server_path.getValue(), sizeof(ws_server_path_val));  // Copy the string value

  Serial.print("WS server: ");
  Serial.println(ws_server.getValue());
  Serial.print("WS server port: ");
  Serial.println(ws_server_port.getValue());
  Serial.print("WS server path: ");
  Serial.println(ws_server_path.getValue());
  // Save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}
