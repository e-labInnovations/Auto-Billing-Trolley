#include <Wire.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>

// define the pins used for the software serial connection
#define RFID_Serial_TX 12
#define RFID_Serial_RX 13

// define the pins used for the remove item button
#define REMOVE_Item_Btn 14

// create a new instance of the EspSoftwareSerial library
EspSoftwareSerial::UART RFID_Serial;

//
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// initialize variables used to store the card number
int count = 0;
char card_no[12];

// initialize variable to toggle between add and remove items mode
bool removeItem = true;

void setup() {
  // initialize the Serial Monitor
  Serial.begin(115200);

  //
  u8g2.begin();

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

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_bubble_tr);  // choose a suitable font
    u8g2.setCursor(0, 15);
    u8g2.print("e-lab");
    u8g2.setFont(u8g2_font_ncenB12_te);  // choose a suitable font
    u8g2.setCursor(0, 30);
    u8g2.print("innovations");
  } while (u8g2.nextPage());
  delay(1000);
}

void loop() {
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
