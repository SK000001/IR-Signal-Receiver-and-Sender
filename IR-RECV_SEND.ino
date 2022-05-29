#include <Arduino.h>
#include <IRremote.hpp>

int RECV_PIN = 2;
int SEND_PIN = 3;
int SEND_BUTTON_PIN = 5;
int DELAY_BETWEEN_REPEAT = 1000;
int DEFAULT_NUMBER_OF_REPEATS_TO_SEND = 3;

struct storedIRDataStruct {
    IRData receivedIRData;
    // extensions for sendRaw
    uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
    uint8_t rawCodeLength; // The length of the code
} sStoredIRData;
int flg = 0;

void storeCode(IRData *aIRReceivedData);
void sendCode(storedIRDataStruct *aIRDataToSend);

void setup() {
  Serial.begin(115200);

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif

  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(SEND_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {
  int buttonState = digitalRead(SEND_BUTTON_PIN);

  /* READ AT BUTTON PRESS AND SEND AT BUTTON RELEASE */
  if (buttonState == LOW) {
    if (flg == 0) {
      Serial.println(F("Button Released"));
      IrReceiver.stop();
      flg = 1;
    }
    sendCode(&sStoredIRData);
    delay(DELAY_BETWEEN_REPEAT);
  } else {
    if (flg == 1) {
      IrReceiver.start();
      Serial.println(F("Button Pressed"));
      flg = 0;
    }
    if (IrReceiver.available()) {
      storeCode(IrReceiver.read());
      IrReceiver.resume();
    }
  }

  /* READ AT BUTTON RELEASE AND SEND AT BUTTON PRESS */
  // if (buttonState == LOW) {
  //   if (flg==0) {
  //     IrReceiver.start();
  //     Serial.println(F("Button released"));
  //     flg = 1;
  //   }
  //   if (IrReceiver.available()) {
  //     storeCode(IrReceiver.read());
  //     IrReceiver.resume();
  //   }
  // } else {
  //     flg = 0;
  //     Serial.println(F("Button Pressed"));
  //     IrReceiver.stop();
  //     sendCode(&sStoredIRData);
  //     delay(DELAY_BETWEEN_REPEAT);
  // }
}

void storeCode(IRData *aIRReceivedData) {
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
    sStoredIRData.receivedIRData = *aIRReceivedData;

    if (sStoredIRData.receivedIRData.protocol == UNKNOWN) {
        Serial.print(F("Received unknown code and store "));
        Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
        Serial.println(F(" timing entries as raw "));
        IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
        sStoredIRData.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
        /*
         * Store the current raw data in a dedicated array for later usage
         */
        IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData.rawCode);
    } else {
        IrReceiver.printIRResultShort(&Serial);
        sStoredIRData.receivedIRData.flags = 0; // clear flags -esp. repeat- for later sending
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
