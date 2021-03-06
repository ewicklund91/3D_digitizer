#ifndef __ENCODERS 
#define __ENCODERS

/*
#ifndef checks to see if this is not currently defined
#define is to give a name to a constant value 

Syntax
#define constantName value

Parameters
constantName: the name of the macro to define.
value: the value to assign to the macro.
*/

//Settings
#define MAX_ENCODERS 8             //Restrict number of encoders
#define STATES_PER_REV 2400        //Lines per rev * 4; if your encoder is 600P/R this value would be 2400 etc.
#define DEFAULT_RATE 10000         //[Hz]

volatile unsigned int timerIncrement = (16000000L / DEFAULT_RATE);

/*
Volatile indicates that value may change between different accesses, even if it does not appear to be modified. 
Primarily in hardware access (memory-mapped I/O), where reading from or writing to memory is used to communicate with peripheral devices,
and in threading, where a different thread may have modified a value.

unsigned ints (unsigned integers) are the same as ints
that they store a 2 byte value
they only store positive values, range of 0 to 65,535 ((2^16) - 1).

Syntax
unsigned int var = val;

Parameters
var: variable name.
val: the value you assign to that variable.
*/


//Some macros
#define SET(x,y) (x |=(1<<y))          //-Bit set/clear macros
#define CLR(x,y) (x &= (~(1<<y)))      // |
#define CHK(x,y) (x & (1<<y))          // |
#define TOG(x,y) (x^=(1<<y))           //-+

/*
Description
^= is often used with a variable and a constant to toggle (invert) particular bits in a variable.

Syntax
x ^= y; // equivalent to x = x ^ y;

Parameters
x: variable. Allowed data types: char, int, long.
y: variable or constant. Allowed data types: char, int, long.

Description
&= is often used with a variable and a constant to force particular bits in a variable to the LOW state (to 0). 

Syntax
x &= y; // equivalent to x = x & y;

Parameters
x: variable. Allowed data types: char, int, long.
y: variable or constant. Allowed data types: char, int, long.

Description
|= is often used with a variable and a constant to "set" (set to 1) particular bits in a variable.

Syntax
x |= y; // equivalent to x = x | y;

Parameters
x: variable. Allowed data types: char, int, long.
y: variable or constant. Allowed data types: char, int, long.
*/


// Encoder state machine:
// encoder offset = encref [old state][encoder input]
// 128 -> state machine error (impossible state change)
volatile int encref[4][4] =
{
  //  0  1  2  3
  {
    0, 1, -1, 128
  }
  ,//0
  {
    -1, 0, 128, 1
  }
  ,//1
  {
    1, 128, 0, -1
  }
  ,//2
  {
    128, -1, 1, 0
  }// 3
};

int setEncoderRate(unsigned int rate)
{
  if ((rate <= 20000) && (rate >= 250))
  {
    timerIncrement = 16000000L / rate;
    return rate;
  }
  return -1;
}



volatile unsigned char attachedEncoders = 0;
class IQencoder* encoders[MAX_ENCODERS];

class IQencoder
{
  public:
    volatile int encoderCounter;
    unsigned char I_pin;
    unsigned char Q_pin;
    unsigned char state;

    void attach(unsigned char I, unsigned char Q)
    {
      I_pin = I;
      Q_pin = Q;
      state = 0;
      pinMode(I_pin, INPUT_PULLUP);
      pinMode(Q_pin, INPUT_PULLUP);
      encoderCounter = 0;
      if (attachedEncoders < MAX_ENCODERS)
        encoders[attachedEncoders++] = this;  //-Add encoder to sampling system

      if (attachedEncoders == 1)              //-First encoder starts sampling system
      {
        TCCR1A = 0x00;                        //-Timer 1 inerrupt
        TCCR1B = 0x01;                        // |
        TCCR1C = 0x00;                        // |
        SET(TIMSK1, OCIE1A);                  // |
        sei();                                //-+
      }
    }

    void setDegrees(float deg)
    {
      encoderCounter = (deg / 360.0) * (float)STATES_PER_REV;
    }
    void setRadians(float rad)
    {
      encoderCounter = (rad / M_PI * 2) * (float)STATES_PER_REV;
    }

    float getRadians()
    {
      return (double)encoderCounter * M_PI * 2 / (double)STATES_PER_REV;
    }
    float getDegrees()
    {
      return (double)encoderCounter * 360.0 / (double)STATES_PER_REV;
    }
};


//Global encoder sampler timer interrupt
SIGNAL(TIMER1_COMPA_vect)
{
  OCR1A += timerIncrement;
  volatile unsigned char input;
  for (unsigned char i = 0; i < attachedEncoders; i++)
  {
    input = 0;
    if (digitalRead(encoders[i]->I_pin) == HIGH)
      input |= 0x02;
    if (digitalRead(encoders[i]->Q_pin) == HIGH)
      input |= 0x01;
    encoders[i]->encoderCounter += encref[encoders[i]->state][input];
    encoders[i]->state = input;
  }
}
#endif
