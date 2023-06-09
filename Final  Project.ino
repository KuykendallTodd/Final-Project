// CPE 301 - Final Project
// Written By Todd Kuykendall
// Date - 4/9/2023
#include <dht.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#include <Wire.h>

//Clock
RTC_DS3231 rtc;
char t[32];
dht DHT;

#define DHT11_PIN 8
#define WRITE_HIGH_PB(pin_num)  *port_b |= (0x01 << pin_num); 
#define WRITE_LOW_PB(pin_num)  *port_b &= ~(0x01 << pin_num);
#define RDA 0x80
#define TBE 0x20 
#define STEPS 100 
// UART Pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
// GPIO Pointers
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;  
// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
//ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
byte in_char, new_char;
int currentTicks;
int timer_running;
//LCD PINS
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
//Stepper
Stepper stepper(STEPS, 46, 48, 50, 52);
int previous = 0;

void setup() {
  // put your setup code here, to run once:
  // setup the Timer for Normal Mode, with the TOV interrupt enabled
  setup_timer_regs();
  U0Init(9600);
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  adc_init();
  stepper.setSpeed(50);
  //Lights 7 = Red 5=Blue 6 = Yellow 4 = Green
  *ddr_b |= 0x80;
}

void loop() {
  //Clock
   DateTime now = rtc.now();
  sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year()); 
  // read the character
  in_char = getChar(); 
  int chk = DHT.read11(DHT11_PIN);
  // if it's the start character
  if(in_char == 'S' || in_char == 's')
  {
    Serial.print(F("System on: "));
    Serial.println(t);
    *port_b &= ~(0x01 << 4); //Green light turns off
    *port_b &= ~(0x01 << 6); //Yellow Light off
    while(new_char != 'Q' && new_char != 'q')
    {
        // Stepper 
        int val = analogRead(14);
        // sensor reading
        stepper.step(val - previous);
        previous = val;
        *port_b &= ~(0x01 << 2);
        *port_b &= ~(0x01 << 7); //Red Light off
        *port_b &= ~(0x01 << 5); //Red Light off
        *port_b |= (0x01 << 4); //Green Light
        int chk = DHT.read11(DHT11_PIN);
        lcd.display();
        delay(1000);
        // set up the LCD's number of columns and rows:
        lcd.begin(16, 2);
        // Print a message to the LCD.
        lcd.print("Temperature=");
        lcd.print(DHT.temperature);
        lcd.setCursor(0, 1);
        lcd.print("Humidity=");
        lcd.print(DHT.humidity);
        delay(1000);
        new_char = getChar();
        //Check Water level
        int water = adc_read(0);
        delay(1000);
        while(water < 20) // Error State
        {
          lcd.begin(16, 2);
          // Print a message to the LCD.
          lcd.print("Error: Low Water");
          *port_b |= (0x01 << 7); //Red light on
          *port_b &= ~(0x01 << 4); //Green Light off
          water = adc_read(0);
          delay(1000);
        }
        while(DHT.temperature  != 24)
        {
              DateTime now = rtc.now();
              sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year()); 
              //Turn fan on 
              chk = DHT.read11(DHT11_PIN);
              lcd.display();
              delay(1000);
              // set up the LCD's number of columns and rows:
              lcd.begin(16, 2);
              // Print a message to the LCD.
              lcd.print("Temperature=");
              lcd.print(DHT.temperature);
              lcd.setCursor(0, 1);
              lcd.print("Humidity=");
              lcd.print(DHT.humidity);
             delay(1000);
              *port_b |= (0x01 << 2);
              *port_b |= (0x01 << 5); //BLue light
              *port_b &= ~(0x01 << 4); //Green Light off
              // Stepper 
               int val = analogRead(14);
               // sensor reading
               stepper.step(val - previous);
              previous = val;
              Serial.print(F("Motor on: "));
              Serial.println(t);
              delay(5000);
        }
    }
  }
  else
  {
    //Disable State Green light Check water level
       chk = DHT.read11(DHT11_PIN);
        *port_b &= ~(0x01 << 2); // Fan
        *port_b |= (0x01 << 6); //Green Light
        *port_b &= ~(0x01 << 7); //Red Light off
        *port_b &= ~(0x01 << 5); //Blue Light off
        *port_b &= ~(0x01 << 4); //Green Light off
         lcd.clear();
        // Stepper 
        int val = analogRead(14);
        // sensor reading
        stepper.step(val - previous);
        previous = val;
  }
}

// Timer setup function
void setup_timer_regs()
{
  // setup the timer control registers
  *myTCCR1A= 0x00;
  *myTCCR1B= 0X00;
  *myTCCR1C= 0x00;
  
  // reset the TOV flag
  *myTIFR1 |= 0x01;
  
  // enable the TOV interrupt
  *myTIMSK1 = 1;
}

// TIMER OVERFLOW ISR
ISR(TIMER1_OVF_vect)
{
  // Stop the Timer
  *myTCCR1B &= 0xF8;
  // Load the Count
  *myTCNT1 =  (unsigned int) (65535 -  (unsigned long) (currentTicks));
  // Start the Timer
  *myTCCR1B |= 0b00000001;
  // if it's not the STOP amount
  if(currentTicks != 65535)
  {
    // XOR to toggle PB6
    *port_b ^= 0x40;
  }
}
/*
 * UART FUNCTIONS
 */
void U0Init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char kbhit()
{
  return *myUCSR0A & RDA;
}

unsigned char getChar()
{
  return *myUDR0;
}

void putChar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}
void adc_init()
{
  // setup the A register 
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}