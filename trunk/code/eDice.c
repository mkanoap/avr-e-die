/*
E-die Firmware version 1.2
AVR code for electronic polyhedral die.
(c) Marcus Porter 2011

Distributed under a Creative Commons Attribution-NonCommercial-ShareAlike 3.0 License.

*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <util/delay.h>


/* define the ports and pins */
#define LED_PORT PORTB
#define B1_PIN PIND   // PIN =  Port input
#define B1_BIT PD2		// die type incrementor button
#define B2_PIN PIND		// PIND for board version 1.2
// #define B2_PIN PINA		// PINA for board versions 1.0 and 1.1. 
#define B2_BIT PD3		// die roll button.  Pin D3 for version 1.2
//#define B2_BIT PA1		// die roll button.  Pin A1 for version 1.0 and 1.1. 

/* define debounce time (time to wait to see if a button is really pressed) */
#define DEBOUNCE_TIME 25
#define DELAY_TIME 10 

/* mode of largest die */
#define MAX_DIE 7

/* how many .5088 second intervals to wait before sleeping */
#define TIMEOUT 118

/* function prototypes */
void delay_ms(uint16_t ms);
void init_io();
uint8_t button_is_pressed(uint8_t BUTTON_PIN,uint8_t BUTTON_BIT); // a button was pushed
uint8_t increment_DTYPE(uint8_t DTYPE, uint8_t DISPLAY_MODE); // increment or show Dice types
void display_number(uint8_t DISPLAY_VALUE,uint8_t DTYPE,uint8_t DISPLAY_MODE); // show a number on the screen (Dtype or random number depending on DISPLAY_MODE)

 /* the types array will be global. MAX_DIE should be index of last value */
uint8_t DTYPES[] = {4,6,8,10,12,20,100,2};

// more globals
uint8_t B1_DONE=0; // whether or not the button is just still being pressed
uint8_t B2_DONE=0; // ditto

// Set up an interrupt for when PCINT0 is triggered
ISR(INT0_vect){
	cli(); // we are awake now, turn off interrupts and continue to whever we were when we went to sleep
}

//ISR(INT1_vect){
//        cli(); // we are awake now, turn of interrupts and continue to whever we were when we went to sleep
//}

int
main (void)
{
    uint8_t DISPLAY_VALUE=6; // the current number being displayed (D type or random number)
	 uint8_t DTYPE=1; // 0=4, 1=6, 2=8, 3=10, 4=12, 5=20, 6=100 7=2
//    uint8_t CURRENT_RANDOM_VALUE=1; // the random number
    uint8_t DISPLAY_MODE=0; // 0= the die type, 1 = the random number
	 int IDLE_TIMER=0; // counter of how many .5088 intervals since a button press

/* set up the timers */
	TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode
	OCR1A   = 63600; // Set CTC compare value to a multiple of all the die types.  The counter resets whenever it hits this value
	TCCR1B |= ((1 << CS10) | (1 << CS11)); // Start timer at 8MH/64, the counter will still roll over in .5088 seconds

        init_io();
        while (1)                       
        {

                if (button_is_pressed(B1_PIN, B1_BIT)) // increment dice type
                {
                        if (B1_DONE==0)
                        {
                            DTYPE=increment_DTYPE(DTYPE,DISPLAY_MODE); // increment or show dice type
                            B1_DONE=1; // mark that the button has been pushed to prevent incrementing
                            DISPLAY_MODE=0; // When displaying, show the Dice type.  If this was previously 1, increment_DTYPE() didn't increment
									 IDLE_TIMER=0; // reset the idle timer
                        }
                } else {
                        B1_DONE=0; // if the button gets pressed again, it's a new press
                }
                if (button_is_pressed(B2_PIN, B2_BIT)) // show new random number
                {
                        if (B2_DONE==0)
                        {
									 DISPLAY_VALUE=(TCNT1 % DTYPES[DTYPE]) + 1; // grab a random number from the counter
                            B2_DONE=1; // mark that the button has been pused to prevent rolling again
                            DISPLAY_MODE=1; // when displaying, show the random value chosen
									 IDLE_TIMER=0; // reset the idle timer
                        }
                } else {
                        B2_DONE=0;
                }
                /* show the correct number, either the last chosen random number, or the d type. */
                display_number(DISPLAY_VALUE,DTYPE,DISPLAY_MODE);
				/* update the idle counter if need be, and go to sleep if it's high enough */
				if (TIFR & (1 << OCF1A)) // if the counter has rolled over, the OCF1A bit will be set (CTC flag)
				{
					TIFR = (1 << OCF1A); // clear the CT flag by writing 1 to it (bizarre)
					IDLE_TIMER +=1;
					if (IDLE_TIMER > TIMEOUT) // if the IDLE time has passed, go to sleep
					{
					//	DISPLAY_VALUE=88; // replace with sleep code
						PORTD = 0x0F; // turn off the LEDs by setting both cathods HIGH, while leaving buttons as input
						set_sleep_mode(SLEEP_MODE_PWR_DOWN); //set sleep mode
						sei(); //enable interrupt
						sleep_enable();
						sleep_mode(); //sleep now
						sleep_disable(); // wake up
						IDLE_TIMER=0; // wakeing up, reset the timer
						cli(); // clear interrupts except when sleeping
                	delay_ms(DEBOUNCE_TIME);
                	delay_ms(DEBOUNCE_TIME);
						B1_DONE=1; // mark that the button has been pushed to prevent incrementing
						DISPLAY_MODE=1; // when waking up, show the random numbers
					} // end of "time to sleep"
				} // end of idle counter increment
        } // end of main loop
} // end of main

/*
 * button_is_pressed - Check if the button is being pressed with debounce.
 * Returns 1 if the button was pressed, 0 otherwise.
 * 
 */
uint8_t
button_is_pressed(uint8_t BUTTON_PIN,uint8_t BUTTON_BIT)
{
        /* the button is pressed when BUTTON_BIT is clear */
        if (bit_is_clear(BUTTON_PIN, BUTTON_BIT))
        {
                delay_ms(DEBOUNCE_TIME);
                if (bit_is_clear(BUTTON_PIN, BUTTON_BIT)) return 1;
        }
       
        return 0;
}

/*
 * increment the dice type
 *
 */
uint8_t
increment_DTYPE(uint8_t loDTYPE, uint8_t loDISPLAY_MODE)
{
    if (loDISPLAY_MODE==0) { /* only change if we are in Dice type display mode */
        loDTYPE=loDTYPE+1;
        if (loDTYPE>MAX_DIE) {
            loDTYPE=0;
		}
    }
    return loDTYPE;
}


/*
 * display the correct number on the LED
 *
 */
void
display_number(uint8_t DISPLAY_VALUE, uint8_t LDTYPE, uint8_t LDISPLAY_MODE)
{
   // Define in an array all the numbers that turn on the LEDs in 7segment display
	uint8_t digits[] = {
   	0b00111111,  // 0 = .gFEDCBA = 63
		0b00000110,  // 1 = .gfedCBa = 6
		0b01011011,  // 2 = .GfEDcBA = 91
		0b01001111,  // 3 = .GfeDCBA = 79
		0b01100110,  // 4 = .GFedCBa = 102
		0b01101101,  // 5 = .GFeDCbA = 109
		0b01111101,  // 6 = .GFEDCbA = 125
		0b00000111,  // 7 = .gfedCBA = 7
		0b01111111,  // 8 = .GFEDCBA = 127
		0b01101111,  // 9 = .GFeDCBA = 111
		0b01000110,  // T = .GfedCBa = 70
		0b01110110,  // H = .GFEdCBa = 118
		0b01000000   // - = .Gfedcba = 64
	};
	// LDISPLAY_MODE = local copy of the display mode. 0= the die type, 1 = the random number
	uint8_t DISPLAY;
	uint8_t WIPE; // value used to wipe the display
	if (LDISPLAY_MODE==1) {
		DISPLAY=DISPLAY_VALUE; // we are showing the random number
		WIPE = 0x00; // turn off all the digets, including the decimal
	} else {
		DISPLAY=DTYPES[LDTYPE]; // we are displaying the die type
		WIPE = 0b10000000; // turn on the decimal point to indicate Die type mode
	}
	// do the right digit
	PORTD = 0x0F; // turn off the LEDs by setting both cathods HIGH
	LED_PORT = WIPE;
	if ((LDTYPE==7) & (LDISPLAY_MODE==1)) { // if it's a D2, show heads and tails
		if (DISPLAY==1) { // tails
			LED_PORT ^= digits[10];
		} else {
			LED_PORT ^= digits[11];
		}
	} else if (B2_DONE==1) { // if the roll button is currently held down
		LED_PORT ^= digits[12]; // show "-"
	} else {
		LED_PORT ^= digits[DISPLAY % 10]; // set right digit
	}
	PORTD &= ~(1 << 1); // turn on right digit by turning OFF bit 1
    delay_ms(DELAY_TIME);

	// If appropriate, do the left digit
	if (B2_DONE==1) { // if the roll button is currently held down
		PORTD = 0x0F; // turn off the LEDs by setting both cathods HIGH, while leaving buttons as input
		PORTD &= ~(1); // turn on the left digit by turning OFF bit 0
		delay_ms(DELAY_TIME);
	} else {
		if ((DISPLAY/10)>0) {
			PORTD = 0x0F; // turn off the LEDs by setting both cathods HIGH, while leaving buttons as input
			LED_PORT=WIPE;
			LED_PORT ^= digits[(DISPLAY/10) % 10];  // display second diget on left
			PORTD = 0x0E; // turn on the left digit by turning OFF bit 0
			delay_ms(DELAY_TIME);
			LED_PORT=0x00;
		}
		if ((LDTYPE==7) & (LDISPLAY_MODE==1) & (DISPLAY==1)) { // if it's a D2, show heads and tails
//			PORTD |= 3; // turn off the LEDs by setting both cathods HIGH
			PORTD = 0x0F; // turn off the LEDs by setting both cathods HIGH
			LED_PORT=WIPE;
			LED_PORT ^= digits[12];  // show a minus for the bottom of the T in tails
//			PORTD &= ~(1); // turn on the left digit by turning OFF bit 0
			PORTD = 0x0E; // turn on the left digit by turning OFF bit 0
			delay_ms(DELAY_TIME);
		}
	}
}

void
init_io()
{
	DDRA = 0x00;  // set port A as input (only important for version 1.0 and 1.1 of board)
	PORTA = 0xFF;       // set port A with internal pullups
	DDRB = 0xFF;  // set Port B as output for segments
	LED_PORT = 0x00;	    // intialize Port B to low (off)
	DDRD = 0x03;  // set port D0,1 as output for LED cathodes, the rest are inputs
	PORTD = 0x0F; // set port D high (off) on cathods on pins D0 and D1.  Set pin D2 and D3 with internal pullup

  // Set physical Pin 6 (PD2 or INT0) as the pin to use for the interrupt (wake up from sleep)
  PCMSK |= (1<<PIND2);

  // interrupt on INT0 pin falling edge (sensor triggered) 
  MCUCR = (0<<ISC01) | (0<<ISC00);

  // turn on interrupts!
  GIMSK  |= (1<<INT0);
	cli(); // we are awake now, turn off interrupts and continue to whever we were when we went to sleep

}

/* 
 * delay_ms - Perform long delays in approximate milliseconds.
 */ 
void 
delay_ms(uint16_t ms) 
{
        while ( ms ) 
        {
                _delay_ms(1);
                ms--;
        }
}

