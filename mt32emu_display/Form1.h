#pragma once

#include <stdlib.h>


struct _UDPsocket {
	char buffer[4096];
};

#include "SDL_net.h"

IPaddress driverAddr;
IPaddress clientAddr;
UDPsocket clientSock;
int clientChannel;


namespace mt32emu_display
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary> 
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the 
	///          'Resource File Name' property for the managed resource compiler tool 
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public __gc class Form1 : public System::Windows::Forms::Form
	{	
	public:
		Form1(void)
		{
			InitializeComponent();
		}
  
	protected:
		void Dispose(Boolean disposing)
		{

			SDLNet_UDP_Unbind(clientSock, clientChannel);
			SDLNet_UDP_Close(clientSock);
			SDLNet_Quit();

			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		}
	private: System::Windows::Forms::GroupBox *  groupBox1;
	private: System::Windows::Forms::PictureBox *  pictureBox[];
	private: System::Windows::Forms::Timer *  timer1;
	private: System::Windows::Forms::PictureBox *  pictureBox1;
	private: System::Windows::Forms::GroupBox *  groupBox2;
	private: System::ComponentModel::IContainer *  components;






	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = new System::ComponentModel::Container();
			System::Resources::ResourceManager *  resources = new System::Resources::ResourceManager(__typeof(mt32emu_display::Form1));
			this->groupBox1 = new System::Windows::Forms::GroupBox();
			this->timer1 = new System::Windows::Forms::Timer(this->components);
			this->pictureBox1 = new System::Windows::Forms::PictureBox();
			this->groupBox2 = new System::Windows::Forms::GroupBox();
			this->SuspendLayout();
			// 
			// groupBox1
			// 
			this->groupBox1->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->groupBox1->Location = System::Drawing::Point(8, 144);
			this->groupBox1->Name = S"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(152, 96);
			this->groupBox1->TabIndex = 0;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = S"Partial Status";
			// 
			// timer1
			// 
			this->timer1->Interval = 10;
			this->timer1->Tick += new System::EventHandler(this, timer1_Tick);
			// 
			// pictureBox1
			// 
			this->pictureBox1->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"pictureBox1.BackgroundImage")));
			this->pictureBox1->Location = System::Drawing::Point(0, 0);
			this->pictureBox1->Name = S"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(800, 134);
			this->pictureBox1->TabIndex = 1;
			this->pictureBox1->TabStop = false;
			// 
			// groupBox2
			// 
			this->groupBox2->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->groupBox2->Location = System::Drawing::Point(168, 144);
			this->groupBox2->Name = S"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(624, 216);
			this->groupBox2->TabIndex = 2;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = S"Channel Status";
			// 
			// Form1
			// 
			this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
			this->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->ClientSize = System::Drawing::Size(800, 398);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->groupBox1);
			this->Icon = (__try_cast<System::Drawing::Icon *  >(resources->GetObject(S"$this.Icon")));
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = S"Form1";
			this->Text = S"MUNT Control Panel";
			this->Load += new System::EventHandler(this, Form1_Load_1);
			this->ResumeLayout(false);

		}	

	private: System::Void Form1_Load_1(System::Object *  sender, System::EventArgs *  e)
			 {
				
				this->pictureBox = new System::Windows::Forms::PictureBox*[32];
				int i;
				int x = 16;
				int y = 24;
				int xcount = 0;
				for(i=0;i<32;i++) {
					this->pictureBox[i] = new System::Windows::Forms::PictureBox();
					this->groupBox1->Controls->Add(this->pictureBox[i]);
					this->pictureBox[i]->BackColor = System::Drawing::Color::Black;
					this->pictureBox[i]->Location = System::Drawing::Point(x, y);
					this->pictureBox[i]->Name = S"pictureBox1";
					this->pictureBox[i]->Size = System::Drawing::Size(8, 8);
					this->pictureBox[i]->TabIndex = 1;
					this->pictureBox[i]->TabStop = false;
					x += 16;
					xcount++;
					if(xcount > 7) {
						x = 16;
						y += 16;
						xcount = 0;

					}
				}


				if(SDLNet_Init() == 0) {


					driverAddr.host = 0x0100007f;
					driverAddr.port = 0xc307;

					//if(SDLNet_ResolveHost(&driverAddr, "127.0.0.1", 1987) == 0) {

						clientSock = SDLNet_UDP_Open(0);
						if(clientSock != NULL) {
							clientChannel = SDLNet_UDP_Bind(clientSock,-1,&driverAddr);
							if(clientChannel != -1) {
								this->timer1->set_Enabled(true);		
							}
						}
						
						
					//}
					
					
				}


				
			 }


	private: System::Void timer1_Tick(System::Object *  sender, System::EventArgs *  e)
			 {
				Uint16 buffer[2048];
				UDPpacket regPacket;
				UDPpacket inPacket;
				int numrecv = 0;

				// Send heartbeat
				buffer[0] = 1;

				regPacket.data = (Uint8 *)&buffer[0];
				regPacket.len = 2;
				regPacket.maxlen = 2;
				regPacket.channel = clientChannel;
				SDLNet_UDP_Send(clientSock, regPacket.channel, &regPacket);


				buffer[0] = 0;

				inPacket.data = (Uint8 *)buffer;
				inPacket.maxlen = 4096;
				inPacket.channel = clientChannel;

				numrecv = SDLNet_UDP_Recv(clientSock, &inPacket);
				if(numrecv) {
					int i;
					if(buffer[0] == 1) {
						int count = (int)buffer[1];
						for(i=0;i<count;i++) {
							int t;
							t = buffer[2 + i];
							switch(t) {
								case 0:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(0,0,0));
									break;
								case 1:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(255,0,0));
									break;
								case 2:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(0,255,0));
									break;
								case 3:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(255,255,0));
									break;
							}

						}
					}

				}


			 }

};
}


