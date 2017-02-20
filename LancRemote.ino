/*
 * 
SIMPLE LANC REMOTE
Version 1.0
Sends LANC commands to the LANC port of a video camera.
Tested with a Canon XF300 camcorder
For the interface circuit interface see 
http://controlyourcamera.blogspot.com/2011/02/arduino-controlled-video-recording-over.html
Feel free to use this code in any way you want.
2011, Martin Koch

"LANC" is a registered trademark of SONY.
CANON calls their LANC compatible port "REMOTE".

  Modified for (my) readability by Alex Sokolsky.  2/2017
  

*/
#include "Trace.h"
#include "PCB.h"
/** LANC Timings */

/** Duration of one LANC bit in microseconds. */
const uint8_t bitDuration = 104; 
/** Writing to the digital port takes about 8 microseconds */
const uint8_t digitalWriteDuration  = 8;
/** mksecs left for write bit */
const uint8_t writeBitDuration = bitDuration-digitalWriteDuration;

/** Reading to the digital port takes about 4 microseconds */
const uint8_t digitalReadDuration = 4;
/** so only 100 microseconds are left for each bit */
const uint8_t readBitDuration = bitDuration-digitalReadDuration;

/** LANC Command Has to be Repeated */
const short int cmdRepeatTimes = 5;

//LANC commands byte 0 + byte 1
//Tested with Canon XF300

//Start-stop video recording
uint8_t REC[] = {0x18, 0x33};

//Zoom in from slowest to fastest speed
uint8_t ZOOM_IN_0[] = {0x28, 0x0};
uint8_t ZOOM_IN_1[] = {0x28, 0x2};
uint8_t ZOOM_IN_2[] = {0x28, 0x4};
uint8_t ZOOM_IN_3[] = {0x28, 0x6};
uint8_t ZOOM_IN_4[] = {0x28, 0x8};
uint8_t ZOOM_IN_5[] = {0x28, 0xA};
uint8_t ZOOM_IN_6[] = {0x28, 0xC};
uint8_t ZOOM_IN_7[] = {0x28, 0xE};

//Zoom out from slowest to fastest speed
uint8_t ZOOM_OUT_0[] = {0x28, 0x10};
uint8_t ZOOM_OUT_1[] = {0x28, 0x12};
uint8_t ZOOM_OUT_2[] = {0x28, 0x14};
uint8_t ZOOM_OUT_3[] = {0x28, 0x16};
uint8_t ZOOM_OUT_4[] = {0x28, 0x18};
uint8_t ZOOM_OUT_5[] = {0x28, 0x1A};
uint8_t ZOOM_OUT_6[] = {0x28, 0x1C};
uint8_t ZOOM_OUT_7[] = {0x28, 0x1E};

//Focus control. Camera must be switched to manual focus
uint8_t FOCUS_AUTO[] = {0x28, 0x41};
uint8_t FOCUS_FAR[]  = {0x28, 0x45};
uint8_t FOCUS_NEAR[] = {0x28, 0x47};

//uint8_t POWER_ON[] = {0x18, 0x5C}; //  Doesn't work because there's no power supply from the LANC port when the camera is off
//uint8_t POWER_OFF[] = {0x18, 0x5E};
//uint8_t POWER_OFF2[] = {0x18, 0x2A}; // Turns the XF300 off and then on again
//uint8_t POWER_SAVE[] = {0x18, 0x6C}; // Didn't work

void setup() 
{
  pinMode(pinLancIn, INPUT);   //listens to the LANC line
  pinMode(pinLancOut, OUTPUT); //writes to the LANC line
  
  pinMode(pinRecButton, INPUT_PULLUP); //start-stop recording button 
  pinMode(pinZoomOutButton, INPUT_PULLUP); 
  pinMode(pinZoomInButton, INPUT_PULLUP); 
  pinMode(pinFocusNearButton, INPUT_PULLUP); 
  pinMode(pinFocusFarButton, INPUT_PULLUP); 
  
  digitalWrite(pinLancOut, LOW); //set LANC line to +5V

  Serial.begin(115200);
  Serial.println("READY");
  
  delay(5000); //Wait for camera to power up completly
}

void loop() 
{
  // this presumes all keys are on ports 0-7
  uint8_t ins = PIND;

  DEBUG_PRINT("ins=0x"); DEBUG_PRINTHEX(ins); DEBUG_PRINTLN("");
  
  if(bitRead(ins, pinRecButton) == 0)
    lancCommand(REC);
  if(bitRead(ins, pinZoomOutButton) == 0)
    lancCommand(ZOOM_OUT_4); 
  else if(bitRead(ins, pinZoomInButton) == 0)
    lancCommand(ZOOM_IN_4);
  if(bitRead(ins, pinFocusNearButton) == 0)
    lancCommand(FOCUS_NEAR);
  else if(bitRead(ins, pinFocusFarButton) == 0)
    lancCommand(FOCUS_FAR); 
}

static void writeByte(uint8_t data)
{
  delayMicroseconds(bitDuration);  // wait START bit duration
  for(short int i=0; i<8; i++) 
  {
    digitalWrite(pinLancOut, bitRead(data, i));
    delayMicroseconds(writeBitDuration); 
  }
}

void lancCommand(uint8_t command[2])
{    
  //DEBUG_PRINT("lancCommand(0x"); DEBUG_PRINTHEX(command[0]); DEBUG_PRINT(",0x"); DEBUG_PRINTHEX(command[1]); DEBUG_PRINTLN(")");
  //uint8_t mode = command[0];
  //uint8_t cmd = command[1];
  // LANC requires for the command to be repeated 3 or 5 times.
  for(short int i = 0; i < cmdRepeatTimes; i++) 
  {
    while(pulseIn(pinLancIn, HIGH) < 5000) 
    {
        //"pulseIn, HIGH" catches any 0V TO +5V TRANSITION and waits until the LANC line goes back to 0V 
        //"pulseIn" also returns the pulse duration so we can check if the previous +5V duration was long enough (>5ms) to be the pause before a new 8 byte data packet
        //Loop till pulse duration is >5ms
        ;
    }

    writeByte(command[0]);
    
    // Byte 0 is written now put LANC line back to +5V
    digitalWrite(pinLancOut, LOW);

    delayMicroseconds(10); //make sure to be in the stop bit before byte 1
    
    while(digitalRead(pinLancIn))
    { 
      // Loop as long as the LANC line is +5V during the stop bit
      ;
    }

    writeByte(command[1]);

    //Byte 1 is written now put LANC line back to +5V
    digitalWrite(pinLancOut, LOW); 
    // Control bytes 0 and 1 are written, 
    // now don’t care what happens in Bytes 2 to 7
    // and just wait for the next start bit after a long pause to send the first two command bytes again.
  }
}

