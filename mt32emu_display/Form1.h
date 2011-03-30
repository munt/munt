#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#using "mt32emu_display_controls.dll"

#include "..\mt32emu_display_controls\DriverCommClass.h"

chanInfo chanList[32];

namespace mt32emu_display
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;

	/// <summary> 
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the 
	///          'Resource File Name' property for the managed resource compiler tool 
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public __gc class Form1 : public System::Windows::Forms::Form, public mt32emu_display_controls::KnobInterface
	{	
	public:
		Form1(void)
		{
			
			InitializeComponent();
			partActive = new System::Boolean[7];
			channelStatus->Tag = ci;
			this->facePlate->Tag = ci;
			this->partialStatus->Tag = ci;
			this->settingsStatus->Tag = ci;
			this->sysexImporter1->Tag = ci;
			this->ticksSinceContact = 5000 / this->timer1->get_Interval();
			this->settingsStatus->disableActiveSettings();
			this->balloonTip1 = new mt32emu_display_controls::BalloonTip();

			// Build context menu
			this->exitMenuItem = new MenuItem();
			this->hbar = new MenuItem();
			this->lcdMenuItem = new MenuItem();
			this->contextMenu1->MenuItems->Add(this->exitMenuItem);
			this->contextMenu1->MenuItems->Add(this->hbar);
			this->contextMenu1->MenuItems->Add(this->lcdMenuItem);

			this->exitMenuItem->Index = 0;
			this->exitMenuItem->Text = S"E&xit";
			this->exitMenuItem->Click += new System::EventHandler(this, &Form1::exit_Click);

			this->hbar->Index = 0;
			this->hbar->Text = S"-";

			this->lcdMenuItem->Index = 0;
			this->lcdMenuItem->Text = S"&Show LCD Text";
			this->lcdMenuItem->Checked = true;
			this->lcdMenuItem->Click += new System::EventHandler(this, &Form1::lcdMenu_Click);
			
			this->notifyIcon1->ContextMenu = this->contextMenu1;

			this->alwaysTopMenu->Click += new System::EventHandler(this, &Form1::onTop_Click);
			this->expModeMenu->Click += new System::EventHandler(this, &Form1::expandMenu_Click);


	        // Create an instance of HookProc.
			MouseHookProcedure = new HookProc(this, &Form1::MouseHookProc);

			hHook = SetWindowsHookEx(WH_MOUSE,
						MouseHookProcedure,
						(IntPtr)0,
						AppDomain::GetCurrentThreadId());
			if(hHook == 0 )
			{
				MessageBox::Show("SetWindowsHookEx Failed");
				return;
			}

			

		}
		System::Void knobUpdated(int newValue) {

		}

		System::Void requestInfo(int addr, int len) {
			this->ci->requestSynthMemory(addr, len);
		}
		System::Void sendInfo(int addr, int len, char * buf) {
		}

		System::Void DoEvents() {
		}

		static int MouseHookProc(int nCode, System::IntPtr wParam, System::IntPtr lParam)
		{
			//Marshall the data from callback.
			MouseHookStruct * MyMouseHookStruct = (MouseHookStruct* ) Marshal::PtrToStructure(lParam, __typeof(MouseHookStruct));

			if (nCode < 0)
			{
				return CallNextHookEx(hHook, nCode, wParam, lParam);
			}
			else
			{
				//Create a string variable with shows current mouse. coordinates
				
				//MyMouseHookStruct->dwExtraInfo
				Form1 * tempForm = (Form1 *)Form::ActiveForm;
				if(tempForm != NULL) {
					if(wParam.ToInt32() & 0x4) {
						tempForm->showContextMenu(MyMouseHookStruct->pt.x,MyMouseHookStruct->pt.y);
					}

				}

				return CallNextHookEx(hHook, nCode, wParam, lParam);
			}
		}

	public: System::Void showContextMenu(int x, int y) {
				this->contextMenu2->Show(this, Point(x - this->Location.X - 16, y - this->Location.Y - 32));
			}
		

	private: System::Windows::Forms::MenuItem *  expModeMenu;
	private: System::Windows::Forms::MenuItem *  alwaysTopMenu;
private: mt32emu_display_controls::SysexImporter *  sysexImporter1;
	public: static const int WH_MOUSE = 7;

	public: __delegate int HookProc(int nCode, System::IntPtr wParam, System::IntPtr lParam);

	[StructLayout(LayoutKind::Sequential)]
	__value struct POINT
	{
		public: int x;
		public: int y;
	};

	[StructLayout(LayoutKind::Sequential)]
	__value struct MouseHookStruct
	{
		public: POINT pt;
		public: int hwnd;
		public: int wHitTestCode;
		public: int dwExtraInfo;
		public: int mouseData;
	};

	[DllImport(S"user32.dll")]
	static int SetWindowsHookEx(int idHook, HookProc * lpfn, System::IntPtr hInstance, int threadId);

	[DllImport(S"user32.dll")]
	static bool UnhookWindowsHookEx(int idHook);

	[DllImport(S"user32.dll")]
	static int CallNextHookEx(int idHook, int nCode, System::IntPtr wParam, System::IntPtr lParam);

	protected:
		void Dispose(Boolean disposing)
		{

			ci->shutDown();

			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		}
	private: System::Windows::Forms::GroupBox *  groupBox1;
	private: System::Windows::Forms::PictureBox *  pictureBox[];
	private: System::Windows::Forms::Timer *  timer1;
	private: System::Int32 ticksSinceContact;
	private: static int hHook = 0;





	private: System::Windows::Forms::GroupBox *  groupBox3;
	private: System::Windows::Forms::PictureBox *  oscoIcon;
	private: System::Windows::Forms::PictureBox *  channelIcon;
	private: System::Windows::Forms::PictureBox *  settingsIcon;
	private: System::Windows::Forms::Label *  label1;
	private: System::Windows::Forms::ToolTip *  channelTip;
	private: System::Windows::Forms::ToolTip *  oscoTip;
	private: System::Windows::Forms::ToolTip *  settingsTip;
	private: System::Windows::Forms::MenuItem * exitMenuItem;
	private: System::Windows::Forms::MenuItem * lcdMenuItem;
	private: System::Windows::Forms::MenuItem * hbar;

	private: System::Boolean partActive[];

	private: System::Windows::Forms::ToolTip *  partialTip;
	private: System::Int32 currentTab;
	private: mt32emu_display_controls::FacePlate *  facePlate;
	private: mt32emu_display_controls::ChannelDisplay *  channelStatus;
	private: mt32emu_display_controls::ControlInterface * ci;
	private: mt32emu_display_controls::PartialDisplay *  partialStatus;
	private: mt32emu_display_controls::SettingsDisplay *  settingsStatus;
	private: mt32emu_display_controls::BalloonTip *  balloonTip1;
	private: System::Windows::Forms::NotifyIcon *  notifyIcon1;
	private: System::Windows::Forms::ContextMenu *  contextMenu1;
	private: System::Windows::Forms::ContextMenu *  contextMenu2;


	private: HookProc * MouseHookProcedure;

	
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
			this->ci = new mt32emu_display_controls::ControlInterface(this->components);
			this->partialStatus = new mt32emu_display_controls::PartialDisplay();
			this->settingsStatus = new mt32emu_display_controls::SettingsDisplay();
			this->notifyIcon1 = new System::Windows::Forms::NotifyIcon(this->components);
			this->contextMenu1 = new System::Windows::Forms::ContextMenu();
			this->contextMenu2 = new System::Windows::Forms::ContextMenu();
			this->expModeMenu = new System::Windows::Forms::MenuItem();
			this->alwaysTopMenu = new System::Windows::Forms::MenuItem();
			this->sysexImporter1 = new mt32emu_display_controls::SysexImporter();
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
			this->groupBox1->Text = S"Partial Info";
			// 
			// timer1
			// 
			this->timer1->Interval = 20;
			this->timer1->Tick += new System::EventHandler(this, &Form1::timer1_Tick);
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
			this->groupBox3->Size = System::Drawing::Size(152, 72);
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
			this->settingsIcon->Click += new System::EventHandler(this, &Form1::panel_Click);
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
			this->oscoIcon->Click += new System::EventHandler(this, &Form1::panel_Click);
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
			this->channelIcon->Click += new System::EventHandler(this, &Form1::panel_Click);
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
			this->facePlate->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"facePlate.BackgroundImage")));
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
			// partialStatus
			// 
			this->partialStatus->Location = System::Drawing::Point(168, 144);
			this->partialStatus->Name = S"partialStatus";
			this->partialStatus->Size = System::Drawing::Size(608, 240);
			this->partialStatus->TabIndex = 27;
			this->partialStatus->Visible = false;
			// 
			// settingsStatus
			// 
			this->settingsStatus->Location = System::Drawing::Point(168, 144);
			this->settingsStatus->Name = S"settingsStatus";
			this->settingsStatus->Size = System::Drawing::Size(608, 240);
			this->settingsStatus->TabIndex = 28;
			this->settingsStatus->Visible = false;
			// 
			// notifyIcon1
			// 
			this->notifyIcon1->Icon = (__try_cast<System::Drawing::Icon *  >(resources->GetObject(S"notifyIcon1.Icon")));
			this->notifyIcon1->Text = S"Munt Control Panel";
			this->notifyIcon1->DoubleClick += new System::EventHandler(this, &Form1::notifyIcon1_DoubleClick);
			// 
			// contextMenu2
			// 
			System::Windows::Forms::MenuItem* __mcTemp__1[] = new System::Windows::Forms::MenuItem*[2];
			__mcTemp__1[0] = this->expModeMenu;
			__mcTemp__1[1] = this->alwaysTopMenu;
			this->contextMenu2->MenuItems->AddRange(__mcTemp__1);
			// 
			// expModeMenu
			// 
			this->expModeMenu->Checked = true;
			this->expModeMenu->Index = 0;
			this->expModeMenu->Text = S"Expanded Mode";
			// 
			// alwaysTopMenu
			// 
			this->alwaysTopMenu->Index = 1;
			this->alwaysTopMenu->Text = S"Always On Top";
			// 
			// sysexImporter1
			// 
			this->sysexImporter1->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->sysexImporter1->Location = System::Drawing::Point(8, 224);
			this->sysexImporter1->Name = S"sysexImporter1";
			this->sysexImporter1->Size = System::Drawing::Size(152, 56);
			this->sysexImporter1->TabIndex = 29;
			this->sysexImporter1->Visible = false;
			// 
			// Form1
			// 
			this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
			this->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->ClientSize = System::Drawing::Size(800, 414);
			this->Controls->Add(this->sysexImporter1);
			this->Controls->Add(this->channelStatus);
			this->Controls->Add(this->facePlate);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->groupBox3);
			this->Controls->Add(this->groupBox1);
			this->Controls->Add(this->partialStatus);
			this->Controls->Add(this->settingsStatus);
			this->Icon = (__try_cast<System::Drawing::Icon *  >(resources->GetObject(S"$this.Icon")));
			this->MaximizeBox = false;
			this->MaximumSize = System::Drawing::Size(808, 448);
			this->MinimumSize = System::Drawing::Size(808, 448);
			this->Name = S"Form1";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = S"Munt Control Panel";
			this->Load += new System::EventHandler(this, &Form1::Form1_Load_1);
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
						 partialStatus->Hide();
						 break;
					 case 2:
						 settingsIcon->set_BackColor(System::Drawing::Color::FromArgb(56,56,60));
						 settingsStatus->Hide();
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
						 partialStatus->Show();
						 break;
					 case 2:
						 settingsIcon->set_BackColor(System::Drawing::Color::FromArgb(96,96,100));
						 settingsStatus->Show();
						 break;
				 }


			 }

	public: System::Void OnResize(EventArgs * e) {
				 if(this->WindowState == FormWindowState::Minimized) {
					 this->Hide();
					 this->notifyIcon1->Visible = true;
				 }
			 }

	public: System::Void form_MouseDown(Object * sender, MouseEventArgs * e) {
				//this->contextMenu2->Show(this, Point(e->X, e->Y));
				
			}

	public: System::Void OnMouseDown(MouseEventArgs * e) {
				//this->form_MouseDown(this, e);
				

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

				if(this->ci->InitConnection()) {
					this->timer1->set_Enabled(true);		
				}
				
			 }

	private: System::Void exit_Click(Object * sender, System::EventArgs * e) {
				 this->Close();
			 }

	private: System::Void lcdMenu_Click(Object * sender, System::EventArgs * e) {
				 this->lcdMenuItem->Checked = !this->lcdMenuItem->Checked;
			 }

	private: System::Void expandMenu_Click(Object * sender, System::EventArgs * e) {
				 this->expModeMenu->Checked = !this->expModeMenu->Checked;
				 if(this->expModeMenu->Checked) {
					this->ClientSize = System::Drawing::Size(800, 414);
					this->MaximumSize = System::Drawing::Size(808, 448);
					this->MinimumSize = System::Drawing::Size(808, 448);
				 } else {

					this->ClientSize = System::Drawing::Size(800, 140);
					this->MaximumSize = System::Drawing::Size(808, 170);
					this->MinimumSize = System::Drawing::Size(808, 170);
 				 }
			 }

	private: System::Void onTop_Click(Object * sender, System::EventArgs * e) {
				 this->alwaysTopMenu->Checked = !this->alwaysTopMenu->Checked;
				 if(this->alwaysTopMenu->Checked) {
					 this->TopMost = true;
				 } else {
					 this->TopMost = false;
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

				unsigned short buffer[2048];

				this->ticksSinceContact++;

 			    this->ci->sendHeartBeat();

				for (;;) {
					int numrecv = this->ci->checkForData((char *)buffer);

					if(numrecv >0) {
						int i;
						bool found = false;
						bool anyActive = false;
						int wPC = 6;
						// Heartbeat response
						if((buffer[0] == 1) && (numrecv == 492)) {
						//if((buffer[0] == 1) && (numrecv == 300)) {
							found = true;
							this->ticksSinceContact = 0;
							this->facePlate->turnOnModule();
							this->sysexImporter1->Visible = true;

							int count = (int)buffer[1];
							for(i=0;i<6;i++) {
								this->partActive[i] = false;
							}
							for(i=0;i<count;i++) {
								int t;
								t = buffer[2 + (i * wPC)];
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
								chanList[i].assignedPart = buffer[2 + (i * wPC) + 1];

								if((chanList[i].assignedPart < 5) || (chanList[i].assignedPart == 8)) {
									int pUse = chanList[i].assignedPart;
									if (pUse == 8) pUse = 5;
									if((t == 1) || (t == 2)) {
									//if(t == 1) {
										this->partActive[pUse] = true;
									}
								}
								chanList[i].freq = buffer[2 + (i * wPC) + 2];
								chanList[i].age = *(unsigned int *)&buffer[2 + (i * wPC) + 3];
								chanList[i].vel = buffer[2 + (i * wPC) + 5];
							}

							channelStatus->sendUpdateData((char *)buffer, count, chanList, wPC);

							if(anyActive) {
								this->facePlate->turnOnMidiLight();
							} else {
								this->facePlate->turnOffMidiLight();
							}

							for(i=0;i<6;i++) {
								this->facePlate->setMaskedChar(i * 2, this->partActive[i]);
							}

							// TODO: Doesn't seem to work.  Wanted to get the buffer size so I could change
							// the timer to tick in sync with it.
							int sndBufSize = *(int *)&buffer[488];


						}
						// LCD Text Display change
						if(buffer[0] == 2) {
							found = true;
							this->facePlate->setLCDText(new System::String((char *)&buffer[1]));

							if((this->notifyIcon1->Visible) && (this->lcdMenuItem->Checked)) {
								char tmpBuf[256];
								sprintf(tmpBuf, "Munt Control Panel");
								this->balloonTip1->ShowBallon(1, tmpBuf, (char *)&buffer[1], 250);
							}
						}

						if(buffer[0] == 5) {
							found = true;
							int addr = *(int* )&buffer[1];
							int len = buffer[3]; 

							if(addr == 0x100000) {
								this->settingsStatus->UpdateActiveSettings((char *)&buffer[4], len);

								mt32emu_display_controls::KnobInterface *ki;
								ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->facePlate);
								ki->sendInfo(addr, len, (char *)&buffer[4]);

							}



						}


						if(!found) {
							char buf[512];
							sprintf(buf, "Unk. Packet: %d", buffer[0]);
							this->facePlate->setLCDText(new System::String(buf));
						}

					} else {
						break;
					}
				}

				if(this->ticksSinceContact > (1000 / this->timer1->get_Interval())) {
					this->facePlate->turnOffModule();
					this->sysexImporter1->Visible = false;
					this->settingsStatus->disableActiveSettings();
					//Prevent overflow
					--this->ticksSinceContact;

				} else {
					
					mt32emu_display_controls::KnobInterface *ki;
					ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->facePlate);
					ki->DoEvents();
					
					ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->settingsStatus);
					ki->DoEvents();
				}



			 }







		private: System::Void notifyIcon1_DoubleClick(System::Object *  sender, System::EventArgs *  e)
				{
					this->Show();
					this->set_WindowState(FormWindowState::Normal);
					notifyIcon1->Visible = false;
				}

		};
}


