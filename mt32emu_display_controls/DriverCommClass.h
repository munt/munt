#ifndef _DRIVER_COMM_CLASS_H_
#define _DRIVER_COMM_CLASS_H_

#include <stdlib.h>
#include <string.h>

struct _UDPsocket {
	char buffer[4096];
};

#include "SDL_net.h"


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


private:

	IPaddress driverAddr;
	IPaddress clientAddr;
	UDPsocket clientSock;
	int clientChannel;
	UDPpacket *inPacket;


};

#endif