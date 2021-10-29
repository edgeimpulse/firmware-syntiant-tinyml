//================================================================================
//================================================================================

/**
	Audio USB class
*/
#ifndef AudioUSB_h
#define AudioUSB_h

#include <stdint.h>
#include <Arduino.h>

#if ARDUINO < 10606
#error AudioUSB requires Arduino IDE 1.6.6 or greater. Please update your IDE.
#endif

#if !defined(USBCON)
#error AudioUSB can only be used with an USB MCU.
#endif

#if defined(ARDUINO_ARCH_AVR)

#include "PluggableUSB.h"

#define EPTYPE_DESCRIPTOR_SIZE uint8_t
//#define EP_TYPE_ISOCHRONOUS_IN_Audio 		EP_TYPE_ISOCHRONOUS_IN
//#define EP_TYPE_ISOCHRONOUS_OUT_Audio 		EP_TYPE_ISOCHRONOUS_OUT
#define EP_TYPE_INTERRUPT_OUT_Audio EP_TYPE_INTERRUPT_OUT
#define EP_TYPE_BULK_IN_Audio EP_TYPE_BULK_IN
#define Audio_BUFFER_SIZE USB_EP_SIZE
#define is_write_enabled(x) (1)

#elif defined(ARDUINO_ARCH_SAM)

#include "USB/PluggableUSB.h"

#define EPTYPE_DESCRIPTOR_SIZE uint32_t
#define EP_TYPE_ISOCHRONOUS_IN_Audio (UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
									  UOTGHS_DEVEPTCFG_EPDIR_IN |        \
									  UOTGHS_DEVEPTCFG_EPTYPE_ISO |      \
									  UOTGHS_DEVEPTCFG_EPBK_1_BANK |     \
									  UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS | \
									  UOTGHS_DEVEPTCFG_ALLOC)
#define EP_TYPE_ISOCHRONOUS_OUT_Audio (UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
									   UOTGHS_DEVEPTCFG_EPTYPE_ISO |      \
									   UOTGHS_DEVEPTCFG_EPBK_1_BANK |     \
									   UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS | \
									   UOTGHS_DEVEPTCFG_ALLOC)
#define EP_TYPE_INTERRUPT_OUT_Audio (UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
									 UOTGHS_DEVEPTCFG_EPTYPE_ISO |      \
									 UOTGHS_DEVEPTCFG_EPBK_1_BANK |     \
									 UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS | \
									 UOTGHS_DEVEPTCFG_ALLOC)
#define EP_TYPE_BULK_IN_Audio (UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
							   UOTGHS_DEVEPTCFG_EPTYPE_BULK |     \
							   UOTGHS_DEVEPTCFG_EPBK_1_BANK |     \
							   UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS | \
							   UOTGHS_DEVEPTCFG_ALLOC)
#define Audio_BUFFER_SIZE EPX_SIZE
#define USB_SendControl USBD_SendControl
#define USB_Available USBD_Available
#define USB_Recv USBD_Recv
#define USB_Send USBD_Send
#define USB_Flush USBD_Flush
#define is_write_enabled(x) Is_udd_write_enabled(x)

#elif defined(ARDUINO_ARCH_SAMD)

#include "USB/PluggableUSB.h"

#define EPTYPE_DESCRIPTOR_SIZE uint32_t
#define EP_TYPE_ISOCHRONOUS_IN_Audio USB_ENDPOINT_TYPE_ISOCHRONOUS | USB_ENDPOINT_IN(0);
#define EP_TYPE_ISOCHRONOUS_OUT_Audio USB_ENDPOINT_TYPE_ISOCHRONOUS | USB_ENDPOINT_OUT(0);
#define EP_TYPE_INTERRUPT_IN_Audio USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0);
#define EP_TYPE_BULK_IN_Audio USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0);
#define Audio_BUFFER_SIZE EPX_SIZE
// mpw #define USB_SendControl				USBDevice.sendControl
#define USB_SendControl2 USBDevice.sendControl
#define USB_Available USBDevice.available
#define USB_Recv USBDevice.recv
#define USB_Send USBDevice.send
#define USB_Flush USBDevice.flush
//#define USB_handleEndpoint				USBDevice.handleEndpoint // MPW

#define is_write_enabled(x) (1)

#else

#error "Unsupported architecture"

#endif

#define Audio_AUDIO 0x01
#define Audio_AUDIO_CONTROL 0x01
#define Audio_STREAMING 0x2
#define INTERFACE 0x4

#define Audio_CS_INTERFACE 0x24
#define Audio_CS_ENDPOINT 0x25
#define Audio_JACK_INPUT_TERMINAL_DESCRIPTOR 0X02
#define Audio_JACK_OUTPUT_TERMINAL_DESCRIPTOR 0X03

_Pragma("pack(1)")
	/// Audio Audio Control Interface Descriptor
	typedef struct
{
	uint8_t len;   // 9
	uint8_t dtype; // 4
	uint8_t dsubType;
	uint16_t bcdADc;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t interfaceNumbers;
} Audio_ACInterfaceDescriptor;

typedef struct
{
	uint8_t len;	  // B
	uint8_t dtype;	  // 24
	uint8_t dsubType; // 2
	uint8_t formatType;
	uint8_t channels;
	uint8_t SubFrameSize;
	uint8_t BitResolution;
	uint8_t SamFreqType;
	uint8_t SamFreq0;
	uint8_t SamFreq1;
	uint8_t SamFreq2;
} Audio_ACFormatDescriptor; // MPW

typedef struct
{
	uint8_t len;			  // 9
	uint8_t dtype;			  // 4
	uint8_t bInterfaceNumber; // 2
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubclass;
	uint8_t bInterfaceProtocol; // unused
	uint8_t iInterface;			// unused
} Audio_AC_Interface_Zero;		// MPW

typedef struct
{
	uint8_t len;   // 9
	uint8_t dtype; // 4
	uint8_t dsubType;
	uint8_t jackType;
	uint8_t jackID;
	uint8_t nPins;
	uint8_t srcJackID;
	uint8_t srcPinID;
	uint8_t jackStrIndex;
} Audio_ACEndpointDescriptor; // MPW

typedef struct
{
	uint8_t len;	  // 7
	uint8_t dtype;	  // 25
	uint8_t dsubType; // 1
	uint8_t Attributes;
	uint8_t LockDelayUnits;
	uint8_t lockDelay0;
	uint8_t lockDelay1;
} Audio_AC_Audio_Data_Descriptor; // MPW

typedef struct
{
	uint8_t len;   // c
	uint8_t dtype; // 24
	uint8_t dsubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bNrChannels;
	uint16_t wChannelConfig;
	uint8_t iChannelNames;
	uint8_t iTerminal;
} AudioJackinDescriptor;

typedef struct
{
	uint8_t len;   // 9
	uint8_t dtype; // 24
	uint8_t dsubType;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
} AudioJackOutDescriptor;

/// Audio Jack EndPoint Descriptor, common to Audio in and out jacks.
typedef struct
{
	EndpointDescriptor len; // 9
	uint8_t refresh;		// 4
	uint8_t sync;
} Audio_EPDescriptor;

typedef struct
{
	uint8_t len;   // 9
	uint8_t dtype; // 2
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t MaxPower;
} Configuration_Descriptor;

/// Audio Jack  EndPoint AudioControl Descriptor, common to Audio in and out ac jacks.
typedef struct
{
	uint8_t len;   // 5
	uint8_t dtype; // 0x24
	uint8_t subtype;
	uint8_t embJacks;
	uint8_t jackID;
} Audio_EP_ACDescriptor;

/// Audio Audio Stream Descriptor Interface
typedef struct
{
	uint8_t len;   // 9
	uint8_t dtype; // 4
	uint8_t dsubType;
	uint16_t bcdADc;
	uint16_t wTotalLength;
} Audio_ASInterfaceDescriptor;

/// Top Level Audio Descriptor used to create a Audio Interface instace \see Audio_::getInterface()
typedef struct
{
	//	IAD
	IADDescriptor iad;
	InterfaceDescriptor Audio_ControlInterface;
	Audio_ACInterfaceDescriptor Audio_ControlInterface_SPC;
	AudioJackinDescriptor Audio_In_Jack_Ext;
	AudioJackOutDescriptor Audio_Out_Jack_Ext;
	Audio_AC_Interface_Zero Audio_StreamInterface_2;
	InterfaceDescriptor Audio_StreamInterface;
	Audio_ASInterfaceDescriptor Audio_StreamInterface_SPC;
	Audio_ACFormatDescriptor Audio_Stream_Format; // MPW
	Audio_EPDescriptor Audio_In_Jack_Endpoint;
	Audio_AC_Audio_Data_Descriptor AudioDataEPDescriptor;
} AudioDescriptor;

// AudioStreaming Interface Descriptor
// Zero-bandwidth Alternate Setting 0
// OR Standard AS Interface Descriptor
// Audio_CS_INTERFACE = 0x24
// Audio_CS_ENDPOINT = 0x25
// INTERFACE = 0x04
// ENDPOINT = 0x5
// STRING = 0x03

// Audio Control Interface
#define D_AC_INTERFACE(_streamingInterfaces, _AudioInterface)                                        \
	{                                                                                                \
		9, Audio_CS_INTERFACE, 0x1, 0x0100, 0x001e, _streamingInterfaces, (uint8_t)(_AudioInterface) \
	}

//Input Terminal Descriptor
#define D_Audio_INJACK(jackProp, _jackID)                                            \
	{                                                                                \
		0x0c, Audio_CS_INTERFACE, jackProp, _jackID, 0x0201, 0x00, 0x01, 0x0000, 0x00, 0x00 \
	}

//Output Terminal Descriptor
#define D_Audio_OUTJACK(jackProp, _jackID, _nPins, _srcID, _srcPin)   \
	{                                                                 \
		0x09, Audio_CS_INTERFACE, jackProp, _jackID, 0x0101, 0x00, 0x01, 0x00 \
	}

// Zero Bandwidth Microphone Interface
#define D_INTERFACE_ZERO(_AudioInterface)            \
	{                               \
		9, INTERFACE, (uint8_t)(_AudioInterface), 0, 0, 1, 2, 0, 0 \
	}

// Normal Bandwidth Microphone Interface
#define D_INTERFACE_ONE(_AudioInterface)             \
	{                               \
		9, INTERFACE, (uint8_t)(_AudioInterface), 1, 1, 1, 2, 0, 0 \
	}

// Class-specific AS General Interface Descriptor
#define D_AS_INTERFACE                                \
	{                                                 \
		0x7, Audio_CS_INTERFACE, 0x01, 0x0102, 0x0001 \
	}

// Type I Format Type Descriptor
#define D_AS_FORMAT                                                                    \
	{                                                                                  \
		0x0B, Audio_CS_INTERFACE, 0x02, 0x01, 0x01, 0x02, 0x10, 0x01, 0x80, 0x3e, 0x00 \
	} // 0x80, 0x3e = 16,000Hz

#define D_Audio_JACK_EP(_addr, _attr, _packetSize) \
	{                                              \
		9, USB_ENDPOINT_DESCRIPTOR_TYPE, _addr, _attr, _packetSize, 1, 0, 0            \
	}

#define D_AUDIO_DATA_EP                     \
	{                                       \
		7, Audio_CS_ENDPOINT, 1, 0, 0, 0, 0 \
	}

#define D_AS_MIDI_INTERFACE                           \
	{                                                 \
		0x7, Audio_CS_INTERFACE, 0x01, 0x0100, 0x0041 \
	}

#ifndef DOXYGEN_ARD
// the following would confuse doxygen documentation tool, so skip in that case for autodoc build
_Pragma("pack()")

#define WEAK __attribute__((weak))

#endif

	/**
 	 Concrete Audio implementation of a PluggableUSBModule
 	 By default, will define one Audio in and one Audio out enpoints.
 */
	class Audio_ : public PluggableUSBModule
{
	// private:
	// 	RingBuffer *_Audio_rx_buffer;
private:
	void accept(void); ///< Accepts a Audio packet, \see Audio_::read()
	//EPTYPE_DESCRIPTOR_SIZE epType[2];   ///< Container that defines the two isochronous Audio IN/OUT endpoints types
	EPTYPE_DESCRIPTOR_SIZE epType[3]; ///< Container that defines the two isochronous Audio IN/OUT endpoints types & one bulk for HID

protected:
	// Implementation of the PUSBListNode

	/// Creates a AudioDescriptor Audio interface and sollicit USBDevice to send control to it.
	///   \see USBDevice::SendControl()
	int getInterface(uint8_t *interfaceNum);
	/// Current implementation just returns 0
	int getDescriptor(USBSetup &setup);
	/// Optional interface usb setup callback, current implementation returns false
	bool setup(USBSetup &setup);
	/// Audio Device short name, defaults to "Audio" and returns a length of 5 chars
	//uint8_t getShortName(char* name);

public:
	/// Creates a Audio USB device with 2 endpoints
	Audio_(void);
	/// Audio Device short name, defaults to "Audio" and returns a length of 4 chars
	uint8_t getShortName(char *name);
	/// Returns the number of bytes currently available from RX
	//uint32_t available(void);
	/// Reads a new Audio message from USB
	//AudioEventPacket_t read(void);
	/// Flushes TX Audio channel
	//void flush(void);
	/// Sends a Audio message to USB
	//void sendAudio(AudioEventPacket_t event);
	/// Sends a Audio buffer of length size to USB
	size_t write(const int16_t *buffer, size_t size);
	/// NIY
	operator bool();
};
extern Audio_ AudioUSB;

#endif /* AudioUSB_h */
