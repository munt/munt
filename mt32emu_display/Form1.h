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
UDPpacket *inPacket;

struct chanInfo {
	bool isPlaying;
	int assignedPart;
	int freq;
};

chanInfo chanList[32];

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
			
			channelStatus->Tag = ci;

		}


		/*
		void SetVolume(int channel, int val) {
			
				char buffer[2048];
				UDPpacket regPacket;
				int numrecv = 0;

				// Send volume message
				buffer[0] = 4;
				buffer[1] = (char)channel;
				buffer[2] = 0xB;
				buffer[3] = 0x7;
				buffer[4] = (char)val;

				regPacket.data = (Uint8 *)&buffer[0];
				regPacket.len = 5;
				regPacket.maxlen = 5;
				regPacket.channel = clientChannel;
				SDLNet_UDP_Send(clientSock, regPacket.channel, &regPacket);

		}
		*/
  
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





	private: System::Windows::Forms::GroupBox *  groupBox3;
	private: System::Windows::Forms::PictureBox *  oscoIcon;
	private: System::Windows::Forms::PictureBox *  channelIcon;
	private: System::Windows::Forms::PictureBox *  settingsIcon;
	private: System::Windows::Forms::Label *  label1;
	private: System::Windows::Forms::ToolTip *  channelTip;
	private: System::Windows::Forms::ToolTip *  oscoTip;
	private: System::Windows::Forms::ToolTip *  settingsTip;

	private: System::Windows::Forms::ToolTip *  partialTip;
	private: System::Int32 currentTab;
	private: mt32emu_display_controls::FacePlate *  facePlate;
	private: mt32emu_display_controls::ChannelDisplay *  channelStatus;
	private: mt32emu_display_controls::ControlInterface * ci;








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
			this->groupBox3 = new System::Windows::Forms::GroupBox();
			this->settingsIcon = new System::Windows::Forms::PictureBox();
			this->oscoIcon = new System::Windows::Forms::PictureBox();
			this->channelIcon = new System::Windows::Forms::PictureBox();
			this->label1 = new System::Windows::Forms::Label();
			this->channelTip = new System::Windows::Forms::ToolTip(this->components);
			this->oscoTip = new System::Windows::Forms::ToolTip(this->components);
			this->settingsTip = new System::Windows::Forms::ToolTip(this->components);
			this->partialTip = new System::Windows::Forms::ToolTip(this->components);
			this->facePlate = new mt32emu_display_controls::FacePlate();
			this->channelStatus = new mt32emu_display_controls::ChannelDisplay();
			this->ci = new mt32emu_display_controls::ControlInterface();
			this->groupBox3->SuspendLayout();
			this->SuspendLayout();
			// 
			// groupBox1
			// 
			this->groupBox1->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->groupBox1->Location = System::Drawing::Point(8, 288);
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
			// groupBox3
			// 
			this->groupBox3->Controls->Add(this->settingsIcon);
			this->groupBox3->Controls->Add(this->oscoIcon);
			this->groupBox3->Controls->Add(this->channelIcon);
			this->groupBox3->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->groupBox3->Font = new System::Drawing::Font(S"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, (System::Byte)0);
			this->groupBox3->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->groupBox3->Location = System::Drawing::Point(8, 144);
			this->groupBox3->Name = S"groupBox3";
			this->groupBox3->Size = System::Drawing::Size(152, 128);
			this->groupBox3->TabIndex = 23;
			this->groupBox3->TabStop = false;
			this->groupBox3->Text = S"Panel Display";
			// 
			// settingsIcon
			// 
			this->settingsIcon->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->settingsIcon->Cursor = System::Windows::Forms::Cursors::Hand;
			this->settingsIcon->Image = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"settingsIcon.Image")));
			this->settingsIcon->Location = System::Drawing::Point(104, 24);
			this->settingsIcon->Name = S"settingsIcon";
			this->settingsIcon->Size = System::Drawing::Size(32, 32);
			this->settingsIcon->TabIndex = 2;
			this->settingsIcon->TabStop = false;
			this->settingsIcon->Click += new System::EventHandler(this, panel_Click);
			// 
			// oscoIcon
			// 
			this->oscoIcon->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->oscoIcon->Cursor = System::Windows::Forms::Cursors::Hand;
			this->oscoIcon->Image = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"oscoIcon.Image")));
			this->oscoIcon->Location = System::Drawing::Point(60, 24);
			this->oscoIcon->Name = S"oscoIcon";
			this->oscoIcon->Size = System::Drawing::Size(32, 32);
			this->oscoIcon->TabIndex = 1;
			this->oscoIcon->TabStop = false;
			this->oscoIcon->Click += new System::EventHandler(this, panel_Click);
			// 
			// channelIcon
			// 
			this->channelIcon->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->channelIcon->Cursor = System::Windows::Forms::Cursors::Hand;
			this->channelIcon->Image = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"channelIcon.Image")));
			this->channelIcon->Location = System::Drawing::Point(16, 24);
			this->channelIcon->Name = S"channelIcon";
			this->channelIcon->Size = System::Drawing::Size(32, 32);
			this->channelIcon->TabIndex = 0;
			this->channelIcon->TabStop = false;
			this->channelIcon->Click += new System::EventHandler(this, panel_Click);
			// 
			// label1
			// 
			this->label1->ForeColor = System::Drawing::SystemColors::ControlLight;
			this->label1->Location = System::Drawing::Point(8, 392);
			this->label1->Name = S"label1";
			this->label1->Size = System::Drawing::Size(776, 16);
			this->label1->TabIndex = 24;
			this->label1->Text = S"The use of the MT-32 faceplate in no way implies any endorsement, cooperation or " 
				S"sponsorship by Roland Corp.  Roland is a trademark of Roland Corp.";
			this->label1->TextAlign = System::Drawing::ContentAlignment::TopCenter;
			// 
			// partialTip
			// 
			this->partialTip->AutoPopDelay = 5000;
			this->partialTip->InitialDelay = 0;
			this->partialTip->ReshowDelay = 100;
			// 
			// facePlate
			// 
			this->facePlate->Location = System::Drawing::Point(0, 0);
			this->facePlate->Name = S"facePlate";
			this->facePlate->Size = System::Drawing::Size(800, 136);
			this->facePlate->TabIndex = 25;
			// 
			// channelStatus
			// 
			this->channelStatus->Location = System::Drawing::Point(168, 144);
			this->channelStatus->Name = S"channelStatus";
			this->channelStatus->Size = System::Drawing::Size(608, 240);
			this->channelStatus->TabIndex = 26;
			// 
			// Form1
			// 
			this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
			this->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->ClientSize = System::Drawing::Size(800, 414);
			this->Controls->Add(this->channelStatus);
			this->Controls->Add(this->facePlate);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->groupBox3);
			this->Controls->Add(this->groupBox1);
			this->Icon = (__try_cast<System::Drawing::Icon *  >(resources->GetObject(S"$this.Icon")));
			this->MaximizeBox = false;
			this->MaximumSize = System::Drawing::Size(808, 448);
			this->MinimumSize = System::Drawing::Size(808, 448);
			this->Name = S"Form1";
			this->Text = S"MUNT Control Panel";
			this->Load += new System::EventHandler(this, Form1_Load_1);
			this->groupBox3->ResumeLayout(false);
			this->ResumeLayout(false);

		}	

	private: System::Void panel_Click(System::Object * sender, System::EventArgs * e) {
				System::Windows::Forms::PictureBox *pb = dynamic_cast<System::Windows::Forms::PictureBox *>(sender);
				this->Form1_UpdateTab(pb->TabIndex);
			 }

	private: System::Void Form1_UpdateTab(System::Int32 newTab) {
				 switch(this->currentTab) {
					 case 0:
						 channelIcon->set_BackColor(System::Drawing::Color::FromArgb(56,56,60));
						 channelStatus->Hide();
						 break;
					 case 1:
						 oscoIcon->set_BackColor(System::Drawing::Color::FromArgb(56,56,60));
						 break;
					 case 2:
						 settingsIcon->set_BackColor(System::Drawing::Color::FromArgb(56,56,60));
						 break;
				 }

				 this->currentTab = newTab;

				 switch(this->currentTab) {
					 case 0:
						 channelIcon->set_BackColor(System::Drawing::Color::FromArgb(96,96,100));
						 channelStatus->Show();
						 break;
					 case 1:
						 oscoIcon->set_BackColor(System::Drawing::Color::FromArgb(96,96,100));
						 break;
					 case 2:
						 settingsIcon->set_BackColor(System::Drawing::Color::FromArgb(96,96,100));
						 break;
				 }


			 }

	private: System::Void Form1_Load_1(System::Object *  sender, System::EventArgs *  e)
			 {

				 this->currentTab = 0;
				 this->Form1_UpdateTab(this->currentTab);
				
				 this->settingsTip->SetToolTip(this->settingsIcon, S"Driver Settings");
				 this->oscoTip->SetToolTip(this->oscoIcon, S"Partial Status");
				 this->channelTip->SetToolTip(this->channelIcon, S"Channel Status");

				 

				 memset(chanList,0,sizeof(chanInfo));

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
					this->pictureBox[i]->TabIndex = i + 1;
					this->pictureBox[i]->TabStop = false;
					this->pictureBox[i]->Cursor = System::Windows::Forms::Cursors::Hand;
					this->pictureBox[i]->MouseEnter += new System::EventHandler(this, &Form1::partial_Enter);
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
								inPacket = SDLNet_AllocPacket(4096);
								if(inPacket != NULL) {
									this->timer1->set_Enabled(true);		
								}
							}
						}
						
						
					//}
					
					
					
				}


				
			 }

	private: System::Void partial_Enter(System::Object * sender, System::EventArgs * e) {
				char strbuf[4096];
				System::Windows::Forms::PictureBox *pb = dynamic_cast<System::Windows::Forms::PictureBox *>(sender);

				sprintf(strbuf, "Partial %d\nClick for information.", pb->TabIndex);


				this->partialTip->SetToolTip(pb, new System::String(strbuf));
				
				
			 }

	private: System::Void timer1_Tick(System::Object *  sender, System::EventArgs *  e)
			 {
				Uint16 buffer[2048];
				UDPpacket regPacket;
				int numrecv = 0;
				
				

				// Send heartbeat
				buffer[0] = 1;

				regPacket.data = (Uint8 *)&buffer[0];
				regPacket.len = 2;
				regPacket.maxlen = 2;
				regPacket.channel = clientChannel;
				SDLNet_UDP_Send(clientSock, regPacket.channel, &regPacket);


				inPacket->channel = clientChannel;

				numrecv = SDLNet_UDP_Recv(clientSock, inPacket);
				if(numrecv >0) {
					int i;
					bool found = false;
					bool anyActive = false;
					memcpy(buffer, inPacket->data, inPacket->len);
					// Heartbeat response
					if((buffer[0] == 1) && (inPacket->len == 296)) {
						memcpy(buffer, inPacket->data, 296);
						found = true;
						int count = (int)buffer[1];
						for(i=0;i<count;i++) {
							int t;
							t = buffer[2 + (i * 3)];
							chanList[i].isPlaying = true;
							switch(t) {
								case 0:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(0,0,0));
									chanList[i].isPlaying = false;
									break;
								case 1:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(255,0,0));
									anyActive = true;
									break;
								case 2:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(0,255,0));
									anyActive = true;
									break;
								case 3:
									this->pictureBox[i]->set_BackColor(System::Drawing::Color::FromArgb(255,255,0));
									anyActive = true;
									break;
							}
							chanList[i].assignedPart = buffer[2 + (i * 3) + 1];
							chanList[i].freq = buffer[2 + (i * 3) + 2];
						}

						channelStatus->sendUpdateData((char *)&buffer[0], count, chanList);

						if(anyActive) {
							this->facePlate->workMidiLight(System::Drawing::Color::Lime);
						} else {
							this->facePlate->workMidiLight(System::Drawing::Color::FromArgb(41,42,51));
						}



					}
					// LCD Text Display change
					if(buffer[0] == 2) {
						found = true;
						this->facePlate->setLCDText(new System::String((char *)&buffer[1]));
					}
					if(!found) {
						System::Windows::Forms::MessageBox::Show(new System::String((char *)&buffer[1]), new System::String((char *)&buffer[1]));
					}

				} else {
					this->facePlate->workMidiLight(System::Drawing::Color::FromArgb(41,42,51));
				}


			 }







};
}


