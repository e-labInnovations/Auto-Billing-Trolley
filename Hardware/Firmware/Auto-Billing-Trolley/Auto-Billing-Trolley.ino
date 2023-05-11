#include <Wire.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <FS.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

/*
Libraries using:
- EspSoftwareSerial - version 8.0.1 
- ESP8266WiFi - version 1.0
- Hash - version 1.0 
- WebSockets - version 2.3.6
- ArduinoJson - version 6.20.1 
- WiFiManager - version 2.0.15-rc.1 
- ESP8266WebServer - version 1.0 
- DNSServer - version 1.1.1 
*/

// Define the pins used for the software serial connection
#define RFID_Serial_TX 12
#define RFID_Serial_RX 13

// Define the pins used for the remove item button
#define REMOVE_Item_Btn 14

// Define the pins used for the Buzzer
#define BUZZER 16

// Define the I2C address of the LCD display
#define DISPLAY_ADDR 0x3F

// Define the path for the JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// Define the SSID and password for the WiFi network
#define AP_SSID "Auto Billing Trolley"
#define PASSWORD "password"

// Create a new instance of the EspSoftwareSerial library
EspSoftwareSerial::UART RFID_Serial;

// Initialize an instance of the u8g2 library for the oled display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Initialize an instance of the WiFiManager library for managing the WiFi connection
WiFiManager wm;

// Initialize an instance of the WebSocketsClient library for communicating with a WebSocket server
WebSocketsClient webSocket;

// Initialize variables used to store the card number
int count = 0;
char card_no[12];

// Initialize a variable to toggle between add and remove items mode
bool removeItem = false;

// Initialize a flag to determine whether the configuration settings have been modified and need to be saved
bool shouldSaveConfig = false;

// Indicates whether the device is currently connected to the server or not
bool serverConnection = false;

// These two character arrays will hold the string representations of the total items and total price
char items_str[10];
char total_str[10];

// Initialize variables to store the WebSocket server address, port, and path
char ws_server_val[21] = "192.168.x.x";  // Replace "x.x" with the actual IP address of the WebSocket server
int ws_server_port_val = 1880;
char ws_server_path_val[41] = "/ws";

void updateDisplay(int mode, const char* ssid = "") {
  // Initialize display and set font
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB14_tr);

    if (mode == 0) {
      // Print the label and value for total items
      u8g2.setCursor(0, 30);
      u8g2.print("Items: ");
      u8g2.setCursor(u8g2.getStrWidth("Items: "), 30);
      u8g2.print(items_str);

      // Print the label and value for total price
      u8g2.setCursor(0, 46);
      u8g2.print("Price: ");
      u8g2.setCursor(u8g2.getStrWidth("Price: "), 46);
      u8g2.print(total_str);
      u8g2.setCursor(u8g2.getStrWidth("Price: ") + u8g2.getStrWidth(total_str), 46);
      u8g2.print(".00");

      // Print the label for add/remove item
      u8g2.setFont(u8g2_font_t0_13b_tr);
      u8g2.setCursor(0, 60);
      u8g2.print(removeItem ? "Remove" : "Add");
    } else if (mode == 1) {
      // Print the label and value for WiFi connection status
      u8g2.setCursor(getCenterX("WiFi"), 28);
      u8g2.print("WiFi");
      u8g2.setCursor(getCenterX("Connected"), 44);
      u8g2.print("Connected");
      u8g2.setCursor(getCenterX(ssid), 60);
      u8g2.print(ssid);
    } else if (mode == 2) {
      // Print the label for server connection status
      u8g2.setCursor(getCenterX("Connected"), 30);
      u8g2.print("Connected");
      u8g2.setCursor(getCenterX("Server"), 60);
      u8g2.print("Server");
    } else if (mode == 3) {
      // Print the label for server connection status
      u8g2.setCursor(getCenterX("Server"), 30);
      u8g2.print("Server");
      u8g2.setCursor(getCenterX("Disconnected"), 60);
      u8g2.print("Disconnected");
    }
  } while (u8g2.nextPage());
}

void setup() {
  // Initialize the Serial Monitor
  Serial.begin(115200);

  // Initialize the display
  u8g2.begin();

  // Set the pin for the remove item button as input with pull-up resistor enabled
  pinMode(REMOVE_Item_Btn, INPUT_PULLUP);

  // Set the pin 2 BUZZER output
  pinMode(BUZZER, OUTPUT);

  // Display a project name on the LCD display for 1 second
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_tenfatguys_tr);
    u8g2.setCursor(14, 30);
    u8g2.print("Auto Billing");
    u8g2.setCursor(32, 46);
    u8g2.print("Trolley");
  } while (u8g2.nextPage());
  delay(1000);

  // Initialize the software serial connection for the RFID reader
  RFID_Serial.begin(9600, SWSERIAL_8N1, RFID_Serial_RX, RFID_Serial_TX, false);

  // Check if the software serial connection was successfully initialized
  if (!RFID_Serial) {
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
    // If software serial connection fails, print an error message and loop infinitely
    while (1) {
      delay(1000);
    }
  }

  // Set both character arrays to zero to initialize them to default values of 0
  memset(items_str, 0, sizeof(items_str));
  memset(total_str, 0, sizeof(total_str));

  // Set the initial value of forceConfig to false
  bool forceConfig = false;

  // Load the configuration from the SPIFFS file system
  bool spiffsSetup = loadConfigFile();

  // If the configuration file does not exist, force the configuration mode
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  // Set the ESP8266 WiFi mode to Station mode
  WiFi.mode(WIFI_STA);

  // Setup the WiFiManager library to handle WiFi configuration and connection
  setupWiFiManager();

  // Set the server address, port, and URL for the WebSocket connection
  webSocket.begin(ws_server_val, ws_server_port_val, ws_server_path_val);

  // Set the WebSocket event handler
  webSocket.onEvent(webSocketEvent);

  // Try to reconnect to the WebSocket server every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);

  // Start the WebSocket heartbeat to keep the connection alive
  // Ping the server every 15000ms and expect a response within 3000ms
  // If the response is not received 2 times, consider the connection as disconnected
  webSocket.enableHeartbeat(15000, 3000, 2);

  // Print the WebSocket server information to the Serial Monitor
  Serial.print("WS server: ");
  Serial.println(ws_server_val);
  Serial.print("WS server port: ");
  Serial.println(ws_server_port_val);
  Serial.print("WS server path: ");
  Serial.println(ws_server_path_val);
}


void loop() {
  // Call the webSocket library's loop() function to handle any incoming WebSocket messages
  webSocket.loop();

  // Read the state of the remove item button
  bool removeItemBtnState = digitalRead(REMOVE_Item_Btn);

  // Toggle between add and remove items mode when the remove item button is pressed
  if (!removeItemBtnState) {
    removeItem = !removeItem;
    //Update the display
    updateDisplay(0);
    delay(500);  // Debounce delay to prevent multiple presses
  }

  // Check if data is available on the RFID reader's software serial connection
  if (RFID_Serial.available()) {
    count = 0;
    // Read up to 12 characters from the software serial connection
    while (RFID_Serial.available() && count < 12) {
      card_no[count] = RFID_Serial.read();
      count++;
      delay(5);
    }
    digitalWrite(BUZZER, HIGH);

    // Send a WebSocket message with the RFID number, depending on the current mode (add or remove item)
    if (!removeItem) {
      Serial.print("Add item: ");
      webSocket.sendTXT(("{ \"command\": \"addItem\", \"rfid\":\"" + String(card_no) + "\"}").c_str());
    } else {
      Serial.print("Remove item: ");
      webSocket.sendTXT(("{ \"command\": \"removeItem\", \"rfid\":\"" + String(card_no) + "\"}").c_str());
      removeItem = false;
    }

    // Print the RFID number to the serial monitor
    Serial.println(card_no);

    delay(1000);
    digitalWrite(BUZZER, LOW);
  }
}

int getCenterX(const char* text) {
  return (128 - u8g2.getStrWidth(text)) / 2;
}

void handleIncomingData(uint8_t* json) {
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
  } else if (command == "cartUpdate") {
    //{"command":"cartUpdate","items":1,"total":26}
    // Retrieve the number of items and total price from JSON object
    int items = doc["items"];
    int total = doc["total"];

    // Print the number of items and total price to the serial monitor
    Serial.print("Items: ");
    Serial.print(items);
    Serial.print("  Total: ");
    Serial.println(total);

    // Convert the integer values to strings
    utoa(items, items_str, 10);
    utoa(total, total_str, 10);

    //Update the display
    updateDisplay(0);
  }
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:  // The WebSocket connection has been disconnected
      Serial.printf("[WSc] Disconnected!\n");
      // Show Disconnected message
      updateDisplay(3);
      break;
    case WStype_CONNECTED:  // A WebSocket connection has been established
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      // send message to server when Connected
      webSocket.sendTXT("{\"command\":\"connected\"}");  // Send a text message to the server after a connection has been established
      // Show connected message
      updateDisplay(2);
      break;
    case WStype_TEXT:  // A text message has been received from the server
      Serial.printf("[WSc] get text: %s\n", payload);
      handleIncomingData(payload);  // Call the handleIncomingData function to process the received data
      break;
    case WStype_BIN:  // A binary message has been received from the server
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);  // Print the binary data in hexadecimal format
      break;
    case WStype_PING:  // A ping message has been received from the server
      // pong will be send automatically
      Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:  // A pong message has been received from the server
      // answer to a ping we send
      Serial.printf("[WSc] get pong\n");
      break;
  }
}

void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  // Create a JSON document with a size of 512 bytes
  StaticJsonDocument<512> json;

  // Set the values of the JSON object
  json["ws_server"] = ws_server_val;            // WebSocket server address
  json["ws_server_port"] = ws_server_port_val;  // WebSocket server port
  json["ws_server_path"] = ws_server_path_val;  // WebSocket server path

  // Open the configuration file for writing
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  // Serialize the JSON data to write to the file
  serializeJsonPretty(json, Serial);           // Print the JSON data to the serial monitor for debugging
  if (serializeJson(json, configFile) == 0) {  // Write the JSON data to the file
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  configFile.close();
}

// Load existing configuration file
bool loadConfigFile() {

  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Mount the SPIFFS file system
  Serial.println("Mounting File System...");
  if (SPIFFS.begin()) {  // If mounted successfully

    Serial.println("mounted file system");

    // Check if the configuration file exists
    if (SPIFFS.exists(JSON_CONFIG_FILE)) {

      // The file exists, so read and load it
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("Opened configuration file");

        // Parse the JSON data from the configuration file
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);

        // If the JSON data was parsed successfully, load the values
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

  // Return false if there was an error
  return false;
}

// Callback notifying us of the need to save configuration
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Called when config mode launched
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

// Initialize WiFiManager
void setupWiFiManager() {
  // wm.resetSettings(); // reset settings - wipe stored credentials for testing
  wm.setSaveConfigCallback(saveConfigCallback);  // Set config save notify callback
  wm.setAPCallback(configModeCallback);          // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setDebugOutput(false);                      //Disable debugging
  wm.setDarkMode(true);                          // Enable dark theme

  // Create WiFiManager parameters
  WiFiManagerParameter ws_server("ws_server", "Websocket Server Address", ws_server_val, 20);
  wm.addParameter(&ws_server);

  char ws_server_port_val_a[16];
  WiFiManagerParameter ws_server_port("ws_server_port", "Websocket Server Port", itoa(ws_server_port_val, ws_server_port_val_a, 10), 5);
  wm.addParameter(&ws_server_port);

  WiFiManagerParameter ws_server_path("ws_server_path", "Websocket Server Path", ws_server_path_val, 40);
  wm.addParameter(&ws_server_path);

  // Add custom JavaScript to the WiFiManager page
  String custom_js_string = "<script>document.getElementById('ws_server_port').type = 'number';</script>";
  WiFiManagerParameter custom_js(custom_js_string.c_str());
  wm.addParameter(&custom_js);

  // Connect to WiFi
  bool res;
  String AP_NAME = AP_SSID;
  res = wm.autoConnect(AP_NAME.c_str(), PASSWORD);  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  updateDisplay(1, WiFi.SSID().c_str());

  // Print WiFi information
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Update parameters with values from WiFiManager
  strncpy(ws_server_val, ws_server.getValue(), sizeof(ws_server_val));                 // Copy the string value
  ws_server_port_val = atoi(ws_server_port.getValue());                                // Convert the number value
  strncpy(ws_server_path_val, ws_server_path.getValue(), sizeof(ws_server_path_val));  // Copy the string value

  Serial.print("WS server: ");
  Serial.println(ws_server.getValue());
  Serial.print("WS server port: ");
  Serial.println(ws_server_port.getValue());
  Serial.print("WS server path: ");
  Serial.println(ws_server_path.getValue());

  // Save the custom parameters to file system if they have changed
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}