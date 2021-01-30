#define F_CPU 16000000UL
#include "util/delay.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_REFRESH 1000
#define BUFFER_LENGTH 64

volatile uint16_t millis = 0;
uint16_t LEDbarTimer = 0;

uint8_t USBbuffer[BUFFER_LENGTH] = {0};
uint8_t bufferPointer = 0;

void led(uint8_t state);

ISR(TIMER0_COMPA_vect){
	millis++;
}

ISR(USB_COM_vect) {
	
}

void setupMillis() {
	TCCR0A |= (1 << WGM01);	//CTC
	TCCR0B |= (1 << CS02) | (1 << CS00);
	OCR0A = 14;	//for 1kHz
	TCNT0 = 0;
	TIMSK0 |= (1 << OCF0A);
}

void setupEndpoint_0() {
	UENUM = 0;					// select endpoint 0
	UECONX |= (1 << EPEN);		// enable endpoint 0
	UECFG0X = 0;				// endpoint 0 is a control endpoint
	UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);    // endpoint 0: 64 bytes, one bank, allocate memory
	while (!(UESTA0X & (1 << CFGOK))) {}	// wait for configuration
	//UEIENX |= (1 << RXSTPE) | (1 << RXOUTE) | (1 << TXINE);
}

void led(uint8_t state) {
	if(state == 0) PORTC &= ~(1 << PORTC5);	//LED off
	else PORTC |= (1 << PORTC5);			//LED on
}

void controlRead() {
	bufferPointer = 1;
	
	while(UEBCLX != 0 && bufferPointer < BUFFER_LENGTH) {	//while there is data in UEDATX
		USBbuffer[bufferPointer] = UEDATX;
		bufferPointer++;
	}
	UEINTX &= ~(1 << RXSTPI);	//clears UEDATX!
	
	for (uint8_t i = 0; i < bufferPointer; i++) {
		PORTD = USBbuffer[i];
		_delay_ms(LED_REFRESH);
	}
}

void controlWrite() {
	
}

int main(void) {
	cli();
	DDRC |= (1 << DDC5);		//LED output
	DDRD = 0xFF;				//LEDbar output
	
	setupMillis();
	
	PLLCSR |= 0B00000100;		//PLL prescaler 1:2 (16MHz source -> 8MHz)
	PLLCSR |= (1 << PLLE);		//enable PLL
	while(!(PLLCSR & (1 << PLOCK))) {}	//wait for PLL lock
	USBCON |= (1 << USBE);		//enable USB
	USBCON &= ~(1 << FRZCLK);	//unfreeze USB clock

	setupEndpoint_0();
	
	UDCON &= ~(1 << DETACH);	//attach USB
	UDCON |= (1 << RSTCPU);		//allow USB reset
	sei();
	
	while (1) {
		if(MCUSR & (1 << USBRF)) {	//USB reset
			cli();
			setupEndpoint_0();
			sei();
		}
		
		if(UEINTX & (1 << RXSTPI)) {	//setup command from USB host
			USBbuffer[0] = UEDATX;
			if(USBbuffer[0] & (1 << 7)) controlRead();
			else controlWrite();
		}
		/*
		if(UEINTX & (1 << RXOUTI)) {
			UEINTX &= ~(1 << RXOUTI);
		}
		if(UEINTX & (1 << TXINI)) {
			UEINTX &= ~(1 << TXINI);
		}
		*/
		if(UDINT & (1 << WAKEUPI)) {
			UEINTX &= ~(1 << WAKEUPI);
			UEINTX &= ~(1 << SUSPI);
		}
	}
}

