#include "stdafx.h"
#include "DriverCommClass.h"

bool DriverCommClass::InitConnection() {
	if(SDLNet_Init() == 0) {


		this->driverAddr.host = 0x0100007f;
		//this->driverAddr.host = 0xffffffff;
		this->driverAddr.port = 0xc307;

		this->clientSock = SDLNet_UDP_Open(0);
		if(this->clientSock != NULL) {
			this->clientChannel = SDLNet_UDP_Bind(this->clientSock,-1,&this->driverAddr);
			if(this->clientChannel != -1) {
				this->inPacket = SDLNet_AllocPacket(4096);
				if(this->inPacket != NULL) {
					return true;

				}
			}
		}
		
	}
	return false;
}


void DriverCommClass::setVolume(int channel, int val) {
	char buffer[2048];
	UDPpacket regPacket;
	int numrecv = 0;

	// Send volume message
	*(Uint16 *)(&buffer[0]) = 4;
	buffer[2] = (char)channel;
	buffer[3] = 0xB;
	buffer[4] = 0x7;
	buffer[5] = (char)val;

	regPacket.data = (Uint8 *)&buffer[0];
	regPacket.len = 6;
	regPacket.maxlen = 6;
	regPacket.channel = clientChannel;
	SDLNet_UDP_Send(clientSock, regPacket.channel, &regPacket);

}

void DriverCommClass::sendHeartBeat() {
	Uint16 buffer[2048];
	UDPpacket regPacket;

	// Send heartbeat
	buffer[0] = 1;

	regPacket.data = (Uint8 *)&buffer[0];
	regPacket.len = 2;
	regPacket.maxlen = 2;
	regPacket.channel = this->clientChannel;
	SDLNet_UDP_Send(this->clientSock, regPacket.channel, &regPacket);

}

void DriverCommClass::shutDown() {
	SDLNet_UDP_Unbind(this->clientSock, this->clientChannel);
	SDLNet_UDP_Close(this->clientSock);
	SDLNet_Quit();
}

int DriverCommClass::checkForData(char * buffer) {
	this->inPacket->channel = this->clientChannel;

	int numrecv = SDLNet_UDP_Recv(this->clientSock, this->inPacket);
	if(numrecv > 0) {
		memcpy(buffer, this->inPacket->data, this->inPacket->len);
		return this->inPacket->len;
	} else {
		return 0;
	}
}

void DriverCommClass::requestSynthMemory(int addr, int len) {
	char buffer[2048];
	UDPpacket regPacket;

	// Synth memory request
	*(Uint16 *)&buffer[0] = 5;
	*(Uint32 *)&buffer[2] = addr;
	*(Uint16 *)&buffer[6] = (Uint16)len;

	regPacket.data = (Uint8 *)&buffer[0];
	regPacket.len = 8;
	regPacket.maxlen = 8;
	regPacket.channel = this->clientChannel;
	SDLNet_UDP_Send(this->clientSock, regPacket.channel, &regPacket);

}

void DriverCommClass::writeSynthMemory(int addr, int len, char *outBuf) {
	char buffer[2048];
	UDPpacket regPacket;

	// Synth memory write
	*(Uint16 *)&buffer[0] = 2;
	*(Uint16 *)&buffer[2] = (Uint16)len + 3;
	buffer[4] = (addr >> 16) & 0x7f;
	buffer[5] = (addr >> 8) & 0x7f;
	buffer[6] = addr & 0x7f;

	memcpy(&buffer[7], outBuf, len);

	regPacket.data = (Uint8 *)&buffer[0];
	regPacket.len = len + 7;
	regPacket.maxlen = len + 7;
	regPacket.channel = this->clientChannel;
	SDLNet_UDP_Send(this->clientSock, regPacket.channel, &regPacket);
}
