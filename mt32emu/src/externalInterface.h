#ifndef MT32EMU_EXTINT_H
#define MT32EMU_EXTINT_H

#if USE_COMM == 1

#include <SDL_net.h>
#include "structures.h"

class externalInterface {
public:
	externalInterface() {
		SDLNet_Init();
		this->openedPort = false;
	}

	~externalInterface() {
		SDLNet_Quit();
	}

	

	bool start();

	bool getStatusRequest(int *requestType);

	bool sendResponse(int requestType, char *requestBuf, int requestLen);

	bool stop();
private:
	bool openedPort;
	IPaddress ipxServerIp;  // IPAddress for server's listening port
	IPaddress ipxClientIp;  // IPAddress for last accessed client request
	UDPsocket ipxServerSocket;  // Listening server socket
};

#endif

#endif