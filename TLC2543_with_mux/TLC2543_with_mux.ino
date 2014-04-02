#define MUXE  4 //MUX E pin 4
#define MUXS0 5 //MUX S0 pin 5
#define MUXS1 6 //MUX S1 pin 6
#define MUXS2 7 //MUX S2 pin 7

#define TLCselpin 10 //Chip Select
#define TLCdataout 11//MOSI 
#define TLCdatain  12//MISO 
#define TLCclock  13//Clock 

#include "Arduino.h"
void setup();
void loop();
int read_adc(int channel);
int rv[11];
int voltage_test_div, voltage_test_pos, voltage_test_neg; //used to test chip internal voltage
int time = 10; //used to keep the uC from throwing out clock pulses too fast :)


void setup(){ 
/******************
Set Pin directions
******************/
 pinMode(TLCselpin, OUTPUT); 
 pinMode(TLCdataout, OUTPUT); 
 pinMode(TLCdatain, INPUT); 
 pinMode(TLCclock, OUTPUT);
 
 pinMode(MUXS0, OUTPUT);
 pinMode(MUXS1, OUTPUT);
 pinMode(MUXS2, OUTPUT); 
 pinMode(MUXE, OUTPUT);
 
 /********************
Set Pin defaults
*********************/
 digitalWrite(MUXE, HIGH);
 digitalWrite(TLCselpin,HIGH); //makes sure the chip is not active!
 digitalWrite(TLCdataout,LOW); 
 digitalWrite(TLCclock,LOW); 
 digitalWrite(MUXE, LOW);

 Serial.begin(9600); //Start serial communication @ 9600 baud
} 

void loop() 
{ 
//byte mux;
byte port_shifted;

 for (int a = 0; a<=7; a++) 
  {
    	///////////////////////////////
    	
    	//mux = 0b00000000;
    	port_shifted = PORTD<<3; //clear out upper 3 bits
    	//PORTD = port_shifted>>3|(mux|(a<<5));
	PORTD = (port_shifted>>3)|(a<<5); //change MUX pin states
    	
    	//////////////////////////////
    	

	for (int j=0; j<=10; j++)
	{
		rv[j] = read_adc(j);
                delayMicroseconds(time); //gives the chip a chance to catch up for next conversion
	}
	
	Serial.println();
	Serial.print("Board:");
	Serial.print(a);
	Serial.print(" ");

	for (int k=0; k<=10; k++)
	{
		Serial.print(rv[k], DEC);
		Serial.print(" ");
	}
	delay(750); 
  } 
}

int read_adc(int channel)
{
  int adcvalue = 0;
  byte commandbits = 0b00000000; //set to all zeros to start with
  commandbits|=((channel)<<4); //shifts in the channel number to the first 4 bits of the byte
  
  digitalWrite(TLCselpin,LOW); //Select Chip for communication
  delayMicroseconds(time); //Used to keep chip from getting confused	
  
  for (int i=7; i>=0; i--) //shifts in the data to the TLC2543
  {		
  	digitalWrite(TLCdataout,(commandbits>>i)&1); //this is a tricky one! It shifts the commandbits so that the MSB is sent first and going down to the LSB
  	delayMicroseconds(time);
  	digitalWrite(TLCclock,HIGH);
	delayMicroseconds(time);
    	digitalWrite(TLCclock,LOW);    
        delayMicroseconds(time);  	
  }
    
    digitalWrite(TLCdataout, LOW); //sets TLCdataout low in preparation for sending 4 null bits
    delayMicroseconds(time);
    
   for (int p=4; p>=1; p--) //all this for loop does is cycle the clock 4 times to make the total data input 12 bits
   {
   	digitalWrite(TLCclock,HIGH);
   	delayMicroseconds(time);
   	digitalWrite(TLCclock,LOW);
   	delayMicroseconds(time);
   }
  
  for (int i=11; i>=0; i--) //and finally we have our data collection for loop
  {
    	adcvalue+=digitalRead(TLCdatain)<<i; //reads in the bit, shifts it over (starting w/ MSB first) then adds it to previous value
    	digitalWrite(TLCclock,HIGH);
        delayMicroseconds(time);
    	digitalWrite(TLCclock,LOW);
        delayMicroseconds(time);
        
  }
  
  digitalWrite(TLCselpin, HIGH); //turn off device
  delayMicroseconds(time);
  
  return adcvalue;
}



