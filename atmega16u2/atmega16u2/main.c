#define F_CPU 16000000UL
#include "util/delay.h"
#include <avr/io.h>


int main(void) {
	//SREG |= 0B10000000;
	DDRC |= 0B00100000;			//LED output
	PLLCSR |= 0B00000100;		//PLL prescaler 1:2 (16MHz source -> 8MHz)
	PLLCSR |= 0B00000010;		//enable PLL
	while(!(PLLCSR & 0B00000001)) {}	//wait for PLL lock
	
	USBCON |= (1 << USBE);		//enable USB
	USBCON &= ~(1 << FRZCLK);	//unfreeze USB clock

	UENUM = 0;					// select endpoint 0
	UECONX |= (1 << EPEN);		// enable endpoint 0
	UECFG0X = 0;				// endpoint 0 is a control endpoint
	UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);    // endpoint 0: 64 bytes, one bank, allocate memory
	while (!(UESTA0X & (1 << CFGOK))) {}	// wait for configuration
	
	UDCON = 0;					//attach USB
	PORTC |= 0B00100000;		//light up LED
	
	while (1) {
		if(UEINTX & 0B00001000) UEINTX &= 0B11110111;
		if(UEINTX & 0B00000100) UEINTX &= 0B11111011;
		if(UEINTX & 0B00000001) UEINTX &= 0B11111110;
	}
}

