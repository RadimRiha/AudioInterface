#define F_CPU 16000000UL
#include "util/delay.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "descriptors.h"

#define LED_REFRESH 1000

//setup request codes
#define GET_STATUS 0
#define CLEAR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADDRESS 5
#define GET_DESCRIPTOR 6
#define SET_DESCRIPTOR 7
#define GET_CONFIGURATION 8
#define SET_CONFIGURATION 9
#define GET_INTERFACE 10
#define SET_INTERFACE 11
#define SYNCH_FRAME 12
									   
//control endpoint values
volatile uint8_t bmRequestType, bRequest;
volatile uint16_t wValue, wIndex, wLength;

volatile uint16_t millis = 0;
uint16_t LEDbarTimer = 0;

volatile uint8_t newCommand = 0;
volatile uint8_t commandCount = 0;
#define BUFFER_LENGTH 64
volatile uint8_t controlBuffer[BUFFER_LENGTH] = {0};
volatile uint8_t bufferIndex = 0;
uint8_t bufferReadIndex = 0;

void led(uint8_t state);
void controlTransfer();

ISR(TIMER0_COMPA_vect){
	millis++;
}

ISR(USB_COM_vect) {

}

void setupMillis() {
	TCCR0A |= (1 << WGM01);					//CTC
	TCCR0B |= (1 << CS02) | (1 << CS00);	//prescaler 1024
	OCR0A = 14;								//for 1kHz (16M/1024/1000-1)
	TCNT0 = 0;
	TIMSK0 |= (1 << OCF0A);					//enable COMPA_vect
}

void setupUSB() {
	PLLCSR |= 0B00000100;		//PLL prescaler 1:2 (16MHz source -> 8MHz)
	PLLCSR |= (1 << PLLE);		//enable PLL
	while(!(PLLCSR & (1 << PLOCK))) {}	//wait for PLL lock
	USBCON |= (1 << USBE);		//enable USB
	USBCON &= ~(1 << FRZCLK);	//unfreeze USB clock
	UDCON &= ~(1 << DETACH);	//attach USB
	UDCON |= (1 << RSTCPU);		//allow USB reset
}

void setupEndpoint_0() {
	UENUM = 0;					//select endpoint 0
	UECONX |= (1 << EPEN);		//enable endpoint
	UECFG0X = 0;				//control endpoint
	UECFG1X = (0b011 << EPSIZE0) | (1 << ALLOC);    //64 bytes, one bank, allocate memory
	while (!(UESTA0X & (1 << CFGOK))) {}			//wait for configuration
	//UEIENX |= (1 << RXSTPE);						//enable USB COM interrupts
}

void setupEndpoint_1() {
	UENUM = 1;										//select endpoint 1
	UECONX |= (1 << EPEN);							//enable endpoint
	UECFG0X = (0b01 << EPTYPE0) | (1 << EPDIR);		//isochronous IN endpoint
	UECFG1X = (0b011 << EPSIZE0) | (1 << ALLOC);    //64 bytes, one bank, allocate memory
	while (!(UESTA0X & (1 << CFGOK))) {}			//wait for configuration
}

void led(uint8_t state) {
	if(state == 0) PORTC &= ~(1 << PORTC5);	//LED off
	else PORTC |= (1 << PORTC5);			//LED on
}

void sendDescriptor(uint8_t descriptorType) {
	UEINTX &= ~(1 << RXSTPI);	//ACK setup packet, clears UEDATX!
	uint8_t i = 0;
	switch (descriptorType) {
		case DESCRIPTOR_DEVICE:
			if(wLength > DD_bLength) wLength = DD_bLength;
			for (uint8_t i = 0; i < wLength; i++) {
				UEDATX = DD_array[i];
			}
			break;
		case DESCRIPTOR_CONFIGURATION:
			//if(wLength > CD_bLength) wLength = CD_bLength;
			for (i = 0; i < CD_bLength; i++) UEDATX = CD_array[i];
			for (i = 0; i < ID_bLength; i++) UEDATX = ID_array[i];
			for (i = 0; i < ED_bLength; i++) UEDATX = ED_array[i];
			break;
		default:
			return;
			break;
	}
	
	UEINTX &= ~(1 << TXINI);			//send the bank
	while(!(UEINTX & (1 << TXINI))) {}	//wait for controller to signal bank free
	while(!(UEINTX & (1 << RXOUTI))) {}	//wait for OUT packet
	UEINTX &= ~(1 << RXOUTI);			//ACK the OUT packet
}

void controlTransfer() {
	switch (bmRequestType) {
		case 0b10000000:
			switch (bRequest) {
				case GET_DESCRIPTOR:
					sendDescriptor(wValue >> 8);
					break;
				default:
					break;
			}
			break;
		case 0b00000000:
			switch (bRequest) {
				case SET_ADDRESS:
					UDADDR = 0;
					UDADDR |= wValue & ~(1 << ADDEN);	//set only address field
					UEINTX &= ~(1 << RXSTPI);			//ACK setup packet
					UEINTX &= ~(1 << TXINI);			
					while(!(UEINTX & (1 << TXINI))) {}	//wait for IN packet
					UDADDR |= (1 << ADDEN);				//enable address field
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

int main(void) {
	cli();
	DDRC |= (1 << DDC5);	//LED output
	DDRD = 0xFF;			//LEDbar output
	setupMillis();
	setupUSB();
	setupEndpoint_1();
	setupEndpoint_0();
	sei();
	
	while (1) {
		if(MCUSR & (1 << USBRF)) {	//USB reset
			cli();
			setupEndpoint_1();
			setupEndpoint_0();
			sei();
		}
		
		//debug
		if((millis - LEDbarTimer) >= LED_REFRESH && (bufferReadIndex < bufferIndex)) {
			if(bufferReadIndex % 4 == 0) led(1);
			else led(0);
			PORTD = controlBuffer[bufferReadIndex];
			bufferReadIndex++;
			LEDbarTimer = millis;
		}
		
		if(UEINTX & (1 << RXSTPI) && UEBCLX != 0) {	//setup command from USB host	
			bmRequestType = UEDATX;
			bRequest = UEDATX;
			wValue = 0;
			wValue =  UEDATX;
			wValue |= (UEDATX << 8);
			wIndex = 0;
			wIndex =  UEDATX;
			wIndex |= (UEDATX << 8);
			wLength = 0;
			wLength =  UEDATX;
			wLength |= (UEDATX << 8);
		
			controlTransfer();
			
			//debug
			if(bufferIndex < (BUFFER_LENGTH - 4)) {
				controlBuffer[bufferIndex] = bmRequestType;
				controlBuffer[bufferIndex+1] = bRequest;
				controlBuffer[bufferIndex+2] = wValue;
				controlBuffer[bufferIndex+3] = wValue >> 8;
				bufferIndex += 4;
			}
			
			if(commandCount < 255) commandCount++;
		}

		if(UDINT & (1 << WAKEUPI)) {	//clear suspend on wakeup
			UEINTX &= ~(1 << WAKEUPI);
			UEINTX &= ~(1 << SUSPI);
		}
	}
}

