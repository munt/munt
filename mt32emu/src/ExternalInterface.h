/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_EXTINT_H
#define MT32EMU_EXTINT_H

#include <SDL_net.h>

namespace MT32Emu {

class ExternalInterface {
private:
	Bit8u inBuffer[4096];

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

	void doControlPanelComm(Synth *synth, int sndBufLength);
	bool getStatusRequest(int *requestType, char * buffer);

	bool sendResponse(int requestType, char *requestBuf, int requestLen);

	bool sendDisplayText(char *requestBuf, int requestLen);

	void handleReport(Synth *synth, ReportType type, const void *reportData);

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

}
#endif
