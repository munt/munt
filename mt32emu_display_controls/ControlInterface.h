#pragma once

#include "DriverCommClass.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Diagnostics;

 
namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for ControlInterface
	/// </summary>
	__gc public class ControlInterface :  public System::ComponentModel::Component
	{
	public:
		ControlInterface(void)
		{
			InitializeComponent();
			this->driverComm = new DriverCommClass();
		}
		ControlInterface(System::ComponentModel::IContainer *container) : components(0)
		{
			/// <summary>
			/// Required for Windows.Forms Class Composition Designer support
			/// </summary>

			container->Add(this);
			InitializeComponent();
			this->driverComm = new DriverCommClass();
		}

		System::Void setVolume(int channel, int volume) { this->driverComm->setVolume(channel, volume); }

		System::Boolean InitConnection() { return this->driverComm->InitConnection(); }

		System::Void sendHeartBeat() { this->driverComm->sendHeartBeat(); }

		System::Void shutDown() { this->driverComm->shutDown(); }

		System::Int32 checkForData(char * buffer) { return this->driverComm->checkForData(buffer); }

		System::Void requestSynthMemory(int addr, int len) { this->driverComm->requestSynthMemory(addr, len); }
		
	protected: 
		void Dispose(Boolean disposing)
		{
			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		}

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container *components;
		DriverCommClass * driverComm;
		
		
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>		
		void InitializeComponent(void)
		{
		}
	};
}