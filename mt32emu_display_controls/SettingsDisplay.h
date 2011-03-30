#pragma once

#include "ControlInterface.h"
#include "KnobInterface.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for SettingsDisplay
	/// </summary>
	public __gc class SettingsDisplay : public System::Windows::Forms::UserControl, public mt32emu_display_controls::KnobInterface
	{
	public: 
		SettingsDisplay(void)
		{
			InitializeComponent();
			systemParam = new MT32Emu::MemParams::System();
			freezeComboBox = false;
			this->reverbMode->GotFocus += new System::EventHandler(this, &SettingsDisplay::reverbMode_GotFocus);
			this->reverbMode->LostFocus += new System::EventHandler(this, &SettingsDisplay::reverbMode_LostFocus);
		}

		System::Boolean needToUpdateActiveSettings() {
			if(!this->Visible) return false;
			if(!tabControl1->SelectedTab->Equals(this->ActivePage)) {
				return false;
			}
			return true;
		}

		System::Void knobUpdated(int newValue) {

		}

		System::Void requestInfo(int addr, int len) {
		}
		System::Void sendInfo(int addr, int len, char * buf) {

		}

		System::Void DoEvents() {
			enableActiveSettings();

			if(needToUpdateActiveSettings()) {
					mt32emu_display_controls::KnobInterface *ki;
					ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->Parent);
					ki->requestInfo(0x100000, 0x17);
			}
		}

		System::Void UpdateActiveSettings(char *memBuf, int len) {
			memcpy(systemParam, memBuf, len);
			if(systemParam->masterVol > 100) systemParam->masterVol = 100;
			this->volumeBar->Value = systemParam->masterVol;
			this->tuningBar->Value = systemParam->masterTune;
			
			if(!freezeComboBox) {
				switch(systemParam->reverbMode) {
					case 0:
						this->reverbMode->Text = S"Room";
						break;
					case 1:
						this->reverbMode->Text = S"Hall";
						break;
					case 2:
						this->reverbMode->Text = S"Plate";
						break;
					case 3:
						this->reverbMode->Text = S"Tap-delay";
						break;
				}
			}
			this->reverbLevel->Value = systemParam->reverbLevel;
			this->reverbTime->Value = systemParam->reverbTime;

		}

		System::Void enableActiveSettings() {
			if(!this->ActivePage->Enabled) {
				this->ActivePage->Enabled = true;
			}
		}

		System::Void disableActiveSettings() {
			if(this->ActivePage->Enabled) {
				this->ActivePage->Enabled = false;
			}
		}


	protected: 
		void Dispose(Boolean disposing)
		{
			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		} 
	private: System::Windows::Forms::GroupBox *  settingsStatus;
	private: System::Windows::Forms::TabControl *  tabControl1;
	private: System::Windows::Forms::TabPage *  ActivePage;
	private: System::Windows::Forms::TabPage *  OutputSettings;

	private: System::Windows::Forms::Label *  label1;

	private: System::Windows::Forms::Label *  label2;

	private: System::Windows::Forms::Label *  label3;

	private: System::Windows::Forms::Label *  label4;

	private: System::Windows::Forms::Label *  label5;
	private: System::Windows::Forms::TrackBar *  tuningBar;
	private: System::Windows::Forms::TrackBar *  volumeBar;
	private: System::Windows::Forms::TrackBar *  reverbLevel;
	private: System::Windows::Forms::TrackBar *  reverbTime;
	private: System::Windows::Forms::ComboBox *  reverbMode;
	private: System::Windows::Forms::GroupBox *  reverbBox;
	private: MT32Emu::MemParams::System * systemParam;
	private: System::Boolean freezeComboBox;






	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container* components;

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>		
		void InitializeComponent(void)
		{
			this->settingsStatus = new System::Windows::Forms::GroupBox();
			this->tabControl1 = new System::Windows::Forms::TabControl();
			this->ActivePage = new System::Windows::Forms::TabPage();
			this->tuningBar = new System::Windows::Forms::TrackBar();
			this->label5 = new System::Windows::Forms::Label();
			this->volumeBar = new System::Windows::Forms::TrackBar();
			this->label4 = new System::Windows::Forms::Label();
			this->reverbBox = new System::Windows::Forms::GroupBox();
			this->reverbLevel = new System::Windows::Forms::TrackBar();
			this->label3 = new System::Windows::Forms::Label();
			this->reverbTime = new System::Windows::Forms::TrackBar();
			this->label2 = new System::Windows::Forms::Label();
			this->reverbMode = new System::Windows::Forms::ComboBox();
			this->label1 = new System::Windows::Forms::Label();
			this->OutputSettings = new System::Windows::Forms::TabPage();
			this->settingsStatus->SuspendLayout();
			this->tabControl1->SuspendLayout();
			this->ActivePage->SuspendLayout();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->tuningBar))->BeginInit();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->volumeBar))->BeginInit();
			this->reverbBox->SuspendLayout();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->reverbLevel))->BeginInit();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->reverbTime))->BeginInit();
			this->SuspendLayout();
			// 
			// settingsStatus
			// 
			this->settingsStatus->Anchor = (System::Windows::Forms::AnchorStyles)(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right);
			this->settingsStatus->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->settingsStatus->Controls->Add(this->tabControl1);
			this->settingsStatus->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->settingsStatus->Location = System::Drawing::Point(0, 0);
			this->settingsStatus->Name = S"settingsStatus";
			this->settingsStatus->Size = System::Drawing::Size(608, 240);
			this->settingsStatus->TabIndex = 14;
			this->settingsStatus->TabStop = false;
			this->settingsStatus->Text = S"Driver Settings";
			// 
			// tabControl1
			// 
			this->tabControl1->Controls->Add(this->ActivePage);
			this->tabControl1->Controls->Add(this->OutputSettings);
			this->tabControl1->HotTrack = true;
			this->tabControl1->Location = System::Drawing::Point(8, 16);
			this->tabControl1->Name = S"tabControl1";
			this->tabControl1->SelectedIndex = 0;
			this->tabControl1->Size = System::Drawing::Size(592, 216);
			this->tabControl1->TabIndex = 0;
			// 
			// ActivePage
			// 
			this->ActivePage->BackColor = System::Drawing::SystemColors::Control;
			this->ActivePage->Controls->Add(this->tuningBar);
			this->ActivePage->Controls->Add(this->label5);
			this->ActivePage->Controls->Add(this->volumeBar);
			this->ActivePage->Controls->Add(this->label4);
			this->ActivePage->Controls->Add(this->reverbBox);
			this->ActivePage->Location = System::Drawing::Point(4, 22);
			this->ActivePage->Name = S"ActivePage";
			this->ActivePage->Size = System::Drawing::Size(584, 190);
			this->ActivePage->TabIndex = 0;
			this->ActivePage->Text = S"Active Settings";
			// 
			// tuningBar
			// 
			this->tuningBar->AutoSize = false;
			this->tuningBar->Enabled = false;
			this->tuningBar->Location = System::Drawing::Point(304, 40);
			this->tuningBar->Maximum = 127;
			this->tuningBar->Name = S"tuningBar";
			this->tuningBar->Size = System::Drawing::Size(272, 16);
			this->tuningBar->TabIndex = 4;
			this->tuningBar->TickFrequency = 16;
			this->tuningBar->Value = 63;
			// 
			// label5
			// 
			this->label5->Enabled = false;
			this->label5->ForeColor = System::Drawing::SystemColors::ControlText;
			this->label5->Location = System::Drawing::Point(216, 40);
			this->label5->Name = S"label5";
			this->label5->Size = System::Drawing::Size(88, 16);
			this->label5->TabIndex = 3;
			this->label5->Text = S"Master Tuning";
			this->label5->TextAlign = System::Drawing::ContentAlignment::BottomLeft;
			// 
			// volumeBar
			// 
			this->volumeBar->AutoSize = false;
			this->volumeBar->Location = System::Drawing::Point(304, 16);
			this->volumeBar->Maximum = 100;
			this->volumeBar->Name = S"volumeBar";
			this->volumeBar->Size = System::Drawing::Size(272, 16);
			this->volumeBar->TabIndex = 2;
			this->volumeBar->TickFrequency = 10;
			this->volumeBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->volumeBar->Scroll += new System::EventHandler(this, &SettingsDisplay::volumeBar_Scroll);
			// 
			// label4
			// 
			this->label4->ForeColor = System::Drawing::SystemColors::ControlText;
			this->label4->Location = System::Drawing::Point(216, 16);
			this->label4->Name = S"label4";
			this->label4->Size = System::Drawing::Size(88, 16);
			this->label4->TabIndex = 1;
			this->label4->Text = S"Master Volume";
			this->label4->TextAlign = System::Drawing::ContentAlignment::BottomLeft;
			// 
			// reverbBox
			// 
			this->reverbBox->Controls->Add(this->reverbLevel);
			this->reverbBox->Controls->Add(this->label3);
			this->reverbBox->Controls->Add(this->reverbTime);
			this->reverbBox->Controls->Add(this->label2);
			this->reverbBox->Controls->Add(this->reverbMode);
			this->reverbBox->Controls->Add(this->label1);
			this->reverbBox->ForeColor = System::Drawing::SystemColors::ControlText;
			this->reverbBox->Location = System::Drawing::Point(8, 8);
			this->reverbBox->Name = S"reverbBox";
			this->reverbBox->Size = System::Drawing::Size(200, 112);
			this->reverbBox->TabIndex = 0;
			this->reverbBox->TabStop = false;
			this->reverbBox->Text = S"Reverb Settings";
			// 
			// reverbLevel
			// 
			this->reverbLevel->AutoSize = false;
			this->reverbLevel->Location = System::Drawing::Point(64, 80);
			this->reverbLevel->Maximum = 7;
			this->reverbLevel->Name = S"reverbLevel";
			this->reverbLevel->Size = System::Drawing::Size(120, 16);
			this->reverbLevel->TabIndex = 6;
			this->reverbLevel->Scroll += new System::EventHandler(this, &SettingsDisplay::reverbLevel_Scroll);
			// 
			// label3
			// 
			this->label3->Location = System::Drawing::Point(8, 80);
			this->label3->Name = S"label3";
			this->label3->Size = System::Drawing::Size(56, 16);
			this->label3->TabIndex = 5;
			this->label3->Text = S"Level";
			// 
			// reverbTime
			// 
			this->reverbTime->AutoSize = false;
			this->reverbTime->Location = System::Drawing::Point(64, 48);
			this->reverbTime->Maximum = 8;
			this->reverbTime->Name = S"reverbTime";
			this->reverbTime->Size = System::Drawing::Size(120, 16);
			this->reverbTime->TabIndex = 4;
			this->reverbTime->Scroll += new System::EventHandler(this, &SettingsDisplay::reverbTime_Scroll);
			// 
			// label2
			// 
			this->label2->Location = System::Drawing::Point(8, 48);
			this->label2->Name = S"label2";
			this->label2->Size = System::Drawing::Size(56, 16);
			this->label2->TabIndex = 3;
			this->label2->Text = S"Time";
			// 
			// reverbMode
			// 
			this->reverbMode->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			System::Object* __mcTemp__1[] = new System::Object*[4];
			__mcTemp__1[0] = S"Room";
			__mcTemp__1[1] = S"Hall";
			__mcTemp__1[2] = S"Plate";
			__mcTemp__1[3] = S"Tap-delay";
			this->reverbMode->Items->AddRange(__mcTemp__1);
			this->reverbMode->Location = System::Drawing::Point(64, 16);
			this->reverbMode->Name = S"reverbMode";
			this->reverbMode->Size = System::Drawing::Size(121, 21);
			this->reverbMode->TabIndex = 2;
			this->reverbMode->SelectedIndexChanged += new System::EventHandler(this, &SettingsDisplay::reverbMode_SelectedIndexChanged);
			// 
			// label1
			// 
			this->label1->Location = System::Drawing::Point(8, 24);
			this->label1->Name = S"label1";
			this->label1->Size = System::Drawing::Size(56, 16);
			this->label1->TabIndex = 1;
			this->label1->Text = S"Mode";
			// 
			// OutputSettings
			// 
			this->OutputSettings->Location = System::Drawing::Point(4, 22);
			this->OutputSettings->Name = S"OutputSettings";
			this->OutputSettings->Size = System::Drawing::Size(584, 190);
			this->OutputSettings->TabIndex = 1;
			this->OutputSettings->Text = S"Output";
			// 
			// SettingsDisplay
			// 
			this->Controls->Add(this->settingsStatus);
			this->Name = S"SettingsDisplay";
			this->Size = System::Drawing::Size(608, 240);
			this->settingsStatus->ResumeLayout(false);
			this->tabControl1->ResumeLayout(false);
			this->ActivePage->ResumeLayout(false);
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->tuningBar))->EndInit();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->volumeBar))->EndInit();
			this->reverbBox->ResumeLayout(false);
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->reverbLevel))->EndInit();
			(__try_cast<System::ComponentModel::ISupportInitialize *  >(this->reverbTime))->EndInit();
			this->ResumeLayout(false);

		}



private: System::Void reverbMode_GotFocus(System::Object *  sender, System::EventArgs *  e) {
			 this->freezeComboBox = true;
		 }
private: System::Void reverbMode_LostFocus(System::Object *  sender, System::EventArgs *  e) {
			 this->freezeComboBox = false;
		 }

private: System::Void volumeBar_Scroll(System::Object *  sender, System::EventArgs *  e)
		 {
			if(this->Tag != NULL) {
				ControlInterface * parent;
				parent = dynamic_cast<ControlInterface*>(this->Tag);
				systemParam->masterVol = this->volumeBar->Value;

				parent->writeSynthMemory(0x100000, 0x17, (char *)systemParam);
			}

		 }

private: System::Void reverbTime_Scroll(System::Object *  sender, System::EventArgs *  e)
		 {
			if(this->Tag != NULL) {
				ControlInterface * parent;
				parent = dynamic_cast<ControlInterface*>(this->Tag);
				systemParam->reverbTime = this->reverbTime->Value;

				parent->writeSynthMemory(0x100000, 0x17, (char *)systemParam);
			}
		 }

private: System::Void reverbLevel_Scroll(System::Object *  sender, System::EventArgs *  e)
		 {
			if(this->Tag != NULL) {
				ControlInterface * parent;
				parent = dynamic_cast<ControlInterface*>(this->Tag);
				systemParam->reverbLevel = this->reverbLevel->Value;

				parent->writeSynthMemory(0x100000, 0x17, (char *)systemParam);
			}
		 }



private: System::Void reverbMode_SelectedIndexChanged(System::Object *  sender, System::EventArgs *  e)
		 {
			 if(this->freezeComboBox) {
				if(this->Tag != NULL) {
					ControlInterface * parent;
					parent = dynamic_cast<ControlInterface*>(this->Tag);
					
					if(reverbMode->Text->Equals(S"Room")) {
						systemParam->reverbMode = 0;
					}

					if(reverbMode->Text->Equals(S"Hall")) {
						systemParam->reverbMode = 1;
					}

					if(reverbMode->Text->Equals(S"Plate")) {
						systemParam->reverbMode = 2;
					}

					if(reverbMode->Text->Equals(S"Tap-delay")) {
						systemParam->reverbMode = 3;
					}

					parent->writeSynthMemory(0x100000, 0x17, (char *)systemParam);
					this->freezeComboBox = false;
				}
				this->reverbBox->Focus();
			 }
		 }

};
}