
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
* *THIS MEANS THAT THE BLUE LED OUTPUT ENDS UP ON A3!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* 
* hid.h
* C:\Users\nathangartner\AppData\Local\arduino15\packages\adafruit\hardware\nrf52\0.20.1\cores\nRF5\TinyUSB\Adafruit_TinyUSB_ArduinoCore\tinyusb\src\class\hid
* 
* Media Control
* Lines 566 & 567
* 
* HID_USAGE_CONSUMER_PLAY                              = 0x00B0,
* HID_USAGE_CONSUMER_PAUSE                             = 0x00B1,
*
* 
********************************************************************* 
* 03.05.2021: minSPEED was adjusted from '50' to '30' and the map() function for minSPEED was changed to the following
* 
* Line 195:     minSPEED = map(pVal, 0, 935, 110, 50);
* 
* Changed to...
* 
* Line 195:     minSPEED = map(pVal, 0, 935, 80, 30);
* *******************************************************************
* 03.16.2021: minSPEED adjusted further as above to '15'on lLine: 97 and the map() function for minSPEED was changed to the following
* 
 * Line 223:     minSPEED = map(pVal, 0, 935, 80, 30);
* 
* Changed to...
* 
* Line 223:      minSPEED = map(pVal, 0, 935, 60, 15);
* 
* Adjusted longPeriod from 2000ms to 3000ms
* 
* ********************************************************************
* 04.15.2021: Addded in ADC code. This is a tester for TSI Project. It is all mostly commevted out and will not interfere with normal Motivex Functionality.
* 
* ********************************************************************
* 05.25.2021: File system for storing lock button data to eeprom
* 
*/


//#include <Adafruit_ADS1015.h>
#include <Wire.h>
#include <bluefruit.h>
#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

//Little FS variables & defines
using namespace Adafruit_LittleFS_Namespace;


File file(InternalFS);
#define FILENAME	"/lockBtn.txt"
uint8_t lock[2] = { };
uint8_t toggleVal = 0;

// ADC instance
//Adafruit_ADS1015 ads1015;


BLEDis bledis;
BLEHidAdafruit blehid;

// Timer Callback
SoftwareTimer blinkTimer;

//#define statLED     19
#define effPot      A0
#define sOrbit      A1 //Swapped the s and l kaden
#define lOrbit      A2
#define bluLED      A3 // kaden added for blue tooth blue led indecator instead of using libary modes 
#define LED_CHRG    A4
#define lockBtn     A5      
#define minCHRG     610
#define Free_Mode   0
#define Lock_Mode   1

//transport control
#define HID_USAGE_CONSUMER_PAUSE   0xB1
#define HID_USAGE_CONSUMER_PLAY    0xB0

// millis timer kaden
unsigned long currentMillis =   0;
unsigned long buttonMillis =    0;
unsigned long prevMillis =      0;
unsigned long button2Millis =   0;
unsigned long initMillis =      0;
unsigned long RPMMillis =       0;
unsigned long batMillis =       0;
unsigned long lockMillis =      0;
unsigned long pLockMillis =     0;

const long period =             300;          //the value is a number of milliseconds
const long longPeriod =         3000;         //the value is a number of milliseconds
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
bool lockBtnVal =       0;
bool prevLockBtnVal =   0;
bool lflg =             0;
bool plflg =            0;
bool writeFlg =         0;
bool flg3 =             0;
bool ledState =         0;
bool pLedState =        0;

int RPM =               0;
int minSPEED =          15;
int pminSPEED =         15;
int pVal =              0;
int batVal =            0;
float minutes =         0;
float interval =        0;
int cntr =              0;

// enumerated states for the LED's
enum LEDstates {

    LED_OFF,
    LED_ON,
    LED_BLINK_2,
    LED_BLINK_3
};

int green_led = LED_OFF;


void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);        // for nrf52840 with native usb
//***>>> DEBUG - BEGIN <<<***
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
  //***>>> DEBUG - END <<<*** 

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName("Motivex_YouTube");

// Configure and Start Device Information Service
bledis.setManufacturer("Adafruit Industries");
bledis.setModel("Bluefruit Feather 52");
bledis.begin();

//ADC Begin
//ads1015.begin();

//ADC Setup
//ads1015.setGain(GAIN_TWO);
//ads1015.startComparator_SingleEnded(0, 1000);

// set up pin as input
pinMode(effPot, INPUT);
pinMode(PIN_VBAT, INPUT_PULLUP);
pinMode(LED_CHRG, OUTPUT);
pinMode(sOrbit, INPUT_PULLUP);
pinMode(bluLED, OUTPUT);//kaden added for blue led setup 
attachInterrupt(digitalPinToInterrupt(sOrbit), fsOrbit, FALLING);
pinMode(lOrbit, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(lOrbit), flOrbit, FALLING);
digitalWrite(19, LOW);
pinMode(lockBtn, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(lockBtn), lockToggle, FALLING);

batVal = analogRead(PIN_VBAT);
digitalWrite(LED_CHRG, LOW);

// Configure the timer with 1000 ms interval, with our callback
blinkTimer.begin(150, blink_timer_callback);

/* Start BLE HID
 * Note: Apple requires BLE device must have min connection interval >= 20m
 * ( The smaller the connection interval the faster we could send data).
 * However for HID and MIDI device, Apple could accept min connection interval
 * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
 * connection interval to 11.25  ms and 15 ms respectively for best performance.
 */
blehid.begin();

Serial.println("FS_init");
// Initialize Internal File System
InternalFS.begin();


file.open(FILENAME, FILE_O_READ);

// file existed
if (file)
{
    Serial.println(FILENAME " file exists");

    uint32_t readlen;
    char buffer[64] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    for (int i = 0; i < sizeof(lock); i++)
    {
        lock[i] = buffer[i];
        Serial.println(lock[i]);
    }

    file.close();
    
}

Serial.println("File Closed");

lockBtnVal = lock[1];
if (lockBtnVal == true) green_led = LED_BLINK_3;
if (lockBtnVal == false) green_led = LED_BLINK_2;
 

/* Set connection interval (min, max) to your perferred value.
 * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
 * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
 */
 /* Bluefruit.Periph.setConnInterval(9, 12); */

blinkTimer.start();

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

void fsOrbit()//Intterut handler for the small orbit of hal sensor *Kaden Added Comment*
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

void flOrbit()//Intterut handler for the large orbit of hal sensor *Kaden Added Comment*
{
    sw2State = digitalRead(A2);
    LEDstate = !LEDstate;
    sw2Count++;
    button2Millis = currentMillis;
    initMillis = currentMillis;
}

void lockToggle()
{
    lockBtnVal = !lockBtnVal;
    lock[1] = lockBtnVal;
    lflg = !lflg;
    writeFlg = true;
    //Serial.println("Lock Int");

}

void Lock()
{
    if (writeFlg == true)
    {
        pVal = analogRead(effPot);
        minSPEED = map(pVal, 0, 935, 15, 80);
        lock[0] = minSPEED;
        InternalFS.remove(FILENAME);
        file.open(FILENAME, FILE_O_WRITE);
        //Serial.print("Open " FILENAME " file to write ... ");

        if (file.open(FILENAME, FILE_O_WRITE))
        {
            //Serial.println("OK");
            file.write((const uint8_t*)lock, sizeof(lock));
            //Serial.print("lock_speed: "); Serial.println(lock[0]);
            file.close();
            writeFlg = false;
            green_led = LED_BLINK_3;
        }
    }
}

void unLock()
{
    if (writeFlg == true)
    {
        InternalFS.remove(FILENAME);
        file.open(FILENAME, FILE_O_WRITE);
        //Serial.print("Open " FILENAME " file to write ... ");

        if (file.open(FILENAME, FILE_O_WRITE))
        {
            //Serial.println("OK");
            file.write((const uint8_t*)lock, sizeof(lock));
            //Serial.print("lock_speed: "); Serial.println(lock[0]);
            file.close();
            writeFlg = false;
            green_led = LED_BLINK_2;
        }
    }
}

void printVal()
{
    //if (plflg != lflg) {
    //    Serial.print("loc0: "); Serial.println(lock[0]);
    //    Serial.print("loc1: "); Serial.println(lock[1]);
    //    Serial.print("minS: "); Serial.println(minSPEED);
    //    file.open(FILENAME, FILE_O_READ);

    //    // file existed
    //    if (file)
    //    {
    //        Serial.println(FILENAME " file exists");

    //        uint32_t readlen;
    //        char buffer[64] = { 0 };
    //        readlen = file.read(buffer, sizeof(buffer));

    //        buffer[readlen] = 0;
    //        for (int i = 0; i < sizeof(lock); i++)
    //        {
    //            lock[i] = buffer[i];
    //            Serial.println(lock[i]);
    //        }

    //        file.close();

    //    }
    //    plflg = lflg;
    //}
    //if (pminSPEED != minSPEED)
    //{
    //    Serial.print("minS: "); Serial.println(minSPEED);
    //    pminSPEED = minSPEED;
    //}
}

void videoAction()
{

    if (lock[1] == false)
    {
        unLock();
        pVal = analogRead(effPot);
        minSPEED = map(pVal, 0, 935, 15, 80);
    }
    else if (lock[1] == true)
    {
        Lock();
        minSPEED = lock[0];
    }

    printVal();
        
    int btime = (currentMillis - buttonMillis);
    int b2time = (currentMillis - button2Millis);
    int initTime = (currentMillis - initMillis);

    if (RPM >= minSPEED) {
        flg = 1;
    }
    else {
        flg = 0;
    }

    if (b2time >= longPeriod) {
        flg = 0;
        //Serial.println("flag set");
    }
    /*Kaden removed this would keep doing this every 5 secounds or so where it would pause and replay. */
    if (initTime >= initPeriod) {
        Serial.println("\t\tPLAY PAUSE");
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
    Serial.print("RPM: "); Serial.println(RPM);
    Serial.print("minSPEED: "); Serial.println(minSPEED);
    Serial.print("lock[1]: "); Serial.println(lock[1]);
    batCheck();
    
    if (digitalRead(LED_BLUE) == HIGH) {//kaden added for Blue tooth led to trun on the extern blue led at the same time as the on board blue led 
      digitalWrite(bluLED, HIGH);
    }  else {
      digitalWrite(bluLED, LOW);
    }
}

// LED blink interrupt to set flag and trigger clientUartPrint()
void blink_timer_callback(TimerHandle_t xTimerID)
{
    (void)xTimerID;
    if (green_led == LED_OFF)
    {
        digitalWrite(LED_CHRG, LOW);
        cntr = 0;
    }
    if (green_led == LED_BLINK_2)
    {
            cntr = cntr +1;
            if (cntr < 2)
            {
                digitalToggle(LED_CHRG);
            }
            else if (cntr >= 2)
            {
                green_led = LED_OFF;
            }
    }
    if (green_led == LED_BLINK_3)
    {
            cntr = cntr + 1;
            if (cntr < 6) 
            {
                digitalToggle(LED_CHRG);
            }
            else if (cntr >= 6)
            {
                green_led = LED_OFF;
            }       
    }

}

