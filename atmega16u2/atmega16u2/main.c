#define F_CPU 16000000UL
#include "util/delay.h"
#include <avr/io.h>
#include <avr/interrupt.h>

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
//descriptor types
#define DESCRIPTOR_DEVICE 1
#define DESCRIPTOR_CONFIGURATION 2
#define DESCRIPTOR_STRING 3
#define DESCRIPTOR_INTERFACE 4
#define DESCRIPTOR_ENDPOINT 5
#define DESCRIPTOR_DEVICE_QUALIFIER 6
#define DESCRIPTOR_OTHER_SPEED_CONFIGURATION 7
#define DESCRIPTOR_INTERFACE_POWER 8
//device descriptor
#define DD_bLength 18
#define DD_bDescriptorType DESCRIPTOR_DEVICE
#define DD_bcdUSB_L 0x10	//USB 1.1
#define DD_bcdUSB_H 0x01
#define DD_bDeviceClass 0x00
#define DD_bDeviceSubClass 0x00
#define DD_bDeviceProtocol 0x00
#define DD_bMaxPacketSize0 64
#define DD_idVendor_L 0xEB
#define DD_idVendor_H 0x03
#define DD_idProduct_L 0x47
#define DD_idProduct_H 0x20
#define DD_bcdDevice_L 0x01
#define DD_bcdDevice_H 0x00
#define DD_iManufacturer 0
#define DD_iProduct 0
#define DD_iSerialNumber 0
#define DD_bNumConfigurations 1
const uint8_t DD_array[DD_bLength] = {DD_bLength, DD_bDescriptorType, DD_bcdUSB_L, DD_bcdUSB_H, DD_bDeviceClass, DD_bDeviceSubClass, 
									   DD_bDeviceProtocol, DD_bMaxPacketSize0, DD_idVendor_L, DD_idVendor_H, DD_idProduct_L, DD_idProduct_H,
									   DD_bcdDevice_L, DD_bcdDevice_H, DD_iManufacturer, DD_iProduct, DD_iSerialNumber, DD_bNumConfigurations};
//configuration descriptor
#define CD_bLength 9
#define CD_bDescriptorType DESCRIPTOR_CONFIGURATION
#define CD_wTotalLength_L 64
#define CD_wTotalLength_H 0
#define CD_bNumInterfaces 1
#define CD_bConfigurationValue 1
#define CD_iConfiguration 0
#define CD_bmAttributes 0b10000000
#define CD_bMaxPower 50 //50*2mA = 100mA
const uint8_t CD_array[CD_bLength] = {CD_bLength, CD_bDescriptorType, CD_wTotalLength_L, CD_wTotalLength_H, CD_bNumInterfaces, 
									   CD_bConfigurationValue, CD_iConfiguration, CD_bmAttributes, CD_bMaxPower};
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
	if(UEINTX & (1 << RXSTPI)) {	//setup command from USB host
		if(UEBCLX == 0) return;
		
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
		
		/*
		if(commandCount == 3){
			PORTD = wValue >> 8;
			led(1);
		}
		
		if(bRequest != GET_DESCRIPTOR && bRequest != SET_ADDRESS && newCommand == 0) {
			newCommand = 1;
			//led(1);
		}
		*/
		
		controlTransfer();
		
		if(bufferIndex < (BUFFER_LENGTH - 4)) {
			controlBuffer[bufferIndex] = bmRequestType;
			controlBuffer[bufferIndex+1] = bRequest;
			controlBuffer[bufferIndex+2] = wValue;
			controlBuffer[bufferIndex+3] = wValue >> 8;
			bufferIndex += 4;
		}
		
		if(commandCount < 255) commandCount++;
	}
}

void setupMillis() {
	TCCR0A |= (1 << WGM01);	//CTC
	TCCR0B |= (1 << CS02) | (1 << CS00);
	OCR0A = 14;	//for 1kHz
	TCNT0 = 0;
	TIMSK0 |= (1 << OCF0A);
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
	UENUM = 0;					// select endpoint 0
	UECONX |= (1 << EPEN);		// enable endpoint 0
	UECFG0X = 0;				// endpoint 0 is a control endpoint
	UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0) | (1 << ALLOC);    // endpoint 0: 64 bytes, one bank, allocate memory
	while (!(UESTA0X & (1 << CFGOK))) {}	// wait for configuration
	//UEIENX |= (1 << RXSTPE);				//enable USB COM interrupts
}

void led(uint8_t state) {
	if(state == 0) PORTC &= ~(1 << PORTC5);	//LED off
	else PORTC |= (1 << PORTC5);			//LED on
}

void sendDescriptor(uint8_t descriptorType) {
	UEINTX &= ~(1 << RXSTPI);			//ACK setup packet, clears UEDATX!
	switch (descriptorType) {
	case DESCRIPTOR_DEVICE:
		if(wLength > DD_bLength) wLength = DD_bLength;
		for (uint8_t i = 0; i < wLength; i++) {
			UEDATX = DD_array[i];
		}
		break;
	case DESCRIPTOR_CONFIGURATION:
		led(1);
		if(wLength > CD_bLength) wLength = CD_bLength;
		for (uint8_t i = 0; i < wLength; i++) {
			UEDATX = CD_array[i];
		}
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
		case CLEAR_FEATURE:
		break;
		}
	break;
	}
}

int main(void) {
	cli();
	DDRC |= (1 << DDC5);		//LED output
	DDRD = 0xFF;				//LEDbar output
	setupMillis();
	setupUSB();
	setupEndpoint_0();
	sei();
	
	while (1) {
		if(MCUSR & (1 << USBRF)) {	//USB reset
			cli();
			setupEndpoint_0();
			sei();
		}
		
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
		
			if(bufferIndex < (BUFFER_LENGTH - 4)) {
				controlBuffer[bufferIndex] = bmRequestType;
				controlBuffer[bufferIndex+1] = bRequest;
				controlBuffer[bufferIndex+2] = wValue;
				controlBuffer[bufferIndex+3] = wValue >> 8;
				bufferIndex += 4;
			}
			
			if(commandCount < 255) commandCount++;
		}

		if(UDINT & (1 << WAKEUPI)) {
			UEINTX &= ~(1 << WAKEUPI);
			UEINTX &= ~(1 << SUSPI);
		}
	}
}

