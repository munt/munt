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

#include <string.h>

#include "mt32emu.h"

#if MT32EMU_USE_EXTINT == 1

#include "externalInterface.h"

namespace MT32Emu {

bool ExternalInterface::start() {
	this->ipxServerIp.host = 0x0100007f;
	this->ipxServerIp.port = 0xc307;

	if(!SDLNet_ResolveHost(&ipxServerIp, NULL, 0xc307)) {
		ipxServerSocket = SDLNet_UDP_Open(1987);
		if(ipxServerSocket == NULL) return false;
		regPacket = SDLNet_AllocPacket(4096);
		if(regPacket == NULL) return false;

		this->openedPort = true;
		return true;
	} else {
        return false;
	}
}

void ExternalInterface::doControlPanelComm(Synth *synth) {
	int reqType;
	int i;
	Bit8u buffer[4096];
	if(getStatusRequest(&reqType)) {
		switch(reqType) {
			case 1:
				Bit16u *bufptr;
				bufptr = (Bit16u *)(&buffer[0]);
				*bufptr++ = (Bit16u)reqType;
				*bufptr++ = (Bit16u)MT32EMU_MAX_PARTIALS;
				for(i=0;i<MT32EMU_MAX_PARTIALS;i++) {
					if(!synth->getPartial(i)->play) {
						*bufptr++ = 0;
						*bufptr++ = 0;
						*bufptr++ = 0;
					} else {
						if(synth->getPartial(i)->envs[EnvelopeType_amp].decaying) {
							*bufptr++ = 3;
						} else {
							if(synth->getPartial(i)->envs[EnvelopeType_amp].envstat == 4) {
								*bufptr++ = 2;
							} else {
								*bufptr++ = 1;
							}
						}
						*bufptr++ = (Bit16u)synth->getPartial(i)->getOwnerPart();
						*bufptr++ = (Bit16u)synth->getPartial(i)->getKey();
					}
				}
				// 8 channel names with description
				*bufptr++ = 8;
				for(i=0;i<8;i++) {
					memcpy(bufptr, synth->getPart(i)->getCurrentInstr(), 10);
					bufptr++;
					bufptr++;
					bufptr++;
					bufptr++;
					bufptr++;
				}

				sendResponse(reqType, (char *)&buffer[0], 278 );
				break;
		}

	}
}

bool ExternalInterface::getStatusRequest(int *requestType) {
	int result;
	UDPpacket inPacket;

	inPacket.channel = -1;
	inPacket.data = &inBuffer[0];
	inPacket.maxlen = 4096;
	result = SDLNet_UDP_Recv(ipxServerSocket, &inPacket);
	if (result != 0) {
		this->ipxClientIp = inPacket.address;
		this->knownClient = true;
		*requestType = (int)*(Bit16u *)(&inBuffer[0]);
		return true;
	} else {
        return false;
	}
}

bool ExternalInterface::sendResponse(int requestType, char *requestBuf, int requestLen) {

	memcpy(regPacket->data, requestBuf, requestLen);
	regPacket->len = requestLen;
	//regPacket->address.host = 0xffffffff;
	//regPacket->address.port = 0x641e;
	regPacket->address = this->ipxClientIp;
	SDLNet_UDP_Send(ipxServerSocket,-1,regPacket);

	if((this->knownClient) && (this->textToDisplay)) {

		memcpy(regPacket->data, txtBuffer, requestLen);
		regPacket->len = 22;
		regPacket->address = this->ipxClientIp;
		//regPacket->address.host = 0xffffffff;
		//regPacket->address.port = 0x641e;
		SDLNet_UDP_Send(ipxServerSocket,-1,regPacket);
		this->textToDisplay = false;
	}

	return true;
}

bool ExternalInterface::sendDisplayText(char *requestBuf, int requestLen) {
	if(!this->knownClient) {
        memcpy(&txtBuffer[0], requestBuf, requestLen);
		this->textToDisplay = true;
	} else {
		this->sendResponse(2, requestBuf, requestLen);
	}
	return true;
}

bool ExternalInterface::stop() {
	if(this->openedPort) {
		if(ipxServerSocket != NULL) {
		SDLNet_UDP_Close(ipxServerSocket);
		}
		this->openedPort = false;
	}
	return true;
}

void ExternalInterface::handleReport(Synth *synth, ReportType type, const void *reportData) {
	switch (type) {
		case ReportType_lcdMessage:
		{
			char buf[514]; //FIXME: Messy...
			*(Bit16u *)(&buf[0]) = 2;
			strcpy(&buf[2], (const char *)reportData);
			sendResponse(2, &buf[0], (int)strlen((const char *)reportData) + 3);
		}
	}
}

}

#endif
