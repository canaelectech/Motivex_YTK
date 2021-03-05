# Motivex_YTK

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
