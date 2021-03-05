/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* 
*This sketch uses two reed switches to determine pedaling cadance
* If the user meets the minSPEED, the BLE Device will send Media control signals to play
* a video on YoutTube Kids on an iPad in Guided Access Mode.
* 
* The files and libraries have been modified to have BLE Connection LED indication on a gpio pin, NOT the onboard Blue LED on the Bluefruit
* 
* Libraries changed on Computer Work Station : CANA-0040
* 
* Modified files include:
* 
* variant.h
* C:\Users\nathangartner\AppData\Local\arduino15\packages\adafruit\hardware\nrf52\0.20.1\variants\feather_nrf52832
* 
* Line 50:  LED_CONN changed to 5 from 19
* Line 53:  LED_BLUE changed to 5 from 19
* Line 81:  PIN_VBAT changed to PIN_A7 from ???
*
* 
********************************************************************* 
* 03.05.2021: minSPEED was adjusted from '50' to '30' and the map() function for minSPEEDwas changed to the following
* 
* Line 195:     minSPEED = map(pVal, 0, 935, 110, 50);
* 
* Changed to...
* 
* Line 195:     minSPEED = map(pVal, 0, 935, 80, 30);
* *******************************************************************
* 
 */


#include <Wire.h>
#include <bluefruit.h>
#include <Arduino.h>



BLEDis bledis;
BLEHidAdafruit blehid;

// Timer Callback
SoftwareTimer blinkTimer;

//#define statLED     19
#define effPot      A0
#define sOrbit      A2
#define lOrbit      A1
#define LED_CHRG    A4
#define minCHRG     710

// millis timer
unsigned long currentMillis =   0;
unsigned long buttonMillis =    0;
unsigned long prevMillis =      0;
unsigned long button2Millis =   0;
unsigned long initMillis =      0;
unsigned long RPMMillis =       0;
unsigned long batMillis =        0; 

const long period =             300;          //the value is a number of milliseconds
const long longPeriod =         2000;         //the value is a number of milliseconds
const long initPeriod =         4000;

// Variables that will change:
int swCount =           0;               // counter for the number of button presses
int sw2Count =          0;               // counter for the number of button2 presses
int prevSwCount =       0;               // counter for previous switch count value
int prevSw2Count =      0;               // counter for previous switch2 count value
int swState =           0;               // current state of the button
int sw2State =          0;               // current state of button2
int lastSwState =       0;               // previous state of the button
int lastSw2State =      0;               // previous state of the button2
bool LEDstate =         0;

bool flg =              0;
bool initFlg =          0;
bool batFlg =           0;

int RPM =               0;
int minSPEED =          30;
int pVal =              0;
int batVal =            0;
float minutes =         0;
float interval =        0;



void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);        // for nrf52840 with native usb

  Serial.println("");
  Serial.println("");
  Serial.println("Bluefruit52 HID Keyscan w/ Reed Switch");
  Serial.println("--------------------------------------\n");
  Serial.println();
  Serial.println("CanAssist MOTIVEX YouTube");
  Serial.println();
  Serial.println("Go to your phone's Bluetooth settings to pair your device");
  Serial.println("then open the YouTube for Kids app on iOS");

  Serial.println();  

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName("Motivex_YouTube");

  // Configure and Start Device Information Service
bledis.setManufacturer("Adafruit Industries");
bledis.setModel("Bluefruit Feather 52");
bledis.begin();

// set up pin as input
pinMode(effPot, INPUT);
pinMode(PIN_VBAT, INPUT);
pinMode(LED_CHRG, OUTPUT);
pinMode(sOrbit, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(sOrbit), fsOrbit, FALLING);
pinMode(lOrbit, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(lOrbit), flOrbit, FALLING);
digitalWrite(19, LOW);

batVal = analogRead(PIN_VBAT);

// Configure the timer with 1000 ms interval, with our callback
//blinkTimer.begin(100, blink_timer_callback);

/* Start BLE HID
 * Note: Apple requires BLE device must have min connection interval >= 20m
 * ( The smaller the connection interval the faster we could send data).
 * However for HID and MIDI device, Apple could accept min connection interval
 * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
 * connection interval to 11.25  ms and 15 ms respectively for best performance.
 */
blehid.begin();

/* Set connection interval (min, max) to your perferred value.
 * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
 * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
 */
 /* Bluefruit.Periph.setConnInterval(9, 12); */

 // Set up and start advertising
startAdv();

}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

    // Include BLE HID service
    Bluefruit.Advertising.addService(blehid);

    // There is enough room for the dev name in the advertising packet
    Bluefruit.Advertising.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void fsOrbit()
{
    swState = digitalRead(A1);

    swCount++;
    if (swCount == 2) {
        float cnTime = (currentMillis - prevMillis);
        cnTime = (cnTime / (1000 * 60));
        RPM = (2 / (5 * cnTime));              
        swCount = 0;
        prevMillis = currentMillis;
    }
    buttonMillis = currentMillis;
    initMillis = currentMillis;
}

void flOrbit()
{
    sw2State = digitalRead(A2);
    LEDstate = !LEDstate;
    sw2Count++;
    button2Millis = currentMillis;
    initMillis = currentMillis;
}

void videoAction()
{
    pVal = analogRead(effPot);

    minSPEED = map(pVal, 0, 935, 80, 30);

    int btime = (currentMillis - buttonMillis);
    int b2time = (currentMillis - button2Millis);
    int initTime = (currentMillis - initMillis);

    if (RPM >= minSPEED) {
        flg = 1;
        //Serial.println("flag set");
    }
    else {
        flg = 0;
    }

    if (b2time >= longPeriod) {
        flg = 0;
        //Serial.println("flag set");
    }

    if (initTime >= initPeriod) {
        //Serial.println("\t\tPLAY PAUSE");
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY);
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_PAUSE);
        initMillis = currentMillis;
    }

    switch (flg) {
    case 0:
        //Serial.println("PAUSE");

        blehid.consumerKeyPress(HID_USAGE_CONSUMER_PAUSE);
        break;
    case 1:
        //Serial.println("PLAY");
        blehid.consumerKeyPress(HID_USAGE_CONSUMER_PLAY);
        break;
    }
    //delay(2);
}

void batCheck()
{
    batVal = analogRead(PIN_VBAT);
    //Serial.println(batVal);

    if (batVal <= minCHRG) {
        int blinkTime = (currentMillis - batMillis);

        if(blinkTime >= 80) {
            digitalToggle(LED_CHRG);
            batMillis = currentMillis;
        }       
    }
    else {
        digitalWrite(LED_CHRG, false);
        //Serial.println("Not Blinking...");
    }
}

void loop()
{
    // for the millis timers
    currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
    videoAction();
    batCheck();
}

