#pragma once

#include "ControlInterface.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::IO;


namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for SysexImporter
	/// </summary>
	public __gc class SysexImporter : public System::Windows::Forms::UserControl
	{
	public: 
		SysexImporter(void)
		{
			InitializeComponent();
			this->sysexBox->AllowDrop = true;
			this->sysexBox->DragOver += new System::Windows::Forms::DragEventHandler(this, &SysexImporter::sysexBox_DragOver);
			this->sysexBox->DragDrop += new System::Windows::Forms::DragEventHandler(this, &SysexImporter::sysexBox_DragDrop);
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
	private: System::Windows::Forms::GroupBox *  sysexBox;

	private: System::Windows::Forms::ProgressBar *  importStatus;
	private: System::Windows::Forms::Label *  labelStatus;





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
			this->sysexBox = new System::Windows::Forms::GroupBox();
			this->importStatus = new System::Windows::Forms::ProgressBar();
			this->labelStatus = new System::Windows::Forms::Label();
			this->sysexBox->SuspendLayout();
			this->SuspendLayout();
			// 
			// sysexBox
			// 
			this->sysexBox->Anchor = (System::Windows::Forms::AnchorStyles)(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right);
			this->sysexBox->BackColor = System::Drawing::Color::Transparent;
			this->sysexBox->Controls->Add(this->importStatus);
			this->sysexBox->Controls->Add(this->labelStatus);
			this->sysexBox->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->sysexBox->Location = System::Drawing::Point(0, 0);
			this->sysexBox->Name = S"sysexBox";
			this->sysexBox->Size = System::Drawing::Size(184, 88);
			this->sysexBox->TabIndex = 0;
			this->sysexBox->TabStop = false;
			this->sysexBox->Text = S"Drag Sysex files here";
			// 
			// importStatus
			// 
			this->importStatus->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->importStatus->Location = System::Drawing::Point(3, 77);
			this->importStatus->Name = S"importStatus";
			this->importStatus->Size = System::Drawing::Size(178, 8);
			this->importStatus->TabIndex = 1;
			this->importStatus->Visible = false;
			// 
			// labelStatus
			// 
			this->labelStatus->Anchor = (System::Windows::Forms::AnchorStyles)(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right);
			this->labelStatus->Location = System::Drawing::Point(3, 16);
			this->labelStatus->Name = S"labelStatus";
			this->labelStatus->Size = System::Drawing::Size(178, 56);
			this->labelStatus->TabIndex = 0;
			this->labelStatus->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// SysexImporter
			// 
			this->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->Controls->Add(this->sysexBox);
			this->Name = S"SysexImporter";
			this->Size = System::Drawing::Size(184, 88);
			this->sysexBox->ResumeLayout(false);
			this->ResumeLayout(false);

		}

	private:
		System::Void sysexBox_DragOver(Object * sender, System::Windows::Forms::DragEventArgs * e) {

			if(!e->Data->GetDataPresent(DataFormats::FileDrop)) {
				e->Effect = DragDropEffects::None;
				return;
			}
			e->Effect = DragDropEffects::Copy;

		}
	private:
		System::Void sysexBox_DragDrop(Object * sender, System::Windows::Forms::DragEventArgs * e) {
			if(!e->Data->GetDataPresent(DataFormats::FileDrop)) return;

			unsigned char sysexBuf[1024];
			int sysexLen = 0;
			bool inSysex = false;
			bool foundSysex = false;

			this->labelStatus->Text = S"";
			this->importStatus->Visible = false;
		

			String * fileNames[] = (String*[])e->Data->GetData(DataFormats::FileDrop);
			for(int i=0;i<fileNames->Length;i++) {
				this->labelStatus->Text = S"Importing Sysex";
				this->labelStatus->Refresh();
				this->importStatus->Minimum = 0;
				this->importStatus->Maximum = 99;
				this->importStatus->Value = 0;
				this->importStatus->Visible = true;

				FileStream * fs;
				try {
					fs = File::Open(fileNames[i], FileMode::Open, FileAccess::Read);
				} catch (IOException * ) {
					this->labelStatus->Text = S"Import Error";
					this->importStatus->Visible = false;
					this->labelStatus->Refresh();
					fs->Close();
					continue;
				}
				int j = 0;
				while(j<fs->Length) {
					unsigned char c;
					try {
						c = (unsigned char)fs->ReadByte();
					} catch (IOException * ) {
						this->labelStatus->Text = S"Import Error";
						this->importStatus->Visible = false;
						this->labelStatus->Refresh();
						fs->Close();
						break;
					}
					j++;
					if(inSysex) {
						if(c == 0xf7) {
							inSysex = false;
							foundSysex = true;
							
							ControlInterface * parent;
							parent = dynamic_cast<ControlInterface*>(this->Tag);
							parent->sendSemiRawSysex(sysexBuf, sysexLen);
							continue;
						}
						sysexBuf[sysexLen++] = c;
						if(sysexLen > 1024) {
							this->labelStatus->Text = S"Import Error";
							this->importStatus->Visible = false;
							this->labelStatus->Refresh();
							fs->Close();
							return;
						}
					}
					if((c == 0xf0) && (!inSysex)) {
						if(j<fs->Length) {
							c = (unsigned char)fs->ReadByte();
							j++;
							if(c == 0x41) {
								c = (unsigned char)fs->ReadByte();
								j++;
								if(c == 0x10) {
									// Skip two control byes
									c = (unsigned char)fs->ReadByte();
									j++;

									c = (unsigned char)fs->ReadByte();
									j++;

									sysexLen = 0;
									inSysex = true;
								} else {
									// FIXME: Implement channel specific sysex
									this->labelStatus->Text = S"Unsupported Sysex";
									this->importStatus->Visible = false;
									this->labelStatus->Refresh();
									fs->Close();
									return;
								}


							} else {
								this->labelStatus->Text = S"Import Error";
								this->importStatus->Visible = false;
								this->labelStatus->Refresh();
								fs->Close();
								return;
							}
						} else {
							this->labelStatus->Text = S"Import Error";
							this->importStatus->Visible = false;
							this->labelStatus->Refresh();
							fs->Close();
							return;
						}
					}
					this->importStatus->Value = (int)((j * 99) / fs->Length);
					this->importStatus->Refresh();
				}
				fs->Close();

			}
			if(foundSysex) {
				this->labelStatus->Text = S"Import Complete";
				this->importStatus->Visible = false;
			} else {
				this->labelStatus->Text = S"No Sysex Found";
				this->importStatus->Visible = false;
			}
			this->labelStatus->Refresh();
			

		}

	};
}