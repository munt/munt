#ifndef MT32EMU_EXTINT_H
#define MT32EMU_EXTINT_H

#include <SDL_net.h>

class ExternalInterface {
public:
	ExternalInterface() {
		SDLNet_Init();
		this->openedPort = false;
		this->textToDisplay = false;
		this->knownClient = false;
	}

	~ExternalInterface() {
		SDLNet_Quit();
	}

	

	bool start();

	void doControlPanelComm(MT32Emu::Synth *synth);
	bool getStatusRequest(int *requestType);

	bool sendResponse(int requestType, char *requestBuf, int requestLen);

	bool sendDisplayText(char *requestBuf, int requestLen);

	bool stop();
private:
	bool openedPort;
	char txtBuffer[512];
	bool textToDisplay;
	bool knownClient;
	IPaddress ipxServerIp;  // IPAddress for server's listening port
	IPaddress ipxClientIp;  // IPAddress for last accessed client request
	UDPsocket ipxServerSocket;  // Listening server socket
	UDPpacket *regPacket;
};

#endif