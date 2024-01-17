// Firmware for design course entitled "Automatic Parking Logging Notification using ATmega328P and GSM Technology".
// Mapua University
// Author: Gene Lorenzo Bacalla


#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

char name1[17], name2[17], plate[8], user_mobile[12], office_mobile[12], park[4];

// SIPO CONFIGURATION
int pinDat = 4;
int pinClk = 7;
int pinRstSIPO = 8;

// TRANSISTOR BASE PINS
int pinSs1 = A0;
int pinSs2 = A1;
int pinSs3 = A2;
int pinSs4 = A3;

// RGB LED
int pinR = 6;
int pinG = 5;

// RFID CONIFIGURATION
int pinRstRFID = A4;
int pinSda = 10;

String _buffer;

// SPEAKER CONFIGURATION
int pinSPK = 9;

// CONTROL BUTTON CONFIGURATIOn
int pinBTN = A5;

// TIMING CONFIGURATION
unsigned long start_t;
int boot_timeout = 0;

bool deviceMode = true;
bool updateMode = false;
bool updateSuccess = false;
bool boot_ = true;
bool timing = true;
//bool load_EEPROM = true;


int beep_long = 800;
int beep_short = 200;

char led_color = 'g';

SoftwareSerial gsm(3, 2);

MFRC522 mfrc522(pinSda, pinRstRFID);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;


void setup() {

  SPI.begin();
  gsm.begin(9600);

  // newly added settings
  _buffer.reserve(50);

  pinMode(pinRstRFID, OUTPUT);
  pinMode(pinBTN, INPUT);

  delay(1000);
  mfrc522.PCD_Init();

  // RGB LED SETTINGS
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  digitalWrite(pinR, LOW);
  digitalWrite(pinG, LOW);

  // SPEAKER SETTINGS
  pinMode(pinSPK, OUTPUT);

  // SHIFT REGISTER SETTINGS
  pinMode(pinDat, OUTPUT);
  pinMode(pinClk, OUTPUT);
  pinMode(pinRstSIPO, OUTPUT);
  digitalWrite(pinDat, LOW);
  digitalWrite(pinClk, LOW);

  rstSIPO();

  // TRANSISTOR BASE VOLTAGE SETTINGS
  pinMode(pinSs1, OUTPUT);
  pinMode(pinSs2, OUTPUT);
  pinMode(pinSs3, OUTPUT);
  pinMode(pinSs4, OUTPUT);

  digitalWrite(pinSs1, LOW);
  digitalWrite(pinSs2, LOW);
  digitalWrite(pinSs3, LOW);
  digitalWrite(pinSs4, LOW);


  // If the office number is not loaded from the EEPROM properly (null characters, interrupted process etc.),
  // then the program will set the updateMode to true and updateSuccess to false, this flags are used in the
  // main program loop.
  while (true) {
    if (!load_EEPROM()) {
      led_color = 'r';
      LEDS(led_color);

      error(true, beep_short);
      delay(50);
      error(true, beep_short);
      delay(50);
      error(true, beep_short);
      delay(50);

      updateEEPROM();
    } else {
      break;
    }
  }


  // tone(pinSPK, 2500);
  // delay(80);
  // noTone(pinSPK);
  printMessage(1,false);
  delay(1500);

  while (!boot_sequence() && boot_) {
    if (boot_timeout == 9) {
      error(true, beep_long);
      delay(50);
      error(true, beep_long);
      delay(50);
      error(true, beep_long);
      delay(50);

      boot_ = false;
    }
  }


  // while (boot_sequence()) {}
  LEDS(led_color);
}

void rstSIPO() {
  digitalWrite(pinRstSIPO, LOW);
  digitalWrite(pinRstSIPO, HIGH);
  digitalWrite(pinRstSIPO, LOW);
}

int isoPlate() {
  int number = 0;

  for (int i = 3; i < sizeof(plate); i++) {
    if (isDigit(plate[i])) {
      int _temp = plate[i] - '0';
      number = number * 10 + _temp;
    }
  }
  return number;
}

void LEDS(char ch) {

  if (ch == 'r') {
    digitalWrite(pinG, LOW);
    digitalWrite(pinR, HIGH);
  } else if (ch == 'g') {
    digitalWrite(pinR, LOW);
    digitalWrite(pinG, HIGH);
  } else if (ch = 'y') {
    digitalWrite(pinR, HIGH);
    digitalWrite(pinG, HIGH);
  } else {
    digitalWrite(pinR, LOW);
    digitalWrite(pinG, LOW);
  }
}

void success(bool auto_revert, int dur) {
  digitalWrite(pinR, LOW);
  digitalWrite(pinG, HIGH);
  tone(pinSPK, 2500);
  delay(dur);
  noTone(pinSPK);
  digitalWrite(pinG, LOW);

  if (auto_revert) {
    LEDS(led_color);
  }
}

void error(bool auto_revert, int dur) {
  digitalWrite(pinG, LOW);
  digitalWrite(pinR, HIGH);
  tone(pinSPK, 2500);
  delay(dur);
  noTone(pinSPK);
  digitalWrite(pinR, LOW);

  if (auto_revert) {
    LEDS(led_color);
  }
}

bool updateEEPROM() {
  if (gsm.available()) {
    if (getSMS_OfficeNumber(gsm.readString())) {
      for (int i = 0; i < 11; i++) {
        EEPROM.write(i, (office_mobile[i] - '0'));
        if (i == 10) {
          return true;
        }
      }
    }
  }
  return false;
}

void updatePark() {

  String _temp = String(park);

  if (_temp == "IN" || _temp == "In" || _temp == "in") {

    park[0] = 'O';
    park[1] = 'U';
    park[2] = 'T';
    park[3] = '\0';
  } else if (_temp == "OUT" || _temp == "Out" || _temp == "out") {
    park[0] = 'I';
    park[1] = 'N';
    park[2] = '\0';
  }
}

bool plateDigitCounter() {
  if (strlen(plate) == 6) {
    return false;
  } else {
    return true;
  }
}


void loop() {

  if (digitalRead(pinBTN)) {

    if (timing) {
      start_t = millis();
      timing = false;
    }

    deviceMode = !deviceMode;

    if (deviceMode) {
      led_color = 'g';
    } else {
      led_color = 'y';
    }

    if ((millis() - start_t) >= 5000) {
      led_color = 'r';
      updateMode = true;
    }

    LEDS(led_color);
    delay(850);
  }

  // This if-statement monitors whether the button is still active (long pressed).
  // Unpressing the button will revert the timing to true, indicating that
  // the user does not intend to long press the button.
  if (!(digitalRead(pinBTN))) {
    timing = true;
  }


  // The updateMode is enclosed in an infinite while-loop to block all processes.
  // Note that this while-loop has no exit condition!
  // This process can only be stopped by powering-off the device.

  while (updateMode) {
    if (updateEEPROM()) {
      success(true, beep_short);
      printMessage(1,false);
    }
  }

  if (deviceMode) {
    if (readRFID()) {
      if (readDataFromBlock()) {
        updatePark();

        if (writeDataToBlock(2)) {
          success(true, beep_short);

          int _plate = isoPlate();
          int d0 = _plate % 10;
          int d1 = (_plate / 10) % 10;
          int d2 = (_plate / 100) % 10;
          int d3 = (_plate / 1000) % 10;


          printMessage(2,false);
          printSeg(d0, d1, d2, d3, plateDigitCounter());

          // if (park[0] == 'O') {
          //   printMessage(3);
          // } else {
          //   printMessage(4);
          // }

          sendSMS(4, true);
          delay(50);

          flushChar();
        } else {
          error(true, beep_short);
          delay(50);
          error(true, beep_short);
          delay(500);
        }
      }
    }
  } else {

    if (gsm.available()) {






      if (getSMS_Registration(gsm.readString())) {


        success(true, beep_short);
        delay(30);
        sendSMS(1, false);

        bool wait_yes = true;
        unsigned long t = millis();
        while (((millis() - t) < 30000) && (wait_yes)) {

          printMessage(7,false);

          if (gsm.available()) {
            if (checkSMS(1, gsm.readString())) {

              success(true, beep_short);
              delay(30);
              //sendSMS(2, false);

              wait_yes = false;
              bool wait_rfid = true;
              t = millis();

              while (((millis() - t) < 20000) && (wait_rfid)) {

                printMessage(8,false);

                if (readRFID()) {
                  if (writeDataToBlock(1)) {

                    success(true, beep_short);
                    delay(30);
                    printMessage(5,false);
                    delay(30);
                    sendSMS(3, true);

                    
                    flushChar();
                    wait_rfid = false;


                  } else {
                    error(true, beep_short);
                    delay(50);
                    error(true, beep_short);
                    delay(500);
                  }
                }
              }
              if (wait_rfid) {  
                sendSMS(5, false);
                delay(50);
                flushChar();
                error(true, beep_long);
              }
            } 
          }
        }
        if (wait_yes) {
          sendSMS(5, false);
          delay(50);
          flushChar();
          error(true, beep_long);
        }
      } 
    } else {
        printMessage(6,true);
    }
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}



void printMessage(int mode,bool sms_avail) {

  uint8_t message[20];

  // PARKING MODE DISPLAY
  //uint8_t please[20] = {0,0,0,103,14,79,119,91,79,0,14,126,95,0,48,118,0,0,0,0}; //  Character Count: 20
  uint8_t hello[12] = { 0, 0, 0, 55, 79, 14, 14, 126, 0, 0, 0, 0 };  // Character Count: 12



  //uint8_t info[11] = {0,0,0,48,118,71,126,0,0,0,0}; // Character Count: 11
  //uint8_t place[16] = {0,0,0,103,14,119,78,55,0,71,126,127,0,0,0,0}; // Character Count: 16

  // REGISTRATION MODE DISPLAY
  uint8_t info[4] = { 48, 118, 71, 126 };
  uint8_t conf[4] = { 78, 126, 118, 71 };
  uint8_t fob[4] = { 0, 71, 126, 127 };


  uint8_t log_out[14] = { 0, 0, 0, 14, 126, 95, 0, 126, 62, 15, 0, 0, 0, 0 };         // Character Count: 14
  uint8_t log_in[13] = { 0, 0, 0, 14, 126, 95, 0, 48, 118, 0, 0, 0, 0 };              //Character Count: 13
  uint8_t success_message[14] = { 0, 0, 0, 91, 62, 78, 78, 79, 91, 91, 0, 0, 0, 0 };  // Character Count: 14
  uint8_t office_message[18] = { 0 };                                                 // Character Count: 18

  uint8_t arrSize = 0;
  uint16_t speed = 250;


  switch (mode) {

    case 1:
      // DISPLAY THE OFFICE NUMBER IN THE EPROM
      for (int i = 3; i < 14; i++) {
        office_message[i] = decoderBCD(office_mobile[i - 3] - '0');
      }
      for (uint8_t u = 0; u < 18; u++) {
        message[u] = office_message[u];
      }
      arrSize = 18;
      break;

    case 2:
      // HELLO
      for (uint8_t i = 0; i < 12; i++) {
        message[i] = hello[i];
      }
      arrSize = 12;
      break;

    case 3:
      // LOG-OUT
      for (uint8_t i = 0; i < 14; i++) {
        message[i] = log_out[i];
      }
      arrSize = 14;
      break;

    case 4:
      // LOG-IN
      for (uint8_t i = 0; i < 13; i++) {
        message[i] = log_in[i];
      }
      arrSize = 13;
      break;

    case 5:
      // SUCCESS
      for (uint8_t i = 0; i < 14; i++) {
        message[i] = success_message[i];
      }
      arrSize = 14;
      break;


    case 6:
      // INFO
      for (uint8_t i = 0; i < 4; i++) {
        message[i] = info[i];
      }
      arrSize = 4;
      break;

    case 7:
      // CONFIRM
      for (uint8_t i = 0; i < 4; i++) {
        message[i] = conf[i];
      }
      arrSize = 4;
      break;

    case 8:
      // FOB
      for (uint8_t i = 0; i < 4; i++) {
        message[i] = fob[i];
      }
      arrSize = 4;
      break;
  }

  uint8_t data = 0;
  for (uint8_t i = 0; i < arrSize - 3; i++) {

    unsigned long t = millis();
    while (millis() - t < speed) {

      data = message[i + 0];
      shiftOut(pinDat, pinClk, MSBFIRST, data);
      digitalWrite(pinSs1, HIGH);
      delay(4);
      digitalWrite(pinSs1, LOW);

      data = message[i + 1];
      shiftOut(pinDat, pinClk, MSBFIRST, data);
      digitalWrite(pinSs2, HIGH);
      delay(4);
      digitalWrite(pinSs2, LOW);

      if (sms_avail) {
        if (gsm.available()) {
          return;
        }
      }

      data = message[i + 2];
      shiftOut(pinDat, pinClk, MSBFIRST, data);
      digitalWrite(pinSs3, HIGH);
      delay(4);
      digitalWrite(pinSs3, LOW);

      data = message[i + 3];
      shiftOut(pinDat, pinClk, MSBFIRST, data);
      digitalWrite(pinSs4, HIGH);
      delay(4);
      digitalWrite(pinSs4, LOW);
    }
  }
  rstSIPO();
}

uint8_t decoderBCD(int data) {
  uint8_t bcd = 0;

  switch (data) {
    case 0:
      bcd = 126;
      break;

    case 1:
      bcd = 48;
      break;

    case 2:
      bcd = 109;
      break;

    case 3:
      bcd = 121;
      break;

    case 4:
      bcd = 51;
      break;

    case 5:
      bcd = 91;
      break;

    case 6:
      bcd = 95;
      break;

    case 7:
      bcd = 112;
      break;

    case 8:
      bcd = 127;
      break;

    case 9:
      bcd = 123;
      break;

    default:
      bcd = 126;
  }
  return bcd;
}

void printSeg(int d0, int d1, int d2, int d3, bool isFour) {  // 1234

  int data;
  unsigned long t = millis();
  while ((millis() - t) < 5500) {

    if (isFour) {

      shiftOut(pinDat, pinClk, MSBFIRST, decoderBCD(d3));
      digitalWrite(pinSs1, HIGH);
      delay(4);
      digitalWrite(pinSs1, LOW);
    }

    shiftOut(pinDat, pinClk, MSBFIRST, decoderBCD(d2));
    digitalWrite(pinSs2, HIGH);
    delay(4);
    digitalWrite(pinSs2, LOW);

    shiftOut(pinDat, pinClk, MSBFIRST, decoderBCD(d1));
    digitalWrite(pinSs3, HIGH);
    delay(4);
    digitalWrite(pinSs3, LOW);

    shiftOut(pinDat, pinClk, MSBFIRST, decoderBCD(d0));
    digitalWrite(pinSs4, HIGH);
    delay(4);
    digitalWrite(pinSs4, LOW);
  }
  rstSIPO();
}

void flushChar() {
  memset(name1, '\0', sizeof(name1));
  memset(name2, '\0', sizeof(name2));
  memset(plate, '\0', sizeof(plate));
  memset(user_mobile, '\0', sizeof(user_mobile));
  memset(park, '\0', sizeof(park));
}

void sendSMS(int mode, bool cc) {
  String _sms;
  //bool _operator = false;

  switch (mode) {

    case 1:
      _sms = "INFORMATION RECEIVED\n\nName: " + String(name1) + String(name2) + "\nPlate: " + String(plate) + "\nPhone: " + String(user_mobile) + "\nPark: " + String(park) + "\n\n" + "Type YES to PROCEED";
      break;
    // case 2:
    //   _sms = "TAP RFID";
    //   break;
    case 3:
      _sms = String(name1) + String(name2) + " IS REGISTERED.";
      break;
    case 4:
      _sms = "PARKING NOTIFICATION\n\n" + String(name1) + String(name2) + " [" + String(plate) +"]" + " have successfully PARKED-" + String(park) + " from the logging sytem." + "\n\nUDHAI OFFICE";
      break;
    case 5:
      _sms = "TIME OUT";
      break;
  }


  if (cc) {

    gsm.println("AT+CMGF=1");
    delay(500);
    gsm.println("AT+CMGS=\"" + String(user_mobile) + "\"\n");
    delay(100);
    gsm.print(_sms);
    delay(100);
    gsm.write(26);
    delay(100);

    delay(5000);

    gsm.println("AT+CMGF=1");
    delay(500);
    gsm.println("AT+CMGS=\"" + String(office_mobile) + "\"\n");
    delay(100);
    gsm.print(_sms);
    delay(100);
    gsm.write(26);
    delay(100);

  } else {
    gsm.println("AT+CMGF=1");
    delay(500);
    gsm.println("AT+CMGS=\"" + String(user_mobile) + "\"\n");
    delay(100);
    gsm.print(_sms);
    delay(100);
    gsm.write(26);
    delay(100);
  }
  gsm.flush();
}

bool boot_sequence() {

  int c = 0;
  bool f = false;

  gsm.flush();

  gsm.println("AT");
  delay(680);

  if (gsm.available()) {
    if (parse_AT(1, gsm.readString())) {
      success(false, beep_short);
      c++;
    } else {
      error(false, beep_short);
    }
  }

  gsm.println("AT+CMGF=1");
  delay(680);


  if (gsm.available()) {
    if (parse_AT(1, gsm.readString())) {
      success(false, beep_short);
      c++;
    } else {
      error(false, beep_short);
    }
  }

  gsm.println("AT+CNMI=1,2,0,0,0");
  delay(680);

  if (gsm.available()) {
    if (parse_AT(1, gsm.readString())) {
      success(false, beep_short);
      c++;
    } else {
      error(false, beep_short);
    }
  }


  gsm.println("AT+CREG?");
  delay(680);


  if (gsm.available()) {
    if (parse_AT(2, gsm.readString())) {
      success(false, beep_short);
      c++;
    } else {
      error(false, beep_short);
    }
  }

  delay(680);

  if (c == 4) {
    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }

    delay(2200);
    f = true;

  } else {
    delay(680);
    f = false;
    boot_timeout += 1;
  }
  return f;
}

bool load_EEPROM() {
  for (int i = 0; i < 11; i++) {
    office_mobile[i] = EEPROM.read(i) + '0';
    if (!isDigit(office_mobile[i])) {
      return false;
    }
  }
  return true;
}

bool checkIntegrity(String sms) {

  String cname, cplate, cmobile, cpark;
  int chk = 0;

  cname = sms.substring(0, sms.indexOf(",", 0));
  cname.trim();

  uint8_t index = cname.length() + 1;
  cplate = sms.substring(index, sms.indexOf(",", index));

  index += cplate.length() + 1;
  cmobile = sms.substring(index, sms.indexOf(",", index));

  index += cmobile.length() + 1;
  cpark = sms.substring(index, sms.indexOf(",", index));

  if (cname.length() < 33) {
    if (checkAlpha(cname, 0, cname.length())) {
      chk++;
    }
  }

  if (cplate.length() < 8) {

    if (checkDigit(cplate, 3, cplate.length())) {
      chk++;
    }
    if (checkAlpha(cplate, 0, 3)) {
      chk++;
    }
  }

  if (cmobile.length() == 12 || cmobile.length() == 11) {
    if (cmobile.charAt(0) == '0' && cmobile.charAt(1) == '9') {
      if (checkDigit(cmobile, 0, cmobile.length())) {
        chk++;
      }
    }
  }

  if (cpark == "IN" || cpark == "OUT") {
    if (checkAlpha(cpark, 0, cpark.length())) {
      chk++;
    }
  }
  if (chk == 5) {
    return true;
  } else {

    flushChar();
    return false;
  }
}

bool checkDigit(String s, uint8_t start_index, uint8_t end_index) {
  for (uint8_t i = start_index; i < end_index; i++) {
    if (!(isDigit(s.charAt(i)))) {
      return false;
    }
  }
  return true;
}

bool checkAlpha(String s, uint8_t start_index, uint8_t end_index) {

  for (uint8_t i = start_index; i < end_index; i++) {
    if (!(isAlpha(s.charAt(i)) || s.charAt(i) == ' ' || s.charAt(i) == '.')) {
      return false;
    }
  }
  return true;
}

bool getSMS_OfficeNumber(String parse) {
  bool _flag = false;

  String _sms = parse;
  _sms.remove(0, 49);
  _sms.trim();

  if (_sms.length() == 11 && _sms.charAt(0) == '0' && _sms.charAt(1) == '9' && checkDigit(_sms, 2, 9)) {
    for (int y = 0; y < 11; y++) {
      office_mobile[y] = _sms[y];
      office_mobile[y + 1] = '\0';
      if (y == 10) {
        return true;
      }
    }
  } else {
    return false;
  }
}

bool getSMS_Registration(String parse) {

  bool _flag = false;
  String _sms = parse;

  _sms.remove(0, 49);
  _sms.trim();


  if (checkSMS(3, parse)) {
    if (checkIntegrity(_sms)) {

      String cname, cplate, cmobile, cpark;
      cname = _sms.substring(0, _sms.indexOf(",", 0));
      cname.trim();

      uint8_t index = cname.length() + 1;
      cplate = _sms.substring(index, _sms.indexOf(",", index));

      index += cplate.length() + 1;
      cmobile = _sms.substring(index, _sms.indexOf(",", index));

      index += cmobile.length() + 1;
      cpark = _sms.substring(index, _sms.indexOf(",", index));

      for (uint8_t p = 0; p < cname.length(); p++) {
        if (p < 16) {
          name1[p] = cname[p];
          name1[p + 1] = '\0';
        } else {
          name2[p - 16] = cname[p];
          name2[p - 15] = '\0';
        }
      }

      for (uint8_t k = 0; k < cplate.length(); k++) {
        plate[k] = cplate[k];
        plate[k + 1] = '\0';
      }

      for (uint8_t y = 0; y < cmobile.length(); y++) {
        user_mobile[y] = cmobile[y];
        user_mobile[y + 1] = '\0';
      }

      for (uint8_t j = 0; j < cpark.length(); j++) {
        park[j] = cpark[j];
        park[j + 1] = '\0';
      }

      _flag = true;
    }
  }
  return _flag;
}

bool readRFID() {
  bool temp = false;
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      temp = true;
    }
  }
  return temp;
}

bool writeDataToBlock(int mode) {

  byte beef[16];
  int mem[5] = { 1, 2, 4, 5, 6 };
  char* ptr[] = { name1, name2, plate, user_mobile, park };

  switch (mode) {

    case 1:
      for (int i = 0; i < 5; i++) {
        if ((mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, mem[i], &key, &(mfrc522.uid))) == MFRC522::STATUS_OK) {
          for (int k = 0; k < 16; k++) {
            beef[k] = static_cast<uint8_t>(ptr[i][k]);
          }
          if ((mfrc522.MIFARE_Write(mem[i], beef, 16)) == MFRC522::STATUS_OK) {

          } else {
            return false;
          }

        } else {
          return false;
        }
      }

      break;

    case 2:
      if ((mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 6, &key, &(mfrc522.uid))) == MFRC522::STATUS_OK) {
        for (int y = 0; y < 16; y++) {
          beef[y] = static_cast<uint8_t>(park[y]);
        }

        if ((mfrc522.MIFARE_Write(6, beef, 16)) == MFRC522::STATUS_OK) {

        } else {
          return false;
        }
      } else {
        return false;
      }
      break;
  }
  return true;
}

bool readDataFromBlock() {
  flushChar();

  byte beef[16];
  byte buffer = 18;
  char* ptr[] = { name1, name2, plate, user_mobile, park };
  int mem[5] = { 1, 2, 4, 5, 6 };

  for (int i = 0; i < 5; i++) {
    if ((mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, mem[i], &key, &(mfrc522.uid))) == MFRC522::STATUS_OK) {
      if ((mfrc522.MIFARE_Read(mem[i], beef, &buffer)) == MFRC522::STATUS_OK) {
        for (int t = 0; t < 16; t++) {
          ptr[i][t] = beef[t];
        }
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool parse_AT(int at_command, String at_reply) {
  int i = at_reply.indexOf('\n');

  if (i != -1) {
    at_reply = at_reply.substring(i + 1);
    at_reply.trim();
  }

  switch (at_command) {
    case 1:
      if (at_reply == "OK") {
        return true;
      }
      break;

    case 2:
      i = at_reply.indexOf(',');
      at_reply = at_reply.substring(i + 1, i + 2);
      at_reply.trim();
      if (at_reply == "1") {
        return true;
      }
      break;

    default:
      return false;
      break;
  }
  return false;
}

bool checkSMS(int mode, String sms) {
  int s_index = 0;
  int chk = 0;

  sms.remove(0, 49);
  sms.trim();
  bool _flag = false;

  switch (mode) {
    case 1:
      if (sms == "YES" || sms == "Yes" || sms == "yes") {
        _flag = true;
      } 
      break;

    case 2:                                            // EASTER EGG
      if (sms.length() == 11 || sms.length() == 13) {  // CHECKS THE OPERATOR NUMBER LENGTH
        _flag = true;
      }
      break;

    case 3:
      for (int i = 0; i < sms.length(); i++) {
        if (sms.charAt(i) == ',') {
          chk++;
        }
      }

      if (chk != 3) {
        _flag = false;
      } else {
        _flag = true;
      }
      break;

    default:
      _flag = false;
      break;
  }
  return _flag;
}
