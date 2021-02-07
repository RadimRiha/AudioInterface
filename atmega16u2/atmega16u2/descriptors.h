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

//configuration descriptor
#define CD_bLength 9
#define CD_bDescriptorType DESCRIPTOR_CONFIGURATION
#define CD_wTotalLength_L (CD_bLength + ID_bLength + ED_bLength)
#define CD_wTotalLength_H 0
#define CD_bNumInterfaces 1
#define CD_bConfigurationValue 0
#define CD_iConfiguration 0
#define CD_bmAttributes 0b10000000
#define CD_bMaxPower 50 //50*2mA = 100mA

//interface descriptor
#define ID_bLength 9
#define ID_bDescriptorType DESCRIPTOR_INTERFACE
#define ID_bInterfaceNumber 0
#define ID_bAlternateSetting 0
#define ID_bNumEndpoints 0
#define ID_bInterfaceClass 0xFF
#define ID_bInterfaceSubClass 1
#define ID_bInterfaceProtocol 2
#define ID_iInterface 0

//endpoint descriptor
#define ED_bLength 7
#define ED_bDescriptorType DESCRIPTOR_ENDPOINT
#define ED_bEndpointAddress 0b10000001	//IN endpoint ID 1
#define ED_bmAttributes 0b00000001		//isochronous endpoint, no sync, data
#define ED_wMaxPacketSize_L 64
#define ED_wMaxPacketSize_H 0
#define ED_bInterval 12					//interval 2^(12-1)ms

const uint8_t DD_array[DD_bLength] = {DD_bLength, DD_bDescriptorType, DD_bcdUSB_L, DD_bcdUSB_H, DD_bDeviceClass, DD_bDeviceSubClass, DD_bDeviceProtocol, DD_bMaxPacketSize0, DD_idVendor_L, DD_idVendor_H, DD_idProduct_L, DD_idProduct_H, DD_bcdDevice_L, DD_bcdDevice_H, DD_iManufacturer, DD_iProduct, DD_iSerialNumber, DD_bNumConfigurations};
const uint8_t CD_array[CD_bLength] = {CD_bLength, CD_bDescriptorType, CD_wTotalLength_L, CD_wTotalLength_H, CD_bNumInterfaces, CD_bConfigurationValue, CD_iConfiguration, CD_bmAttributes, CD_bMaxPower};				  
const uint8_t ID_array[ID_bLength] = {ID_bLength, ID_bDescriptorType, ID_bInterfaceNumber, ID_bAlternateSetting, ID_bNumEndpoints, ID_bInterfaceClass, ID_bInterfaceSubClass, ID_bInterfaceProtocol, ID_iInterface};
const uint8_t ED_array[ED_bLength] = {ED_bLength, ED_bDescriptorType, ED_bEndpointAddress, ED_bmAttributes, ED_wMaxPacketSize_L, ED_wMaxPacketSize_H, ED_bInterval};
	