/*
IMPORTANT: The ADAFRUIT PWM library has to be modified such that when the "off" value is 0 it actually writes 4096 to completely turn off the lights.
  
Scene fade mapping:
Fade Time / Current Color Value = Delay time per step
Delay time per step is an int so the decimal is truncated
Loop runs and delays 1 ms at the end
Colors wait for x,y,z ms to pass before decreasing/increasing their step
Colors set flag when they have all reached their projected value causing while loop to exit
(color flags used to account for the error in truncating the delay time per color)
*/

#include <Wire.h>
//#include <IRremote.h>
#include <EEPROM.h>
#include <Adafruit_PWMServoDriver.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define RED 0
#define GREEN 1
#define BLUE 2

byte hexToInt(char hexChar);


int currentRED = 1000, currentBLUE = 1000, currentGREEN = 1000;
int sceneRED = 4095, sceneBLUE = 4095, sceneGREEN = 4095;
int fadeTIME = 7;
int hex_convert_result = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x0D, 0x14, 0xD9 };
IPAddress ip(192, 168, 0, 177);

unsigned int localPort = 8888;      // local port to listen on

char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
////////////////////////////////////////////////////////////////////////////////////////////////////////////


EthernetUDP Udp;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void setup()
{
        Serial.begin(19200);
        
        Ethernet.begin(mac,ip);
        Udp.begin(localPort);
        
        pwm.begin();
        pwm.setPWMFreq(1600); //PWM driver refresh rate (1600 is the max)
        
        uint8_t twbrbackup = TWBR; //save current settings for I2C i/o speed
        TWBR = 12; //put I2C into fast+ mode
        
        pwm.setPWM(RED, 0, currentRED); //Turn on the lights after a POR
        pwm.setPWM(GREEN, 0, currentGREEN); //Turn on the lights after a POR
        pwm.setPWM(BLUE, 0, currentBLUE); //Turn on the lights after a POR
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() 
{
        
     int packetSize = Udp.parsePacket();
     if(packetSize)
     {
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remote = Udp.remoteIP();
        for (int i =0; i < 4; i++)
        {
          Serial.print(remote[i], DEC);
          if (i < 3)
          {
            Serial.print(".");
          }
        }
        Serial.print(", port ");
        Serial.println(Udp.remotePort());

        // read the packet into packetBufffer
        Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
        Serial.println("Contents:");
        Serial.println(packetBuffer);
        
        /*
          packetBuffer will be in the format RRRGGGBBB where: 
          RRR is the 3 digit hex value (12 bits) for the red color
          GGG is the 3 digit hex value (12 bits) for the green color, 
          BBB the 3 digit hex value (12 bits) for the blue color
        */
         

 
        hex_convert_result = 0;
        hex_convert_result = (hexToInt(packetBuffer[0]) << 8);
        hex_convert_result += (hexToInt(packetBuffer[1]) << 4);
        hex_convert_result += hexToInt(packetBuffer[2]);
        sceneRED = hex_convert_result;
        //Serial.println(sceneRED, HEX);
        //Serial.println(sceneRED, DEC);
        //Serial.println();
                
        hex_convert_result = 0;
        hex_convert_result = (hexToInt(packetBuffer[3]) << 8);
        hex_convert_result += (hexToInt(packetBuffer[4]) << 4);
        hex_convert_result += hexToInt(packetBuffer[5]);
        sceneGREEN = hex_convert_result;
        //Serial.println(sceneGREEN, HEX);
        //Serial.println(sceneGREEN, DEC);
        //Serial.println();
                
        hex_convert_result = 0;
        hex_convert_result = (hexToInt(packetBuffer[6]) << 8);
        hex_convert_result += (hexToInt(packetBuffer[7]) << 4);
        hex_convert_result += hexToInt(packetBuffer[8]);
        sceneBLUE = hex_convert_result;
        //Serial.println(sceneBLUE, HEX);
        //Serial.println(sceneBLUE, DEC);
        //Serial.println();

        fadeCALL();
    
        }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte hexToInt(char hexChar)
{
    //Serial.println("hexToInt Called");
    //Serial.println(hexChar);
    if (hexChar >= '0' && hexChar <= '9') 
    {
        hexChar = hexChar - '0';
        //Serial.println(hexChar, DEC);
        return hexChar;
    } 
        else if (hexChar >= 'a' && hexChar <= 'f') 
               {
                  hexChar = hexChar - 'a' + 10;
                  //Serial.println(hexChar, DEC);
                  return hexChar;
               } 
        else if (hexChar >= 'A' && hexChar <= 'F') 
                      {
                        hexChar = hexChar - 'A' + 10;
                        //Serial.println(hexChar, DEC);
                        return hexChar;
                      } 
  
}


void fadeCALL(void)
{
  
        int fadeDIFFred=0, fadeDIFFgreen=0, fadeDIFFblue=0, i=0;
        int fadeSTEPred=0, fadeSTEPgreen=0, fadeSTEPblue=0, redCOUNTER = 0, greenCOUNTER = 0, blueCOUNTER = 0;
        byte fadeFLAG = 0, redSIGN=0, greenSIGN=0, blueSIGN=0, redFLAG = 0, greenFLAG = 0, blueFLAG = 0, pwrONflag = 0, pwrOFFflag = 0;
        int stepREG = 1000;
 
 
        //fade from current scene to new scene
        fadeDIFFred = sceneRED - currentRED; //calculate the difference between the new and old
        fadeDIFFgreen = sceneGREEN - currentGREEN;
        fadeDIFFblue = sceneBLUE - currentBLUE;
   
        if(fadeDIFFred != 0)
        {
                fadeSTEPred = stepREG / abs(fadeDIFFred); //calculate the step size needed to fade within the fade time
        }
   
        if(fadeDIFFgreen != 0)
        {
                fadeSTEPgreen = stepREG / abs(fadeDIFFgreen);
        }
   
        if(fadeDIFFblue != 0)
        {
                fadeSTEPblue = stepREG / abs(fadeDIFFblue);
        }
   
        redSIGN = (fadeDIFFred < 0) ? 1 : 2; //figure out if we need to fade up or fade down to the new value
        greenSIGN = (fadeDIFFgreen < 0) ? 1 : 2;
        blueSIGN = (fadeDIFFblue < 0) ? 1 : 2;
 

        while(!fadeFLAG)
        {
     
                if(currentRED - sceneRED == 0)
                        redFLAG = 1; //if RED is at its new value set the color flag
     
                if(redFLAG != 1) //if it hasn't reached its new value yet, keep working on it!
                {
                        if(redCOUNTER == fadeSTEPred) //check to see if enough "steps" have elapsed
                        {
                                if(redSIGN == 1) //we need to dim the color
                                {
                                        currentRED -= 1; 
                                }
                                else //we need to increase the color
                                {
                                        currentRED += 1; 
                                }
                                pwm.setPWM(RED, 0, currentRED); //update the hue
                                redCOUNTER = 0; //reset the counter
                        }
                        else
                        {
                                redCOUNTER++; //increase the steps if enough time hasn't elapsed yet...
                        }
                }     
     
                //Green color
                if(currentGREEN - sceneGREEN == 0)
                        greenFLAG = 1;
     
                if(greenFLAG != 1)
                {
                       if(greenCOUNTER == fadeSTEPgreen)
                        {
                                if(greenSIGN == 1)
                                {
                                        currentGREEN -= 1; 
                                }
                                else
                                {
                                        currentGREEN += 1; 
                                }
                 
                                pwm.setPWM(GREEN, 0, currentGREEN);
                                greenCOUNTER = 0;
                        }
                        else
                        {
                                greenCOUNTER++; 
                        }
                }
          
          
                //Blue color
                if(currentBLUE - sceneBLUE == 0)
                       blueFLAG = 1;
     
                if(blueFLAG != 1)
                {
                        if(blueCOUNTER == fadeSTEPblue)
                        {
                                if(blueSIGN == 1)
                                {
                                        currentBLUE -= 1; 
                                }
                                else
                                {
                                        currentBLUE += 1; 
                                }
                                pwm.setPWM(BLUE, 0, currentBLUE);
                        blueCOUNTER = 0;
                        }
                        else
                        {
                                blueCOUNTER++; 
                        }
                }     
     
                if((redFLAG+greenFLAG+blueFLAG) == 3) //see if all 3 colors have reached their new hue
                        fadeFLAG = 1; //if so set the flag to exit the loop!
     

                delay(1);
        }
 
        fadeFLAG = 0;
        redFLAG = 0;
        greenFLAG = 0;
        blueFLAG = 0;
        pwrONflag = 0;
        pwrOFFflag = 0;
   
}

