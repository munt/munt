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
	private: System::Windows::Forms::Label *  label9;
	private: System::Windows::Forms::PictureBox *  noteBox[];
	private: System::Windows::Forms::Label *  DisplayText;
	private: System::Windows::Forms::Label *  chan8;
	private: System::Windows::Forms::Label *  chan7;
	private: System::Windows::Forms::Label *  chan6;
	private: System::Windows::Forms::Label *  chan5;
	private: System::Windows::Forms::Label *  chan4;
	private: System::Windows::Forms::Label *  chan3;
	private: System::Windows::Forms::Label *  chan2;
	private: System::Windows::Forms::Label *  chan1;
	private: System::Windows::Forms::PictureBox *  midiLight;

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
			this->label9 = new System::Windows::Forms::Label();
			this->chan8 = new System::Windows::Forms::Label();
			this->chan7 = new System::Windows::Forms::Label();
			this->chan6 = new System::Windows::Forms::Label();
			this->chan5 = new System::Windows::Forms::Label();
			this->chan4 = new System::Windows::Forms::Label();
			this->chan3 = new System::Windows::Forms::Label();
			this->chan2 = new System::Windows::Forms::Label();
			this->chan1 = new System::Windows::Forms::Label();
			this->DisplayText = new System::Windows::Forms::Label();
			this->midiLight = new System::Windows::Forms::PictureBox();
			this->groupBox2->SuspendLayout();
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
			this->pictureBox1->Cursor = System::Windows::Forms::Cursors::Hand;
			this->pictureBox1->Location = System::Drawing::Point(0, 0);
			this->pictureBox1->Name = S"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(800, 134);
			this->pictureBox1->TabIndex = 10;
			this->pictureBox1->TabStop = false;
			// 
			// groupBox2
			// 
			this->groupBox2->Controls->Add(this->label9);
			this->groupBox2->Controls->Add(this->chan8);
			this->groupBox2->Controls->Add(this->chan7);
			this->groupBox2->Controls->Add(this->chan6);
			this->groupBox2->Controls->Add(this->chan5);
			this->groupBox2->Controls->Add(this->chan4);
			this->groupBox2->Controls->Add(this->chan3);
			this->groupBox2->Controls->Add(this->chan2);
			this->groupBox2->Controls->Add(this->chan1);
			this->groupBox2->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->groupBox2->Location = System::Drawing::Point(168, 144);
			this->groupBox2->Name = S"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(624, 240);
			this->groupBox2->TabIndex = 11;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = S"Channel Status";
			// 
			// label9
			// 
			this->label9->Location = System::Drawing::Point(8, 216);
			this->label9->Name = S"label9";
			this->label9->Size = System::Drawing::Size(104, 16);
			this->label9->TabIndex = 12;
			this->label9->Text = S"Rhythm Channel";
			this->label9->Click += new System::EventHandler(this, label9_Click);
			// 
			// chan8
			// 
			this->chan8->Location = System::Drawing::Point(8, 192);
			this->chan8->Name = S"chan8";
			this->chan8->Size = System::Drawing::Size(104, 16);
			this->chan8->TabIndex = 13;
			this->chan8->Text = S"Channel 8";
			// 
			// chan7
			// 
			this->chan7->Location = System::Drawing::Point(8, 168);
			this->chan7->Name = S"chan7";
			this->chan7->Size = System::Drawing::Size(104, 16);
			this->chan7->TabIndex = 14;
			this->chan7->Text = S"Channel 7";
			// 
			// chan6
			// 
			this->chan6->Location = System::Drawing::Point(8, 144);
			this->chan6->Name = S"chan6";
			this->chan6->Size = System::Drawing::Size(104, 16);
			this->chan6->TabIndex = 15;
			this->chan6->Text = S"Channel 6";
			// 
			// chan5
			// 
			this->chan5->Location = System::Drawing::Point(8, 120);
			this->chan5->Name = S"chan5";
			this->chan5->Size = System::Drawing::Size(104, 16);
			this->chan5->TabIndex = 16;
			this->chan5->Text = S"Channel 5";
			// 
			// chan4
			// 
			this->chan4->Location = System::Drawing::Point(8, 96);
			this->chan4->Name = S"chan4";
			this->chan4->Size = System::Drawing::Size(104, 16);
			this->chan4->TabIndex = 17;
			this->chan4->Text = S"Channel 4";
			// 
			// chan3
			// 
			this->chan3->Location = System::Drawing::Point(8, 72);
			this->chan3->Name = S"chan3";
			this->chan3->Size = System::Drawing::Size(104, 16);
			this->chan3->TabIndex = 18;
			this->chan3->Text = S"Channel 3";
			// 
			// chan2
			// 
			this->chan2->Location = System::Drawing::Point(8, 48);
			this->chan2->Name = S"chan2";
			this->chan2->Size = System::Drawing::Size(104, 16);
			this->chan2->TabIndex = 19;
			this->chan2->Text = S"Channel 2";
			// 
			// chan1
			// 
			this->chan1->Location = System::Drawing::Point(8, 24);
			this->chan1->Name = S"chan1";
			this->chan1->Size = System::Drawing::Size(104, 16);
			this->chan1->TabIndex = 20;
			this->chan1->Text = S"Channel 1";
			// 
			// DisplayText
			// 
			this->DisplayText->BackColor = System::Drawing::Color::Transparent;
			this->DisplayText->Cursor = System::Windows::Forms::Cursors::Hand;
			this->DisplayText->Font = new System::Drawing::Font(S"LcdD", 15.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, (System::Byte)0);
			this->DisplayText->ForeColor = System::Drawing::Color::Lime;
			this->DisplayText->ImageAlign = System::Drawing::ContentAlignment::TopLeft;
			this->DisplayText->Location = System::Drawing::Point(80, 64);
			this->DisplayText->Name = S"DisplayText";
			this->DisplayText->Size = System::Drawing::Size(184, 24);
			this->DisplayText->TabIndex = 21;
			this->DisplayText->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->DisplayText->Click += new System::EventHandler(this, label10_Click);
			// 
			// midiLight
			// 
			this->midiLight->BackColor = System::Drawing::Color::FromArgb((System::Byte)41, (System::Byte)42, (System::Byte)51);
			this->midiLight->Cursor = System::Windows::Forms::Cursors::Hand;
			this->midiLight->Location = System::Drawing::Point(295, 74);
			this->midiLight->Name = S"midiLight";
			this->midiLight->Size = System::Drawing::Size(10, 2);
			this->midiLight->TabIndex = 22;
			this->midiLight->TabStop = false;
			// 
			// Form1
			// 
			this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
			this->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->ClientSize = System::Drawing::Size(800, 398);
			this->Controls->Add(this->midiLight);
			this->Controls->Add(this->DisplayText);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->groupBox1);
			this->Icon = (__try_cast<System::Drawing::Icon *  >(resources->GetObject(S"$this.Icon")));
			this->MaximizeBox = false;
			this->Name = S"Form1";
			this->Text = S"MUNT Control Panel";
			this->Load += new System::EventHandler(this, Form1_Load_1);
			this->groupBox2->ResumeLayout(false);
			this->ResumeLayout(false);

		}	

	private: System::Void Form1_Load_1(System::Object *  sender, System::EventArgs *  e)
			 {
				

				 memset(chanList,0,sizeof(chanInfo));

				this->pictureBox = new System::Windows::Forms::PictureBox*[32];
				this->noteBox = new System::Windows::Forms::PictureBox*[9];
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

				for(i=0;i<9;i++) {
					this->noteBox[i] = new System::Windows::Forms::PictureBox();
					this->groupBox2->Controls->Add(this->noteBox[i]);
					this->noteBox[i]->Location = System::Drawing::Point(232, 24 + (i * 24));
					this->noteBox[i]->Name = S"pictureBox2";
					this->noteBox[i]->Size = System::Drawing::Size(384, 16);
					//this->noteBox[i]->BackColor = System::Drawing::Color::Wheat;
					this->noteBox[i]->TabIndex = i+1;
					this->noteBox[i]->TabStop = false;
					this->noteBox[i]->Paint += new System::Windows::Forms::PaintEventHandler(this, &Form1::noteBox_Paint);
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

	private: System::Void noteBox_Paint(System::Object * sender, System::Windows::Forms::PaintEventArgs* e) {
				int i;
				System::Windows::Forms::PictureBox *pb = dynamic_cast<System::Windows::Forms::PictureBox *>(sender);

				i = (pb->TabIndex - 1);
				int notesPlaying = 0;
				int noteList[32];
				int j, r;
				for(j=0;j<32;j++) {
					if(chanList[j].isPlaying) {
						if(chanList[j].assignedPart == i) {
							bool found = false;
							for(r=0;r<notesPlaying;r++) {
								if(noteList[r] == chanList[j].freq) {
									found = true;
									break;
								}
							}
							if(!found) {
								noteList[notesPlaying] = chanList[j].freq;
								notesPlaying++;
							}
						}
					}
				}
				Graphics * g = e->Graphics;

				g->Clear(System::Drawing::Color::FromArgb(69, 69, 73));
				for(j=0;j<notesPlaying;j++) {
					int xat;
					xat = (noteList[j]) * 3;
					g->FillRectangle(System::Drawing::Brushes::Lime, xat, 0, 3, 24);
				}

				

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
					if((buffer[0] == 1) && (inPacket->len == 278)) {
						memcpy(buffer, inPacket->data, 278);
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


						if(anyActive) {
							this->midiLight->set_BackColor(System::Drawing::Color::Lime);
						} else {
							this->midiLight->set_BackColor(System::Drawing::Color::FromArgb(41,42,51));
						}


						int instrlist = (int)buffer[2 + (count * 3)];
						// Assume 8 for now
						char tmpStr[11];

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 0)], 10);
						tmpStr[10] = 0;
						chan1->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 1)], 10);
						tmpStr[10] = 0;
						chan2->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 2)], 10);
						tmpStr[10] = 0;
						chan3->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 3)], 10);
						tmpStr[10] = 0;
						chan4->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 4)], 10);
						tmpStr[10] = 0;
						chan5->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 5)], 10);
						tmpStr[10] = 0;
						chan6->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 6)], 10);
						tmpStr[10] = 0;
						chan7->set_Text(new System::String(tmpStr));

						memcpy(tmpStr, &buffer[(count * 3) + 3 + (5 * 7)], 10);
						tmpStr[10] = 0;
						chan8->set_Text(new System::String(tmpStr));

						for(i=0;i<9;i++) {
							this->noteBox[i]->Refresh();
						}

					}
					// LCD Text Display change
					if(buffer[0] == 2) {
						found = true;
						DisplayText->set_Text(new System::String((char *)&buffer[1]));
					}
					if(!found) {
						System::Windows::Forms::MessageBox::Show(new System::String((char *)&buffer[1]), new System::String((char *)&buffer[1]));
					}

				} else {
					this->midiLight->set_BackColor(System::Drawing::Color::FromArgb(41,42,51));
				}


			 }


private: System::Void label10_Click(System::Object *  sender, System::EventArgs *  e)
		 {
		 }

private: System::Void label9_Click(System::Object *  sender, System::EventArgs *  e)
		 {
		 }

};
}


