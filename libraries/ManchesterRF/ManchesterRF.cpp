/*
This code is based on the Atmel Corporation Manchester
Coding Basics Application Note.

http://www.atmel.com/dyn/resources/prod_documents/doc9164.pdf

Quotes from the application note:

"Manchester coding states that there will always be a transition of the message signal
at the mid-point of the data bit frame.
What occurs at the bit edges depends on the state of the previous bit frame and
does not always produce a transition. A logical '1' is defined as a mid-point transition
from low to high and a '0' is a mid-point transition from high to low.

We use Timing Based Manchester Decode.
In this approach we will capture the time between each transition coming from the demodulation
circuit."

Timer 2 is used with a ATMega328. Timer 1 is used for a ATtiny85.

This code gives a basic data rate as 1200 bauds. In manchester encoding we send 1 0 for a data bit 0.
We send 0 1 for a data bit 1. This ensures an average over time of a fixed DC level in the TX/RX.
This is required by the ASK RF link system to ensure its correct operation.
The data rate is then 600 bits/s.
*/

#include "ManchesterRF.h"

#define MDEBUG 1


/*
  message is an array of bytes received by receiver in one burst
  structure of an message array, size is a number of bytes received
    [size][data][data]...[data][undefined][undefined] ...[undefined]
    [ 0  ][ 1  ][ 2  ]...[size][ size + 1][ size + 2] ...[MAN_MESSAGE_SIZE-1]


  receive buffer consist of a ring buffer of messages
  [   0   ][   1   ][   2   ][   3   ]...[MAN_BUF_SIZE-1]
  [message][message][message][message]...[message]
               ^        ^        ^
  working -----+        |        |      place where client will read current message content from
                        |        |
  man_rx_buff_start ----+        |      place of the next message, isDataAvailable() will look there
                                 |
            man_rx_buff_end -----+      place where the new message is being received


  The circular buffer is implemented with one slot open, that slot is read from the main program

*/

// MAN_BUF_SIZE must be 2 or more based on available microcontroller memory

#define MAN_MESSAGE_SIZE 16
#if defined( __AVR_ATtinyX5__ )
  #define MAN_BUF_SIZE 2
#elif defined( __AVR_ATtiny84__ )
  #define MAN_BUF_SIZE 2
#elif defined( __AVR_ATmega328P__ )
  #define MAN_BUF_SIZE 4
#elif defined( __AVR_ATmega1284P__ )
  #define MAN_BUF_SIZE 8
#else
  #define MAN_BUF_SIZE 4
#endif

#define MAN_IS_BUFF_EMPTY (::man_rx_buff_end == ::man_rx_buff_start)
#define MAN_IS_BUFF_FULL ((::man_rx_buff_end+1) % MAN_BUF_SIZE == ::man_rx_buff_start)

//#define MAN_ADD_TO_CHECKSUM(sum, data) ((uint8_t(sum*sum)) ^ data)

uint8_t man_rx_buff[MAN_BUF_SIZE][MAN_MESSAGE_SIZE];
volatile uint8_t man_rx_buff_start = 0;
volatile uint8_t man_rx_buff_end = 0;

uint8_t man_tx_buff[MAN_MESSAGE_SIZE];

static uint8_t RxPin = 255;
static uint8_t directRxPort = 0x00;
static uint8_t directRxMask = 0x00;

static uint8_t directDebugPort = 0x00;
static uint8_t directDebugMask = 0x00;

static uint8_t rx_sample = 0;
static uint8_t rx_last_sample = 0;
static uint8_t rx_pulse_width = 0;
static uint8_t rx_pulse_width_inc = 8;
static uint8_t rx_sync_count = 0;
static uint8_t rx_mode = RX_MODE_IDLE;

static uint16_t rx_manBits = 0; //the received manchester 32 bits
static uint8_t rx_numMB = 0; //the number of received manchester bits
static uint8_t rx_curByte = 0;

//static uint8_t rx_maxBytes = 2;
//static uint8_t rx_default_data[2];
//static uint8_t* rx_data = rx_default_data;

uint8_t sranie[256];
uint8_t isranie = 0;



//global functions

//calculates integer logarithm base 2
uint8_t MAN_log2(uint8_t a) {
  uint8_t r = 0;
  while (a >>= 1) r++;
  return r;
}


inline void DEBUG_TOGGLE() {
  #if defined (MDEBUG)
    if (::directDebugPort && ::directDebugMask) { //use direct port manipulation (much faster)
      //toggle pin
      switch(::directDebugPort) {
        #if defined(PINA)
          case 1: PINA |= ::directDebugMask; break;
        #endif
        #if defined(PINB)
          case 2: PINB |= ::directDebugMask; break;
        #endif
        #if defined(PINC)
          case 3: PINC |= ::directDebugMask; break;
        #endif
        #if defined(PIND)
          case 4: PIND |= ::directDebugMask; break;
        #endif
        default:;
      }
    }
  #endif
}

ManchesterRF::ManchesterRF(uint8_t SF) :
	speedFactor(SF),
	delay10(0),
	delay20(0),
	delay11(0),
	delay21(0),
	TxPin(0),
	directTxMask(0),
	directTxPort(0),
	balanceFactor(0)
{
	//debug
	//pinMode(7, OUTPUT);
}

void ManchesterRF::pin2PortMask(uint8_t pin, uint8_t &port, uint8_t &mask) {
  mask = 0;
  port = 0;
  #if defined( __AVR_ATtinyX5__ )
    if (pin < 8) {
      mask = _BV(pin);
      port = 2;
    }
  #elif defined( __AVR_ATtiny84__ )
    if (pin <= 10 && pin >= 3) {
      mask = _BV(10 - pin);
      port = 1;
    } else if (pin < 3) {
      mask = _BV(pin);
      port = 2;
    }
  #elif defined( __AVR_ATmega328P__ )
    if (pin < 8) {
      mask = _BV(pin);
      port = 4;
    } else if (pin <= 13) {
      mask = _BV(pin - 8);
      port = 2;
    } else {
      mask = _BV(pin - A0);
      port = 3;
    }
  #elif defined( __AVR_ATmega1284P__ )
    if (pin < 8) {
      mask = _BV(pin);
    }
  #endif
}

/**************************  TRANSMIT INIT ****************************/

void ManchesterRF::setBalance(int8_t bf) {
  this->balanceFactor = bf;
}

void ManchesterRF::TXInit() {

//  balanceFactor = bf;
//  speedFactor = SF;

  //emprirically determined values to compensate for the time loss in overhead

  #if F_CPU < 8000000UL
    uint16_t compensationFactor = 48; //24;//40;
  #elif F_CPU < 16000000UL
    uint16_t compensationFactor = 6;
  #else //16000000Mhz
    uint16_t compensationFactor = 0;
  #endif

  #if F_CPU < 8000000UL
    uint16_t compensationFactor2 = 0;
  #elif F_CPU < 16000000UL
    uint16_t compensationFactor2 = 0;
  #else //16000000Mhz
    uint16_t compensationFactor2 = 0;
  #endif

  /*
  Base delay | speed factor
  3072 - 0
  1536 - 1
  768  - 2
  384  - 3
  192  - 4
  96   - 5
  48   - 6
  24   - 7
  12   - 8

  */

  //this must be signed int
  int temp10 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor + balanceFactor;
  int temp11 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor - balanceFactor;

  int temp20 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor2 + balanceFactor;
  int temp21 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor2 - balanceFactor;

  if (temp10 < 0) {
    temp21 += temp10; //borrow from second delay to maintain constant speed
    temp10 = 0;
  }

  if (temp11 < 0) {
    temp20 += temp11; //borrow from second delay to maintain constant speed
    temp11 = 0;
  }

  if (temp21 < 0) temp21 = 0; //too slow for such speed
  if (temp20 < 0) temp20 = 0; //too slow for such speed

  delay10 = temp10;
  delay11 = temp11;
  delay20 = temp20;
  delay21 = temp21;


//  if ((HALF_BIT_INTERVAL >> speedFactor) <= compensationFactor) delay1 = 0; //oops, we are too slow for such speeds
//  else delay1 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor;
//  delay2 = (HALF_BIT_INTERVAL >> speedFactor); // - 2;


  //delay1 = delay2 = (HALF_BIT_INTERVAL >> speedFactor);
}


void ManchesterRF::TXInit(uint8_t pin) {
  this->TxPin = pin;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  this->pin2PortMask(pin, this->directTxPort, this->directTxMask);
  this->TXInit();
}


void ManchesterRF::TXInit(uint8_t port, uint8_t mask) {
  //set pin as output and low
  switch(port) {
    #if defined(PORTA)
      case 1: DDRA |= mask; PORTA &= ~mask; break;
    #endif
    #if defined(PORTB)
      case 2: DDRB |= mask; PORTB &= ~mask; break;
    #endif
    #if defined(PORTC)
      case 3: DDRC |= mask; PORTC &= ~mask; break;
    #endif
    #if defined(PORTD)
      case 4: DDRD |= mask; PORTD &= ~mask; break;
    #endif
    default:;
  }
  this->directTxPort = port;
  this->directTxMask = mask;
  this->TXInit();
}


/***********************  RECEIVER INIT *************************/

void ManchesterRF::RXInit() {

  //setup timers depending on the microcontroller used

  #if defined( __AVR_ATtinyX5__ )

    /*
    Timer 1 is used with a ATtiny85.
    http://www.atmel.com/Images/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf page 88
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1C is 8 bit register
    */

    if (0) {

    #if F_CPU == 1000000UL
			if (speedFactor == 1) { //preserve PWM

			} else {
				TCCR1 = _BV(CTC1) | _BV(CS12); // 1/8 prescaler
				OCR1C = (64 >> speedFactor) - 1;
			}
    #elif F_CPU == 8000000UL
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (64 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (128 >> speedFactor) - 1;
    #elif F_CPU == 16500000UL
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (132 >> speedFactor) - 1;
    #else
    #error "Manchester library only supports 1mhz, 8mhz, 16mhz, 16.5Mhz clock speeds on ATtiny85 chip"
    #endif

    OCR1A = 0; // Trigger interrupt when TCNT1 is reset to 0
    TIMSK |= _BV(OCIE1A); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

    } else { //new

    unsigned long f = F_CPU >> (speedFactor);
//    unsigned int base_pulse_inc =  1 * 256 * 16 / (F_CPU / 1000000);

    if (f < 500000) { //processer not fast enough, increase pulse width instead
      TCCR1 &= B11110000; //clear old prescaller
      TCCR1 |= _BV(CS10); //set new prescaller to 1
      rx_pulse_width_inc = 16;
      if (f < 250000) rx_pulse_width_inc = 32;
      if (f < 125000) rx_pulse_width_inc = 64;

    } else {
      //timer 1 has only fast PWM
      byte prescaller = (f/500000);
      TCCR1 &= B11110000; //clear old prescaller
      TCCR1 |= B00001111 & (0 + MAN_log2(prescaller)); //set new prescaller
      rx_pulse_width_inc = 4;
    }
    //TIMSK is shared with timer 0 and timer 1
    TIMSK |= _BV(TOIE1); // Turn on interrupt on overflow

    } //new

/*

    unsigned long f = F_CPU >> (speedFactor);
    unsigned int base_pulse_inc =  1 * 256 * 16 / (F_CPU / 1000000);

    if (f < 1000000) {
        TCCR2B = _BV(CS20); // 1/1 prescaler
        TCCR2A = _BV(WGM21) | _BV(WGM20); //fast PWM
        rx_pulse_width_inc = ((8 * 500000/f) * base_pulse_inc) >> 8;
    } else if (f < 4000000) {
        TCCR2B = _BV(CS20); // 1/1 prescaler
        TCCR2A = _BV(WGM20); //phase correct pwm
        rx_pulse_width_inc = ((8 * 1000000/f)* base_pulse_inc) >> 8;
    } else if (f < 8000000) {
        TCCR2B = _BV(CS21); // 1/8 prescaler
        TCCR2A = _BV(WGM21) | _BV(WGM20); //fast PWM
        rx_pulse_width_inc = (8 * base_pulse_inc) >> 8;
    } else {
        TCCR2B = _BV(CS21); // 1/8 prescaler
        TCCR2A = _BV(WGM20); //phase correct pwm
        rx_pulse_width_inc = ((8 * 8000000/f)* base_pulse_inc) >> 8;
    }

    TIMSK2 = _BV(TOIE2); // Turn on interrupt

*/


  #elif defined( __AVR_ATtiny84__ )

    /*
    Timer 1 is used with a ATtiny84.
    http://www.atmel.com/Images/doc8006.pdf page 111
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1A is 8 bit register
    */

    if (0) { //old
    #if F_CPU == 1000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11); // 1/8 prescaler
      OCR1A = (64 >> speedFactor) - 1;
    #elif F_CPU == 8000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1A = (64 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1A = (128 >> speedFactor) - 1;
    #else
    #error "Manchester library only supports 1mhz, 8mhz, 16mhz on ATtiny84"
    #endif

    TIMSK1 |= _BV(OCIE1A); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

    } else { //new PWM preserving

      //TIMER 1 and TIMER 0 share the same prescaller, changing the prescaller will effect millis()
      unsigned long f = F_CPU >> (speedFactor);
      unsigned int base_pulse_inc =  1 * 256 * 16 / (F_CPU / 1000000);
/*
  F_CPU 1Mhz
  speedFactor | f
  0  -  1
  1  -  0.5
  2  -  0.25
  3  -  0.125

  F_CPU 8Mhz
  speedFactor | f
  0  -  8
  1  -  4
  2  -  2
  3  -  1
  4  -  0.5
  5  -  0.25

  F_CPU 16Mhz
  speedFactor | f
  0  -  16
  1  -  8
  2  -  4
  3  -  2
  4  -  1
  5  -  0.5
*/


      if (f < 2000000) {
        TCCR1B &= B11111000; //clear the old prescaller
        TCCR1B |= _BV(CS10); //set new prescaller to 1
        //set fast PWM 8bit
        TCCR1B &= B11100111; //clear old value
        TCCR1A &= B11111100; //clear old value
        TCCR1B |= _BV(WGM12); //set new value
        TCCR1A |= _BV(WGM10); //set new value
        rx_pulse_width_inc = 8 * 500000/f;
      } else if (f < 4000000) {
        TCCR1B &= B11111000; //clear the old prescaller
        TCCR1B |= _BV(CS10); //set new prescaller to 1
        //phase correct PWM 8bit
        TCCR1B &= B11100111; //clear old value
        TCCR1A &= B11111100; //clear old value
        TCCR1A |= _BV(WGM10); //set new value
        rx_pulse_width_inc = 8 * 1000000/f;
      } else if (f < 8000000) {
        TCCR1B &= B11111000; //clear the old prescaller
        TCCR1B |= _BV(CS11); //set new prescaller to 8
        //set fast PWM 8bit
        TCCR1B &= B11100111; //clear old value
        TCCR1A &= B11111100; //clear old value
        TCCR1B |= _BV(WGM12); //set new value
        TCCR1A |= _BV(WGM10); //set new value
        rx_pulse_width_inc = 8;
      } else {
        TCCR1B &= B11111000; //clear the old prescaller
        TCCR1B |= _BV(CS11); //set new prescaller to 8
        //phase correct PWM 8bit
        TCCR1B &= B11100111; //clear old value
        TCCR1A &= B11111100; //clear old value
        TCCR1A |= _BV(WGM10); //set new value
        rx_pulse_width_inc = 8 * 8000000/f;
      }

      TIMSK1 = _BV(TOIE1); // Turn on interrupt
    }
  #elif defined(__AVR_ATmega32U4__)

    /*
    Timer 3 is used with a ATMega32U4.
    http://www.atmel.com/Images/doc7766.pdf page 133
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR3A is 16 bit register
    */

    TCCR3B = _BV(WGM32) | _BV(CS31); // 1/8 prescaler
    #if F_CPU == 1000000UL
      OCR3A = (64 >> speedFactor) - 1;
    #elif F_CPU == 8000000UL
      OCR3A = (512 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      OCR3A = (1024 >> speedFactor) - 1;
    #else
    #error "Manchester library only supports 1mhz, 8mhz, 16mhz on ATMega32U4"
    #endif

    TCCR3A = 0; // reset counter on match
    TIFR3 = _BV(OCF3A); // clear interrupt flag
    TIMSK3 = _BV(OCIE3A); // Turn on interrupt
    TCNT3 = 0; // Set counter to 0

  #elif defined(__AVR_ATmega8__)

    /*
    Timer/counter 1 is used with ATmega8.
    http://www.atmel.com/Images/Atmel-2486-8-bit-AVR-microcontroller-ATmega8_L_datasheet.pdf page 99
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1A is 16 bit register
    */

    TCCR1A = _BV(WGM12); // reset counter on match
    TCCR1B =  _BV(CS11); // 1/8 prescaler
    #if F_CPU == 1000000UL
      OCR1A = (64 >> speedFactor) - 1;
    #elif F_CPU == 8000000UL
      OCR1A = (512 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      OCR1A = (1024 >> speedFactor) - 1;
    #else
    #error "Manchester library only supports 1Mhz, 8mhz, 16mhz on ATMega8"
    #endif
    TIFR = _BV(OCF1A);  // clear interrupt flag
    TIMSK = _BV(OCIE1A); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

  #elif defined(__AVR_ATmega328P__)

    /*
    Timer 2 is used with a ATMega328.
    pins 11 and 3 are connected to Timer2
    http://www.atmel.com/dyn/resources/prod_documents/doc8161.pdf page 162
    */

/*
  F_CPU 1Mhz
  speedFactor | f
  0  -  1
  1  -  0.5
  2  -  0.25
  3  -  0.125

  F_CPU 8Mhz
  speedFactor | f
  0  -  8
  1  -  4
  2  -  2
  3  -  1
  4  -  0.5
  5  -  0.25

  F_CPU 16Mhz
  speedFactor | f
  0  -  16
  1  -  8
  2  -  4
  3  -  2
  4  -  1
  5  -  0.5
*/

    unsigned long f = F_CPU >> (speedFactor);
    unsigned int base_pulse_inc =  1 * 256 * 16 / (F_CPU / 1000000);

    if (f < 2000000) {
        TCCR2B = _BV(CS20); // 1/1 prescaler
        TCCR2A = _BV(WGM21) | _BV(WGM20); //fast PWM
        rx_pulse_width_inc = ((8 * 500000/f) * base_pulse_inc) >> 8;
    } else if (f < 4000000) {
        TCCR2B = _BV(CS20); // 1/1 prescaler
        TCCR2A = _BV(WGM20); //phase correct pwm
        rx_pulse_width_inc = ((8 * 1000000/f)* base_pulse_inc) >> 8;
//    } else if (f < 8000000) {
//        TCCR2B = _BV(CS21); // 1/8 prescaler
//        TCCR2A = _BV(WGM21) | _BV(WGM20); //fast PWM
//        rx_pulse_width_inc = (8 * base_pulse_inc) >> 8;
    } else {
        TCCR2B = _BV(CS21); // 1/8 prescaler
        TCCR2A = _BV(WGM20); //phase correct pwm
        rx_pulse_width_inc = ((8 * 8000000/f)* base_pulse_inc) >> 8;
    }

    TIMSK2 = _BV(TOIE2); // Turn on interrupt


  #elif defined(__AVR_ATmega1284P__)

    /*
    Timer 3 is used on ATMega1284, Timer3 controlls PWM on SPI pins
    http://www.atmel.com/images/doc8059.pdf page 134
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR3A is 16 bit register
    */

    if (0) { //old method

    TCCR3A = _BV(WGM32); // reset counter on match
    #if F_CPU == 1000000UL
      TCCR3B = _BV(CS30); // 1/1 prescaler
      OCR3A = (512 >> speedFactor) - 1;
    #elif F_CPU == 8000000UL
      TCCR3B = _BV(CS31); // 1/8 prescaler
      OCR3A = (512 >> speedFactor) - 1;
    #elif F_CPU == 12000000UL
      TCCR3B = _BV(CS31); // 1/8 prescaler
      OCR3A = (768 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      TCCR3B = _BV(CS31); // 1/8 prescaler
      OCR3A = (1024 >> speedFactor) - 1;
    #elif F_CPU == 20000000UL
      TCCR3B = _BV(CS31); // 1/8 prescaler
      OCR3A = (1280 >> speedFactor) - 1;
    #elif F_CPU == 24000000UL
      TCCR3B = _BV(CS31); // 1/8 prescaler
      OCR3A = (1536 >> speedFactor) - 1;
    #else
			#error "Manchester library only supports 1Mhz, 8Mhz, 12Mhz, 16Mhz, 20Mhz, 24Mhz on ATMega1284"
    #endif
    TIMSK3 = _BV(OCIE3A); // Turn on interrupt
    TCNT3 = 0; // Set counter to 0

    } else { //new PWM preserving method

      unsigned long f = F_CPU >> (speedFactor);
      unsigned int base_pulse_inc =  1 * 256 * 16 / (F_CPU / 1000000);

      if (f < 1000000) {
        TCCR3B &= B11111000; //clear the prescaler
        TCCR3B |= _BV(CS30); //prescaler 1
        //fast 8 bit PWM
        TCCR3A &= B11111100; //clear old value
        TCCR3B &= B11100111; //clear old value
        TCCR3A |= _BV(WGM30);
        TCCR3B |= _BV(WGM32);
        rx_pulse_width_inc = 8; //((8 * 500000/f) * base_pulse_inc) >> 8;
        if (f < 500000) { rx_pulse_width_inc = 13; }
      } else if (f < 4000000) { //assume 16Mhz, (sF = 4, f=1M) (sF = 3, f=2M)
        TCCR3B &= B11111000; //clear the prescaler
        TCCR3B |= _BV(CS30); //prescaler 1
        //phase correct 8 bit PWM
        TCCR3A &= B11111100; //clear old value
        TCCR3B &= B11100111; //clear old value
        TCCR3A |= _BV(WGM30);
        //(128 << speedFactor) / (Mhz�)
        rx_pulse_width_inc = 8; //((8 * 1000000/f)* base_pulse_inc) >> 8; //only 6 and 7 works on 20MHz
      } else if (f < 8000000) {
        TCCR3B &= B11111000; //clear the prescaler
        TCCR3B |= _BV(CS31); //prescaler 8
        //fast 8 bit PWM
        TCCR3A &= B11111100; //clear old value
        TCCR3B &= B11100111; //clear old value
        TCCR3A |= _BV(WGM30);
        TCCR3B |= _BV(WGM32);
        rx_pulse_width_inc = (8 * base_pulse_inc) >> 8;
      } else {
        TCCR3B &= B11111000; //clear the prescaler
        TCCR3B |= _BV(CS31); //prescaler 8
        //phase correct 8 bit PWM
        TCCR3A &= B11111100; //clear old value
        TCCR3B &= B11100111; //clear old value
        TCCR3A |= _BV(WGM30);
        rx_pulse_width_inc = ((8 * 8000000/f)* base_pulse_inc) >> 8;
      }

      TIMSK3 = _BV(TOIE3); // Turn on interrupt



    }

  #else
		#error "Manchester library doesnt support your microcontroller"
  #endif

    ::rx_mode = RX_MODE_PRE;
}


void ManchesterRF::RXInit(uint8_t pin) {
  ::RxPin = pin;
  pinMode(pin, INPUT);
  this->pin2PortMask(pin, ::directRxPort, ::directRxMask);
  this->RXInit();
}


void ManchesterRF::RXInit(uint8_t port, uint8_t mask) {
  ::directRxPort = port;
  ::directRxMask = mask;
  this->RXInit();
}

void ManchesterRF::setDebugPortMask(uint8_t port, uint8_t mask) {
  ::directDebugPort = port;
  ::directDebugMask = mask;
}


/*
void ManchesterRF::setup(uint8_t Tpin, uint8_t Rpin, uint8_t SF)
{
  setupTransmit(Tpin, SF);
  setupReceive(Rpin, SF);
}
*/

/*
void ManchesterRF::transmit(uint16_t data)
{
  uint8_t byteData[2] = {data >> 8, data & 0xFF};
  transmitArray(2, byteData);
}
*/


size_t ManchesterRF::write(uint8_t value) {
  //TODO

  return 1;
}



void ManchesterRF::sendZero(void) {
  if (this->directTxPort && this->directTxMask) { //use direct port manipulation (much faster)
    delayMicroseconds(delay11);
    //go HIGH
    switch(this->directTxPort) {
      #if defined(PORTA)
        case 1: PORTA |= this->directTxMask; break;
      #endif
      #if defined(PORTB)
        case 2: PORTB |= this->directTxMask; break;
      #endif
      #if defined(PORTC)
        case 3: PORTC |= this->directTxMask; break;
      #endif
      #if defined(PORTD)
        case 4: PORTD |= this->directTxMask; break;
      #endif
      default:;
    }
    delayMicroseconds(delay20);

    //go LOW
    switch(this->directTxPort) {
      #if defined(PORTA)
        case 1: PORTA &= ~this->directTxMask; break;
      #endif
      #if defined(PORTB)
        case 2: PORTB &= ~this->directTxMask; break;
      #endif
      #if defined(PORTC)
        case 3: PORTC &= ~this->directTxMask; break;
      #endif
      #if defined(PORTD)
        case 4: PORTD &= ~this->directTxMask; break;
      #endif
      default:;
    }
  } else {
    delayMicroseconds(delay11);
    digitalWrite(TxPin, HIGH);

    delayMicroseconds(delay20);
    digitalWrite(TxPin, LOW);
  }
}//end of send a zero


void ManchesterRF::sendOne(void) {
  if (directTxMask) { //use direct port manipulation (much faster)
    delayMicroseconds(delay10);
    //go LOW
    switch(this->directTxPort) {
      #if defined(PORTA)
        case 1: PORTA &= ~this->directTxMask; break;
      #endif
      #if defined(PORTB)
        case 2: PORTB &= ~this->directTxMask; break;
      #endif
      #if defined(PORTC)
        case 3: PORTC &= ~this->directTxMask; break;
      #endif
      #if defined(PORTD)
        case 4: PORTD &= ~this->directTxMask; break;
      #endif
      default:;
    }

    delayMicroseconds(delay21);
    //go HIGH
    switch(this->directTxPort) {
      #if defined(PORTA)
        case 1: PORTA |= this->directTxMask; break;
      #endif
      #if defined(PORTB)
        case 2: PORTB |= this->directTxMask; break;
      #endif
      #if defined(PORTC)
        case 3: PORTC |= this->directTxMask; break;
      #endif
      #if defined(PORTD)
        case 4: PORTD |= this->directTxMask; break;
      #endif
      default:;
    }
  } else {
    delayMicroseconds(delay10);
    digitalWrite(TxPin, LOW);

    delayMicroseconds(delay21);
    digitalWrite(TxPin, HIGH);
  }
}//end of send one


/*
The 433.92 Mhz receivers have AGC, if no signal is present the gain will be set
to its highest level.

In this condition it will switch high to low at random intervals due to input noise.
A CRO connected to the data line looks like 433.92 is full of transmissions.

Any ASK transmission method must first sent a capture signal of 101010........
When the receiver has adjusted its AGC to the required level for the transmisssion
the actual data transmission can occur.

We send 14 0's 1010... It takes 1 to 3 10's for the receiver to adjust to
the transmit level.

The receiver waits until we have at least 10 10's and then a start pulse 01.
The receiver is then operating correctly and we have locked onto the transmission.
*/
uint8_t ManchesterRF::transmitArray(uint8_t size, uint8_t *data) {
  if (size == 0) return 0;

  #if F_CPU < 88000000UL
    char cSREG;
    cSREG = SREG; /* store SREG value */
    cli();
  #endif

  //send preamble
  for( uint8_t i = 0; i < 14; i++) {
    sendZero();
    //match the overhead in the data cycle, I have to do it this way, so optimizer won't mess with it
    #if F_CPU < 88000000UL
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
      __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    #endif
	}

  // Send a single 1
  sendOne(); //start data pulse


  //match the overhead in the data cycle, I have to do it this way, so optimizer won't mess with it
  #if F_CPU < 88000000UL
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
    __asm__ __volatile__ ("mov r0, r0"); //wait one cycle
  #endif

  // Send the user data

  for (uint8_t i = 0; i < size; i++) {
    uint8_t mask = 0x01; //mask to send bits
    //uint8_t d = data[i] ^ DECOUPLING_MASK;
    for (uint8_t j = 0; j < 8; j++) {
      if (((data[i] ^ DECOUPLING_MASK) & mask) == 0)
        sendZero();
      else
        sendOne();
      mask <<= 1; //get next bit
    }//end of byte
  }//end of data

  // Send terminating 0 to correctly terminate the previous bit and to turn the transmitter off
  sendZero();
  sendZero();

  #if F_CPU < 88000000UL
    SREG = cSREG;
  #endif
  return size;
}//end of send the data


uint8_t ManchesterRF::receiveArray(uint8_t &size, uint8_t **data) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  size = man_rx_buff[man_rx_buff_start][0];
  *data = &man_rx_buff[man_rx_buff_start][1];
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  return 1;
}


//TODO use repairing codes perhabs?
//http://en.wikipedia.org/wiki/Hamming_code

/*
    format of the message including checksum and ID

    [0][1][2][3][4][5][6][7][8][9][a][b][c][d][e][f]
    [    ID    ][ checksum ][         data         ]
                  checksum = ID xor data[7:4] xor data[3:0] xor 0b0011

*/

/*
//decode 8 bit payload and 4 bit ID from the message, return true if checksum is correct, otherwise false
uint8_t ManchesterRF::decodeMessage(uint16_t m, uint8_t &id, uint8_t &data)
{
  //extract components
  data = (m & 0xFF);
  id = (m >> 12);
  uint8_t ch = (m >> 8) & 0b1111; //checksum received
  //calculate checksum
  uint8_t ech = (id ^ data ^ (data >> 4) ^ 0b0011) & 0b1111; //checksum expected
  return ch == ech;
}

//encode 8 bit payload, 4 bit ID and 4 bit checksum into 16 bit
uint16_t ManchesterRF::encodeMessage(uint8_t id, uint8_t data) {
  uint8_t chsum = (id ^ data ^ (data >> 4) ^ 0b0011) & 0b1111;
  uint16_t m = ((id) << 12) | (chsum << 8) | (data);
  return m;
}
*/



/*
#define MAN_IS_BUFF_EMPTY (man_rx_buff_end == man_rx_buff_start)
#define MAN_IS_BUFF_FULL ((man_rx_buff_end+1) % MAN_BUF_SIZE == man_rx_buff_start)

static uint8_t man_rx_buff[MAN_BUF_SIZE][MAN_MESSAGE_SIZE];
static uint8_t man_rx_buff_start = 0;
static uint8_t man_rx_buff_end = 0;
*/

/*
packet format
  [from][ to ][meta][payload][payload][payload] ... [checksum]

*/


uint8_t ManchesterRF::transmitPacket(uint8_t size, uint8_t from, uint8_t to, uint8_t meta, uint8_t *payload) {
  ::man_tx_buff[0] = from;
  ::man_tx_buff[1] = to;
  ::man_tx_buff[2] = meta;
//  ::man_tx_buff[3] = 111;
//  ::man_tx_buff[4] = 222;
  for (uint8_t i = 0; i < size && i < 10; i++) {
    ::man_tx_buff[i + 3] = payload[i];
  }

  //calculate the modification of a Fletcher checksum
  uint8_t c0 = size + 4;
  uint8_t c1 = size + 4;
  for(int i = 0; i < size + 3; i++) { //zeroth element is the size, it's not part of the packet, but we will add it as an extra check; sizeth element is the checksum we are calculating
    c0 += ::man_tx_buff[i];
    c1 += c0;
  }
  c1 ^= c0;
  ::man_tx_buff[size + 3] = c1;

  return this->transmitArray(size + 4, ::man_tx_buff);
}

uint8_t ManchesterRF::receivePacket(uint8_t &size, uint8_t &from, uint8_t &to, uint8_t &meta, uint8_t **payload) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  if (man_rx_buff[man_rx_buff_start][0] < 4) { //not enough bytes for a valid packet
    man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
    return 0;
  }
  uint8_t msize = man_rx_buff[man_rx_buff_start][0];
  size = msize - 4; //payload size
  from = man_rx_buff[man_rx_buff_start][1];
  to   = man_rx_buff[man_rx_buff_start][2];
  meta = man_rx_buff[man_rx_buff_start][3];
  *payload = &man_rx_buff[man_rx_buff_start][4];
  uint8_t ch = man_rx_buff[man_rx_buff_start][msize];

  //calculate the modification of a Fletcher checksum
  uint8_t c0 = 0;
  uint8_t c1 = 0;
  for(int i = 0; i < msize; i++) { //zeroth element is the size, it's not part of the packet, but we will add it as an extra check; sizeth element is the checksum we are calculating
    c0 += man_rx_buff[man_rx_buff_start][i];
    c1 += c0;
  }
  c1 ^= c0;
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  if (ch == c1) return 1;
  return 0;
}


uint8_t ManchesterRF::transmitByte(uint8_t data) {
  return this->transmitArray(1, &data);
}

uint8_t ManchesterRF::receiveByte(uint8_t &data) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  if (man_rx_buff[man_rx_buff_start][0] < 1) { //not enough bytes
    man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
    return 0;
  }
  data = man_rx_buff[man_rx_buff_start][1];
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  return 1;
}


uint8_t ManchesterRF::transmitByte(uint8_t data0, uint8_t data1) {
  ::man_tx_buff[0] = data0;
  ::man_tx_buff[1] = data1;
  return this->transmitArray(2, ::man_tx_buff);
}

uint8_t ManchesterRF::receiveByte(uint8_t &data0, uint8_t &data1) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  if (man_rx_buff[man_rx_buff_start][0] < 2) { //not enough bytes
    man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
    return 0;
  }
  data0 = man_rx_buff[man_rx_buff_start][1];
  data1 = man_rx_buff[man_rx_buff_start][2];
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  return 1;
}

uint8_t ManchesterRF::transmitByte(uint8_t data0, uint8_t data1, uint8_t data2) {
  ::man_tx_buff[0] = data0;
  ::man_tx_buff[1] = data1;
  ::man_tx_buff[2] = data2;
  return this->transmitArray(3, ::man_tx_buff);
}

uint8_t ManchesterRF::receiveByte(uint8_t &data0, uint8_t &data1, uint8_t &data2) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  if (man_rx_buff[man_rx_buff_start][0] < 3) { //not enough bytes
    man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
    return 0;
  }
  data0 = man_rx_buff[man_rx_buff_start][1];
  data1 = man_rx_buff[man_rx_buff_start][2];
  data2 = man_rx_buff[man_rx_buff_start][3];
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  return 1;
}


uint8_t ManchesterRF::transmitWord(uint16_t data) {
  return this->transmitArray(2, (uint8_t*)&data);
}


uint8_t ManchesterRF::receiveWord(uint16_t &data) {
  if (MAN_IS_BUFF_EMPTY) return 0;
  if (man_rx_buff[man_rx_buff_start][0] < 2) { //not enough bytes for a valid word
    man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
    return 0;
  }
  data = (man_rx_buff[man_rx_buff_start][1] << 8 ) + man_rx_buff[man_rx_buff_start][2];
  man_rx_buff_start = (man_rx_buff_start + 1) % MAN_BUF_SIZE; //remove message from the buffer
  return 1;
}

int ManchesterRF::available() {
  return !MAN_IS_BUFF_EMPTY;
}


int ManchesterRF::peek() { //TODO
  return 0;
}


int ManchesterRF::read() { //TODO
  return 0;
}


void ManchesterRF::flush() { //TODO

}


/*
void ManchesterRF::beginReceiveArray(uint8_t maxBytes, uint8_t *data) {
  ::MANRX_BeginReceiveBytes(maxBytes, data);
}

*/

void ManchesterRF::beginReceive(void) {
  ::MANRX_BeginReceive();
}

/*
uint8_t ManchesterRF::receiveComplete(void) {
  return ::MANRX_ReceiveComplete();
}
*/

/*
uint16_t ManchesterRF::getMessage(void) {
  return ::MANRX_GetMessage();
}
*/

void ManchesterRF::stopReceive(void) {
  ::MANRX_StopReceive();
}




void MANRX_BeginReceive(void) {
//  rx_maxBytes = 2;
//  rx_data = rx_default_data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_BeginReceiveBytes(uint8_t maxBytes, uint8_t *data) {
//  rx_maxBytes = maxBytes;
//  rx_data = data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_StopReceive(void) {
  rx_mode = RX_MODE_IDLE;
}



//rx_manBits, &rx_numMB, &rx_curByte
void AddManBit(uint8_t bit) {
  rx_manBits <<= 1;
  if (bit) rx_manBits |= 0x01;
  (rx_numMB)++;
  if (rx_numMB == 16) {
    uint8_t newData = 0;
    for (int8_t i = 0; i < 8; i++) {
      // rx_manBits holds 16 bits of manchester data
      // 1 = LO,HI
      // 0 = HI,LO
      // We can decode each bit by looking at the bottom bit of each pair.
      newData <<= 1;
      newData |= (rx_manBits & 1); // store the one
      rx_manBits = rx_manBits >> 2; //get next data bit
    }

    man_rx_buff[man_rx_buff_end][(rx_curByte) +1] = newData ^ DECOUPLING_MASK;
    man_rx_buff[man_rx_buff_end][0] = (rx_curByte)+1;

    //data[rx_curByte] = newData ^ DECOUPLING_MASK;
    (rx_curByte)++;
    rx_numMB = 0;
  }
}


void MAN_RX_INTERRUPT_HANDLER() {
  if (rx_mode < RX_MODE_MSG) {//receiving something


    // Increment counter
    rx_pulse_width += rx_pulse_width_inc;


    // Check receive pin
    if (::directRxPort && ::directRxMask) { //use direct port manipulation (much faster)

      switch(::directRxPort) {
        #if defined(PINA)
          case 1: rx_sample = PINA & ::directRxMask; break;
        #endif
        #if defined(PINB)
          case 2: rx_sample = PINB & ::directRxMask; break;
        #endif
        #if defined(PINC)
          case 3: rx_sample = PINC & ::directRxMask; break;
        #endif
        #if defined(PIND)
          case 4: rx_sample = PIND & ::directRxMask; break;
        #endif
        default: rx_sample = 0;
      }

    } else {
			rx_sample = digitalRead(RxPin);
    }


		if (rx_sample != rx_last_sample) { //pin has changed
//      PIND |= (1<<7);
//      PINA |= (1<<0);
      //sss = rx_pulse_width;
      isranie++;
      sranie[isranie] = rx_pulse_width;
			if (rx_mode == RX_MODE_PRE) {
				if (rx_sample) { // Wait for first transition to HIGH
					rx_pulse_width = 0;
					rx_sync_count = 0;
					rx_mode = RX_MODE_SYNC;
					isranie = 0;
				}
			} else if (rx_mode == RX_MODE_SYNC) { // Initial sync block

				if (((rx_sync_count < 20) || (rx_last_sample)) && ((rx_pulse_width < MinCount) || (rx_pulse_width > MaxCount))) {
					// First 20 bits and all 1 bits are expected to be regular
					// Transition was too slow/fast
					rx_mode = RX_MODE_PRE;
					isranie = 0;
				} else if ((!rx_last_sample) && ((rx_pulse_width < MinCount) || (rx_pulse_width > MaxLongCount))) {
					// 0 bits after the 20th bit are allowed to be a double bit
					// Transition was too slow/fast
					rx_mode = RX_MODE_PRE;
					isranie = 0;
				} else {
					rx_sync_count++;
					if ((!rx_last_sample) && (rx_sync_count >= 20) && (rx_pulse_width >= MinLongCount)) {
						// We have seen at least 10 regular transitions
						// Lock sequence ends with unencoded bits 01
						// This is encoded and TX as HI,LO,LO,HI
						// We have seen a long low - we are now locked!
//		PINA |= (1<<3); 		//toggle debug pin A3 on tiny84
  					rx_mode = RX_MODE_DATA;
						rx_manBits = 0;
						rx_numMB = 0;
						rx_curByte = 0;
					} else if (rx_sync_count >= 64) { //preamble too long
						rx_mode = RX_MODE_PRE; //TODO do we really need to drop the packet?
            isranie = 0;
					}
					rx_pulse_width = 0;
				}
			} else if (rx_mode == RX_MODE_DATA) { // Receive data

				if ((rx_pulse_width < MinCount) || (rx_pulse_width > MaxLongCount)) {// wrong pulse length
					rx_mode = RX_MODE_PRE;
					if (rx_curByte > 0) { //if received something, save what we have
						if (!MAN_IS_BUFF_FULL) man_rx_buff_end = (man_rx_buff_end+1) % MAN_BUF_SIZE;
					}
					isranie = 0;
				} else {
					if (rx_pulse_width >= MinLongCount) {// was the previous bit a double bit?
						AddManBit(rx_last_sample);
					}
					if ((rx_sample) && (rx_curByte >= MAN_MESSAGE_SIZE)) { //message too long, truncate it and start over
//						rx_mode = RX_MODE_MSG;
						isranie = 0;
						rx_mode = RX_MODE_PRE;
						if (!MAN_IS_BUFF_FULL) man_rx_buff_end = (man_rx_buff_end+1) % MAN_BUF_SIZE;
					} else {
						// Add the current bit
						AddManBit(rx_sample);
						rx_pulse_width = 0;
					}
				}
			}
			// Get ready for next loop
			rx_last_sample = rx_sample;
		}
  }
} //MAN_RX_INTERRUPT_HANDLER


/*********** TIMER COMPARATOR INTERRUPT **********/

#if defined( __AVR_ATtinyX5__ )
	ISR(TIMER1_COMPA_vect)
#elif defined( __AVR_ATtiny84__ )
	ISR(TIM1_COMPA_vect)
#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1284P__)
	ISR(TIMER3_COMPA_vect)
#elif defined(__AVR_ATmega328P__)
	ISR(TIMER2_COMPA_vect)
#else
	#error "Manchester library doesnt support your microcontroller"
#endif
{
//	PIND |= (1<<7); 		//toggle debug pin 7
	MAN_RX_INTERRUPT_HANDLER();
}

/*********** TIMER OVERFLOW INTERRUPT **********/

#if defined( __AVR_ATtinyX5__ )
	ISR(TIMER1_OVF_vect)
#elif defined( __AVR_ATtiny84__ )
	ISR(TIM1_OVF_vect)
#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1284P__)
	ISR(TIMER3_OVF_vect)
#elif defined(__AVR_ATmega328P__)
	ISR(TIMER2_OVF_vect)
#else
	#error "Manchester library doesnt support your microcontroller"
#endif
{
  DEBUG_TOGGLE();
	MAN_RX_INTERRUPT_HANDLER();
}

//end
