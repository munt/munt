#ifndef _DRIVER_COMM_CLASS_H_
#define _DRIVER_COMM_CLASS_H_

#include <stdlib.h>
#include <string.h>

struct _UDPsocket {
	char buffer[4096];
};

#include "SDL_net.h"


struct chanInfo {
	bool isPlaying;
	int assignedPart;
	int freq;
	int age;
	int vel;
};


class DriverCommClass
{
public:
	
	DriverCommClass() {}
	~DriverCommClass() {}
	

	bool InitConnection();
	void sendHeartBeat();
	void setVolume(int channel, int val);
	void shutDown();
	int checkForData(char * buffer);
	void requestSynthMemory(int addr, int len);
	void writeSynthMemory(int addr, int len, char *outBuf);
	void sendSemiRawSysex(unsigned char * sysexBuf, int sysexLen);


private:

	IPaddress driverAddr;
	IPaddress clientAddr;
	UDPsocket clientSock;
	int clientChannel;
	UDPpacket *inPacket;


};

#endif