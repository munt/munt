/* Copyright (c) 2003-2004 Various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mt32emu.h"

#if USE_COMM == 1

MT32Emu::Bit8u inBuffer[4096];

bool externalInterface::start() {
	this->ipxServerIp.host = 0x0100007f;
	this->ipxServerIp.port = 0xc307;

	if(!SDLNet_ResolveHost(&ipxServerIp, NULL, 0xc307)) {
		ipxServerSocket = SDLNet_UDP_Open(1987);
		if(ipxServerSocket == NULL) return false;

		this->openedPort = true;
		return true;
	} else {
        return false;
	}
}

bool externalInterface::getStatusRequest(int *requestType) {
	int result;
	UDPpacket inPacket;

	inPacket.channel = -1;
	inPacket.data = &inBuffer[0];
	inPacket.maxlen = 4096;
	result = SDLNet_UDP_Recv(ipxServerSocket, &inPacket);
	if (result != 0) {
		this->ipxClientIp = inPacket.address;
		*requestType = (int)*(MT32Emu::Bit16u *)(&inBuffer[0]);
		return true;
	} else {
        return false;
	}
}

bool externalInterface::sendResponse(int requestType, char *requestBuf, int requestLen) {
	UDPpacket regPacket;

	regPacket.data = (Uint8 *)requestBuf;
	regPacket.len = requestLen;
	regPacket.maxlen = requestLen;
	regPacket.address = this->ipxClientIp;
	SDLNet_UDP_Send(ipxServerSocket,-1,&regPacket);

	return true;
}

bool externalInterface::stop() {
	if(this->openedPort) {
		if(ipxServerSocket != NULL) {
		SDLNet_UDP_Close(ipxServerSocket);
		}
		this->openedPort = false;
	}
	return true;
}


#endif

