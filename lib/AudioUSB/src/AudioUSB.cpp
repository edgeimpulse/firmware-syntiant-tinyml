/*
** Copyright (c) 2015, Gary Grewal
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "AudioUSB.h"

#define Audio_AC_INTERFACE 	pluggedInterface	// Audio AC Interface
#define Audio_INTERFACE 		((uint8_t)(pluggedInterface+1))
#define Audio_STREAMING_INTERFACE 	((uint8_t)(pluggedInterface+2))

#define Audio_FIRST_ENDPOINT pluggedEndpoint
#define Audio_ENDPOINT_OUT	pluggedEndpoint
#define Audio_ENDPOINT_IN	((uint8_t)(pluggedEndpoint+1))

#define Audio_RX Audio_ENDPOINT_OUT
#define Audio_TX Audio_ENDPOINT_IN

Audio_ AudioUSB;

int Audio_::getInterface(uint8_t* interfaceNum)
{
	interfaceNum[0] += 2;	// uses 2 interfaces
	//interfaceNum[0] += 3;	// uses 3 interfaces
	
	AudioDescriptor _AudioInterface __attribute__((aligned(4))) =
	{
		D_IAD(Audio_AC_INTERFACE, 2, Audio_AUDIO, Audio_AUDIO_CONTROL, 0),
		//D_IAD(Audio_AC_INTERFACE, 3, Audio_AUDIO, Audio_AUDIO_CONTROL, 0),	
		D_INTERFACE(Audio_AC_INTERFACE,0,Audio_AUDIO,Audio_AUDIO_CONTROL,0),					
		D_AC_INTERFACE(0x1, Audio_INTERFACE),
		D_Audio_INJACK(Audio_JACK_INPUT_TERMINAL_DESCRIPTOR, 0x1),
		D_Audio_OUTJACK(Audio_JACK_OUTPUT_TERMINAL_DESCRIPTOR, 0x2, 1, 1, 1),
		D_INTERFACE_ZERO(Audio_INTERFACE),
		D_INTERFACE_ONE(Audio_INTERFACE),
		D_AS_INTERFACE,
		D_AS_FORMAT,
		D_Audio_JACK_EP(USB_ENDPOINT_IN(Audio_ENDPOINT_OUT),USB_ENDPOINT_TYPE_ISOCHRONOUS,Audio_BUFFER_SIZE),
		D_AUDIO_DATA_EP	
	};
	return USB_SendControl2(0, &_AudioInterface, sizeof(_AudioInterface));
}

bool Audio_::setup(USBSetup& setup __attribute__((unused)))
{
	//Support requests here if needed. Typically these are optional
	return false;
}

int Audio_::getDescriptor(USBSetup& setup __attribute__((unused)))
{
	return 0;
}

uint8_t Audio_::getShortName(char* name)
{
	memcpy(name, "Audio", 5);
	return 5;
}

size_t Audio_::write(const int16_t *buffer, size_t size)
{
	USBDevice.send(pluggedEndpoint, buffer, size);
	return 1;

	/* only try to send bytes if the high-level Audio connection itself
	 is open (not just the pipe) - the OS should set lineState when the port
	 is opened and clear lineState when the port is closed.
	 bytes sent before the user opens the connection or after
	 the connection is closed are lost - just like with a UART. */
}

Audio_::Audio_(void) : PluggableUSBModule(1, 2, epType)
//Audio_::Audio_(void) : PluggableUSBModule(2, 3, epType)
{
	epType[0] = EP_TYPE_ISOCHRONOUS_IN_Audio;		// Audio_ENDPOINT_IN
	PluggableUSB().plug(this);
}

