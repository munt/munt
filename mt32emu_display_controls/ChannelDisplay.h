#pragma once

#include <stdlib.h>
#include <string.h>

#include "ControlInterface.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

struct chanInfo {
	bool isPlaying;
	int assignedPart;
	int freq;
};

chanInfo *chanList;


namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for ChannelDisplay
	/// </summary>
	public __gc class ChannelDisplay : public System::Windows::Forms::UserControl
	{
	public: 
		ChannelDisplay(void)
		{
			
			chanList = NULL;
			lockedVol = false;
			InitializeComponent();
			this->noteBox = new System::Windows::Forms::PictureBox*[9];
			this->chanVol = new System::Windows::Forms::HScrollBar*[9];

			int i;
			for(i=0;i<9;i++) {
				this->noteBox[i] = new System::Windows::Forms::PictureBox();
				this->channelStatus->Controls->Add(this->noteBox[i]);

				this->chanVol[i] = new System::Windows::Forms::HScrollBar();
				this->channelStatus->Controls->Add(this->chanVol[i]);

				this->noteBox[i]->Location = System::Drawing::Point(216, 24 + (i * 24));
				this->noteBox[i]->Name = S"pictureBox2";
				this->noteBox[i]->Size = System::Drawing::Size(384, 16);
				//this->noteBox[i]->BackColor = System::Drawing::Color::Wheat;
				this->noteBox[i]->TabIndex = i;
				this->noteBox[i]->TabStop = false;
				this->noteBox[i]->Paint += new System::Windows::Forms::PaintEventHandler(this, &ChannelDisplay::noteBox_Paint);

				this->chanVol[i]->Location = System::Drawing::Point(104, 28 + (i * 24));
				this->chanVol[i]->Name = S"hScrollBar1";
				this->chanVol[i]->Minimum = 0;
				this->chanVol[i]->Maximum = 127;
				this->chanVol[i]->Size = System::Drawing::Size(104, 8);
				this->chanVol[i]->TabIndex = i;
				this->chanVol[i]->Scroll += new ScrollEventHandler(this, &ChannelDisplay::volBar_Scroll);

			}


		}

		System::Void sendUpdateData(char * updateData, int count, chanInfo *chList) {
		
			unsigned short * buffer;

			buffer = (unsigned short *)updateData;

			chanList = chList;

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

			int i;

			int bufloc = (count * 3) + 3 + (5 * 8);
			for(i=0;i<9;i++) {
				this->chanVol[i]->Value = buffer[bufloc + i];
				this->noteBox[i]->Refresh();
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
	private: System::Windows::Forms::GroupBox *  channelStatus;
	private: System::Windows::Forms::Label *  label9;
	private: System::Windows::Forms::Label *  chan8;
	private: System::Windows::Forms::Label *  chan7;
	private: System::Windows::Forms::Label *  chan6;
	private: System::Windows::Forms::Label *  chan5;
	private: System::Windows::Forms::Label *  chan4;
	private: System::Windows::Forms::Label *  chan3;
	private: System::Windows::Forms::Label *  chan2;
	private: System::Windows::Forms::Label *  chan1;
	private: System::Windows::Forms::PictureBox *  noteBox[];
	private: System::Windows::Forms::HScrollBar *  chanVol[];
	private: System::Boolean lockedVol;



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
			this->channelStatus = new System::Windows::Forms::GroupBox();
			this->label9 = new System::Windows::Forms::Label();
			this->chan8 = new System::Windows::Forms::Label();
			this->chan7 = new System::Windows::Forms::Label();
			this->chan6 = new System::Windows::Forms::Label();
			this->chan5 = new System::Windows::Forms::Label();
			this->chan4 = new System::Windows::Forms::Label();
			this->chan3 = new System::Windows::Forms::Label();
			this->chan2 = new System::Windows::Forms::Label();
			this->chan1 = new System::Windows::Forms::Label();
			this->channelStatus->SuspendLayout();
			this->SuspendLayout();
			// 
			// channelStatus
			// 
			this->channelStatus->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->channelStatus->Controls->Add(this->label9);
			this->channelStatus->Controls->Add(this->chan8);
			this->channelStatus->Controls->Add(this->chan7);
			this->channelStatus->Controls->Add(this->chan6);
			this->channelStatus->Controls->Add(this->chan5);
			this->channelStatus->Controls->Add(this->chan4);
			this->channelStatus->Controls->Add(this->chan3);
			this->channelStatus->Controls->Add(this->chan2);
			this->channelStatus->Controls->Add(this->chan1);
			this->channelStatus->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->channelStatus->Location = System::Drawing::Point(0, 0);
			this->channelStatus->Name = S"channelStatus";
			this->channelStatus->Size = System::Drawing::Size(608, 240);
			this->channelStatus->TabIndex = 12;
			this->channelStatus->TabStop = false;
			this->channelStatus->Text = S"Channel Status";
			// 
			// label9
			// 
			this->label9->Location = System::Drawing::Point(8, 216);
			this->label9->Name = S"label9";
			this->label9->Size = System::Drawing::Size(88, 16);
			this->label9->TabIndex = 12;
			this->label9->Text = S"Rhythm Channel";
			// 
			// chan8
			// 
			this->chan8->Location = System::Drawing::Point(8, 192);
			this->chan8->Name = S"chan8";
			this->chan8->Size = System::Drawing::Size(88, 16);
			this->chan8->TabIndex = 13;
			this->chan8->Text = S"Channel 8";
			// 
			// chan7
			// 
			this->chan7->Location = System::Drawing::Point(8, 168);
			this->chan7->Name = S"chan7";
			this->chan7->Size = System::Drawing::Size(88, 16);
			this->chan7->TabIndex = 14;
			this->chan7->Text = S"Channel 7";
			// 
			// chan6
			// 
			this->chan6->Location = System::Drawing::Point(8, 144);
			this->chan6->Name = S"chan6";
			this->chan6->Size = System::Drawing::Size(88, 16);
			this->chan6->TabIndex = 15;
			this->chan6->Text = S"Channel 6";
			// 
			// chan5
			// 
			this->chan5->Location = System::Drawing::Point(8, 120);
			this->chan5->Name = S"chan5";
			this->chan5->Size = System::Drawing::Size(88, 16);
			this->chan5->TabIndex = 16;
			this->chan5->Text = S"Channel 5";
			// 
			// chan4
			// 
			this->chan4->Location = System::Drawing::Point(8, 96);
			this->chan4->Name = S"chan4";
			this->chan4->Size = System::Drawing::Size(88, 16);
			this->chan4->TabIndex = 17;
			this->chan4->Text = S"Channel 4";
			// 
			// chan3
			// 
			this->chan3->Location = System::Drawing::Point(8, 72);
			this->chan3->Name = S"chan3";
			this->chan3->Size = System::Drawing::Size(88, 16);
			this->chan3->TabIndex = 18;
			this->chan3->Text = S"Channel 3";
			// 
			// chan2
			// 
			this->chan2->Location = System::Drawing::Point(8, 48);
			this->chan2->Name = S"chan2";
			this->chan2->Size = System::Drawing::Size(88, 16);
			this->chan2->TabIndex = 19;
			this->chan2->Text = S"Channel 2";
			// 
			// chan1
			// 
			this->chan1->Location = System::Drawing::Point(8, 24);
			this->chan1->Name = S"chan1";
			this->chan1->Size = System::Drawing::Size(88, 16);
			this->chan1->TabIndex = 20;
			this->chan1->Text = S"Channel 1";
			// 
			// ChannelDisplay
			// 
			this->Controls->Add(this->channelStatus);
			this->Name = S"ChannelDisplay";
			this->Size = System::Drawing::Size(608, 240);
			this->channelStatus->ResumeLayout(false);
			this->ResumeLayout(false);

		}



	private: System::Void volBar_Scroll(System::Object *sender, System::Windows::Forms::ScrollEventArgs * e) {
				 System::Windows::Forms::HScrollBar * sb = dynamic_cast<System::Windows::Forms::HScrollBar *>(sender);
				 
				 if(this->Tag != NULL) {
					//System::Windows::Forms::Form * frm = dynamic_cast<System::Windows::Forms::Form *>(this->Parent);
					//frm->Hide();
					ControlInterface * parent;
					parent = dynamic_cast<ControlInterface*>(this->Tag);
					//if(parent != NULL) {
						parent->SetVolume(sb->TabIndex, e->NewValue);
					//}
				 }
				 
			 }

	private: System::Void noteBox_Paint(System::Object * sender, System::Windows::Forms::PaintEventArgs* e) {
				Graphics * g = e->Graphics;
				

				g->Clear(System::Drawing::Color::FromArgb(69, 69, 73));

				 if(chanList == NULL) return;
				int i;
				System::Windows::Forms::PictureBox *pb = dynamic_cast<System::Windows::Forms::PictureBox *>(sender);

				i = pb->TabIndex;
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
				for(j=0;j<notesPlaying;j++) {
					int xat;
					xat = (noteList[j] - 12) * 4;
					g->FillRectangle(System::Drawing::Brushes::Lime, xat, 0, 4, 24);
				}

				

			 }


};
}