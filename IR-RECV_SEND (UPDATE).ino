#include <Arduino.h>
#include <IRremote.hpp>

struct storedIRDataStruct {
    IRData receivedIRData;
    // extensions for sendRaw
    uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
    uint8_t rawCodeLength; // The length of the code
} sStoredIRData[2];

void storeCode(IRData *aIRReceivedData, int n);
void sendCode(storedIRDataStruct *aIRDataToSend);

int RECV_PIN = 2; // Tsop pin
int SEND_PIN = 3; // Ir Led pin

int IN_BUTTON = 5; // take in data
int SETUP_BUTTON = 9; // Change MOD
int OFF_BUTTON = 12; // off
int ON_BUTTON = 11; // on 

int DELAY_BETWEEN_REPEAT = 50;
int DEFAULT_NUMBER_OF_REPEATS_TO_SEND = 3;

int flg = 1;
int setup_flg = 0;
int on_flg = 0;
int off_flg = 0;

void setup() {
  Serial.begin(115200);

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif

  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK);
  pinMode(IN_BUTTON, INPUT);
  pinMode(SETUP_BUTTON, INPUT);
  pinMode(OFF_BUTTON, INPUT);
  pinMode(ON_BUTTON, INPUT);
}

void loop() {
  if (digitalRead(SETUP_BUTTON) == HIGH) { // 9
    flg = 1;
    setup_flg = 1;
    on_flg = 0;
    off_flg = 0;
  }

  /* setup on and off */
  if (setup_flg == 1) {
    if (flg == 1) {
      Serial.println("SETUP ON!");
      IrReceiver.start();
      flg = 2;
    }
    
    /* for on button */
    if (on_flg == 0) {
      if (flg == 2) {
        Serial.println("TAKING IN ON DATA..");
        flg = 3;
      }
      while (digitalRead(IN_BUTTON) == HIGH) {
        if (IrReceiver.available()) {
          storeCode(IrReceiver.read(), 0);
          IrReceiver.resume();
        }
        on_flg = 1;
      }
    }

    /* for off button */
    if (on_flg == 1 && off_flg == 0) {
      if (flg == 3) {
        Serial.println("TAKING IN OFF DATA..");
        flg = 4;
      }
      while (digitalRead(IN_BUTTON) == HIGH) {
        if (IrReceiver.available()) {
          storeCode(IrReceiver.read(), 1);
          IrReceiver.resume();
        }
        off_flg = 1;
      }
    }

    if (on_flg == 1 && off_flg == 1) {
      setup_flg = 0;
    }
  } else {
    /* run on and off */
    if (flg == 4) {
      IrReceiver.stop();
      flg = 0;
    }

    if (digitalRead(ON_BUTTON) == HIGH) {
      Serial.println("ON BUTTON PRESSED..");
      sendCode(&sStoredIRData[0]); 
    }

    if (digitalRead(OFF_BUTTON) == HIGH) {
      Serial.println("OFF BUTTON PRESSED..");
      sendCode(&sStoredIRData[1]); 
    }

    delay(DELAY_BETWEEN_REPEAT);
  }
}

void storeCode(IRData *aIRReceivedData, int n) {
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
        Serial.println(F("Ignore repeat"));
        return;
    }
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
        Serial.println(F("Ignore autorepeat"));
        return;
    }
    if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
        Serial.println(F("Ignore parity error"));
        return;
    }
    /*
     * Copy decoded data
     */
    sStoredIRData[n].receivedIRData = *aIRReceivedData;

    if (sStoredIRData[n].receivedIRData.protocol == UNKNOWN) {
        Serial.print(F("Received unknown code and store "));
        Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
        Serial.println(F(" timing entries as raw "));
        IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
        sStoredIRData[n].rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
        /*
         * Store the current raw data in a dedicated array for later usage
         */
        IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData[n].rawCode);
    } else {
        IrReceiver.printIRResultShort(&Serial);
        sStoredIRData[n].receivedIRData.flags = 0; // clear flags -esp. repeat- for later sending
        Serial.println();
    }
}

void sendCode(storedIRDataStruct *aIRDataToSend) {
    if (aIRDataToSend->receivedIRData.protocol == UNKNOWN /* i.e. raw */) {
        // Assume 38 KHz
        IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);

        Serial.print(F("Sent raw "));
        Serial.print(aIRDataToSend->rawCodeLength);
        Serial.println(F(" marks or spaces"));
    } else {

        /*
         * Use the write function, which does the switch for different protocols
         */
        IrSender.write(&aIRDataToSend->receivedIRData, DEFAULT_NUMBER_OF_REPEATS_TO_SEND);

        Serial.print(F("Sent: "));
        printIRResultShort(&Serial, &aIRDataToSend->receivedIRData);
    }
}
