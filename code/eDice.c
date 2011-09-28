#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>


/* define the ports and pins */
#define BUTTON_PORT PORTA
#define LED_PORT PORTB
#define B1_PIN PINA
#define B1_BIT PA0		// die type incrementor button
#define B2_PIN PINA
#define B2_BIT PA1		// die roll button

/* define debounce time (time to wait to see if a button is really pressed) */
#define DEBOUNCE_TIME 25

/* mode of largest die */
#define MAX_DIE 7
/* function prototypes */

void delay_ms(uint16_t ms);
void init_io();
uint8_t button_is_pressed(uint8_t BUTTON_PIN,uint8_t BUTTON_BIT); // a button was pushed
uint8_t increment_DTYPE(uint8_t DTYPE, uint8_t DISPLAY_MODE); // increment or show Dice types
uint8_t increment_CURRENT_RANDOM_VALUE(uint8_t DTYPE, uint8_t CURRENT_RANDOM_VALUE); // increment the random number
void display_number(uint8_t DISPLAY_VALUE,uint8_t DTYPE,uint8_t DISPLAY_MODE); // show a number on the screen (Dtype or random number depending on DISPLAY_MODE)

 /* the types array will be global. MAX_DIE should be index of last value */
uint8_t DTYPES[] = {4,6,8,10,12,20,100,2};
uint8_t DEBUG_DTYPE=4; // delete me later

int
main (void)
{
    uint8_t B1_DONE=0; // whether or not the button is just still being pressed
    uint8_t B2_DONE=0; // ditto
    uint8_t DISPLAY_VALUE=6; // the current number being displayed (D type or random number)
    uint8_t DTYPE=1; // 0=4, 1=6, 2=8, 3=10, 4=12, 5=20, 6=100
    uint8_t CURRENT_RANDOM_VALUE=1; // the random number
    uint8_t DISPLAY_MODE=0; // 0= the die type, 1 = the random number

        init_io();
        while (1)                       
        {
//DEBUG_DTYPE=DTYPES[DTYPE];

                if (button_is_pressed(B1_PIN, B1_BIT)) // increment dice type
                {
                        if (B1_DONE==0)
                        {
                            DTYPE=increment_DTYPE(DTYPE,DISPLAY_MODE); // increment or show dice type
                            B1_DONE=1; // mark that the button has been pushed to prevent incrementing
                            DISPLAY_MODE=0; // When displaying, show the Dice type.  If this was previously 1, increment_DTYPE() didn't increment
                        }
                } else {
                        B1_DONE=0; // if the button gets pressed again, it's a new press
                }
                if (button_is_pressed(B2_PIN, B2_BIT)) // show new random number
                {
                        if (B2_DONE==0)
                        {
                            DISPLAY_VALUE=CURRENT_RANDOM_VALUE; // grab the current random number and save it
                            B2_DONE=1; // mark that the button has been pused to prevent rolling again
                            DISPLAY_MODE=1; // when displaying, show the random value chosen
                        }
                } else {
                        B2_DONE=0;
                }
                /* move the random number forward, no matter what */
                CURRENT_RANDOM_VALUE=increment_CURRENT_RANDOM_VALUE(DTYPE,CURRENT_RANDOM_VALUE);
                /* show the correct number, either the last chosen random number, or the d type. */
                display_number(DISPLAY_VALUE,DTYPE,DISPLAY_MODE);
        }
}

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
 * increment the random number
 *
 */
uint8_t
increment_CURRENT_RANDOM_VALUE(uint8_t loDTYPE,uint8_t DVAL)
{
    DVAL=DVAL+1;
    /* DTYPES is a global array of dice types. 
     *  If the random number is bigger than the current die type, wrap around to 1
     */
    if (DVAL>DTYPES[loDTYPE]) {
        DVAL=1;
    }
    return DVAL;
}

/*
 * display the correct number on the LED
 *
 */
void
display_number(uint8_t DISPLAY_VALUE, uint8_t DTYPE, uint8_t LDISPLAY_MODE)
{
   // Define in an array all the numbers that turn on the LEDs in 7segment display
	uint8_t digits[] = {
   		0b00111111,  // 0 = .gfEDCBA = 63
		0b00000110,  // 1 = .gfedCBa = 6
		0b01011011,  // 2 = .GfEDcBA = 91
		0b01001111,  // 3 = .GfeDCBA = 79
		0b01100110,  // 4 = .GFedCBa = 102
		0b01101101,  // 5 = .GFeDCbA = 109
		0b01111101,  // 6 = .GFEDCbA = 125
		0b00000111,  // 7 = .gfedCBA = 7
		0b01111111,  // 8 = .GFEDCBA = 127
		0b01101111,  // 9 = .GFeDCBA = 111
		0b01000110,  // T = .GfedcBA = ???
		0b01110110,  // H = .GFEdCBa = ???
		0b01000000   // - = .Gfedcba = ???
	};
	// LDISPLAY_MODE = local copy of the display mode. 0= the die type, 1 = the random number
	uint8_t DISPLAY;
	uint8_t WIPE; // value used to wipe the display
	if (LDISPLAY_MODE==1) {
		DISPLAY=DISPLAY_VALUE; // we are showing the random number
		WIPE = 0x00; // turn off all the digets, including the decimal
	} else {
		DISPLAY=DTYPES[DTYPE]; // we are displaying the die type
		WIPE = 0b10000000; // turn on the decimal point to indicate Die type mode
	}
	// do the right digit
	PORTD = 3; // turn off both LEDs by setting both cathods HIGH
	LED_PORT = WIPE;
	if (DTYPE==7 & LDISPLAY_MODE==1) { // if it's a D2, show heads and tails
		if (DISPLAY==1) { // tails
			LED_PORT ^= digits[10];
		} else {
			LED_PORT ^= digits[11];
		}
	} else {
		LED_PORT ^= digits[DISPLAY % 10]; // set right digit
	}
	PORTD = 1; // turn on right digit by turning OFF bit 2
    delay_ms(1);

	// If appropriate, do the left digit
	if ((DISPLAY/10)>0) {
		PORTD = 3; // turn off the LEDs by setting both cathods HIGH
		LED_PORT=WIPE;
		LED_PORT ^= digits[(DISPLAY/10) % 10];  // display second diget on left
		PORTD = 2; // turn on the left digit by turning OFF bit 1
		delay_ms(1);
	}
	if (DTYPE==7 & LDISPLAY_MODE==1 & DISPLAY==1) { // if it's a D2, show heads and tails
		PORTD = 3; // turn off the LEDs by setting both cathods HIGH
		LED_PORT=WIPE;
		LED_PORT ^= digits[12];  // show a minus for the bottom of the T in tails
		PORTD = 2; // turn on the left digit by turning OFF bit 1
		delay_ms(1);
	}
}

void
init_io()
{
	DDRA = 0x00;  // set port A as input
	BUTTON_PORT = 0xFF;       // set port A with internal pullups
	DDRB = 0xFF;  // set Port B as output for segments
	LED_PORT = 0x00;	    // intialize Port B to low (off)
	DDRD = 0xFF;  // set port D as output for LED cathodes
	PORTD = 0x03; // set port D high on cathods (off)
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

