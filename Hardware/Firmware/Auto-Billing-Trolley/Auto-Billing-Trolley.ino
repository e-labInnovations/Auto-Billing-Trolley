#include <SoftwareSerial.h>

// define the pins used for the software serial connection
#define RFID_Serial_TX 12
#define RFID_Serial_RX 13

// create a new instance of the EspSoftwareSerial library
EspSoftwareSerial::UART RFID_Serial;

// initialize variables used to store the card number
int count = 0;
char card_no[12];

void setup() {
  // initialize the Serial Monitor
  Serial.begin(115200);

  // initialize the software serial connection for the RFID reader
  RFID_Serial.begin(9600, SWSERIAL_8N1, RFID_Serial_RX, RFID_Serial_TX, false);

  // check if the software serial connection was successfully initialized
  if (!RFID_Serial) {
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
    while (1) {
      delay(1000);
    }
  }
}

void loop() {
  // check if data is available on the RFID reader's software serial connection
  if (RFID_Serial.available()) {
    count = 0;
    // read up to 12 characters from the software serial connection
    while (RFID_Serial.available() && count < 12) {
      card_no[count] = RFID_Serial.read();
      count++;
      delay(5);
    }
    // print the card number to the Serial Monitor
    Serial.println(card_no);
  }
}
