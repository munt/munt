#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KnobInterface.h"
#include "ControlInterface.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


int ButtonList[][4]= {
	{394,44, 412,65},
	{432,44, 452,65},
	{472,44, 492,65},

	{394,84, 412,105},
	{432,84, 452,105},
	{472,84, 492,105},
	
	{522,44, 574,65},
	{595,44, 647,65},

	{522,84, 574,105},
	{595,84, 647,105},
	{-1,-1, -1,-1}};






namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for FacePlate
	/// </summary>
public __gc class FacePlate : public System::Windows::Forms::UserControl, public mt32emu_display_controls::KnobInterface
	{
	public: 
		FacePlate(void)
		{
			buttonsSelected = new System::Boolean[32];
			InitializeComponent();
			displayMode = 0;
			systemParam = new MT32Emu::MemParams::SystemArea();
			setLCDText(S"1 2 3 4 5 R |vol:100");
		}

	System::Void knobUpdated(int newValue) {
			 mt32emu_display_controls::KnobInterface *ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->Parent);
			 ki->knobUpdated(newValue);
			 if(displayMode == 0) {
				ControlInterface * parent;
				parent = dynamic_cast<ControlInterface*>(this->Tag);
				systemParam->masterVol = newValue;

				parent->writeSynthMemory(0x100000, 0x17, (char *)systemParam);

			 }
		}

	System::Void requestInfo(int addr, int len) {
		}
	System::Void sendInfo(int addr, int len, char * buf) {
			if(addr == 0x100000) {
				memcpy(systemParam, buf, len);
			}
			if((addr == 0x100000) && (displayMode == 0)) {
				char strBuf[512];
				sprintf(strBuf, "1 2 3 4 5 R |vol:%3d", buf[0x16]);
				setLCDText(new String(strBuf));
			}

		}
	System::Void DoEvents() {
            mt32emu_display_controls::KnobInterface *ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->Parent);
			switch(displayMode) {
				case 0:
					ki->requestInfo(0x100000, 0x17);
					break;
				default:
					break;
			}

		}

	System::Void turnOnModule() {
		if(!this->lcdDisplay1->Visible)
			this->lcdDisplay1->Visible = true;
	}

	System::Void turnOffModule() {
		if(this->lcdDisplay1->Visible)
			this->lcdDisplay1->Visible = false;
	}


	System::Void setLCDText(System::String * lcdText) {
				this->lcdDisplay1->setDisplayText(lcdText);
			}
	System::Void workMidiLight(System::Drawing::Color useColor) {
				this->midiLight->set_BackColor(useColor);
			}	

	System::Void setMaskedChar(int i, bool value) {
				this->lcdDisplay1->setMaskedChar(i, value);
			}

	int getKnobValue() {
				return this->displayKnob1->getValue();
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
		void OnMouseHover(System::EventArgs * e) {
			__super::OnMouseHover(e);
		}

		
		void OnMouseMove(/*System::Object * sender, */System::Windows::Forms::MouseEventArgs * e) {
			int i = 0;
			int xat = e->get_X();
			int yat = e->get_Y();
			bool found = false;
			bool changed = false;

			while(ButtonList[i][0] != -1) {
				this->buttonsSelected[i + 16] = false;
				int xmin = ButtonList[i][0];
				int xmax = ButtonList[i][2];

				int ymin = ButtonList[i][1];
				int ymax = ButtonList[i][3];

				if((xat >= xmin) && (xat <= xmax)) {
					if((yat >= ymin) && (yat <= ymax)) {
						this->buttonsSelected[i + 16] = true;
						found = true;
					}
				}

				if(this->buttonsSelected[i] != this->buttonsSelected[i + 16]) changed = true;
				this->buttonsSelected[i] = this->buttonsSelected[i + 16];


				i++;
			}
			if(found) {
				this->Cursor = System::Windows::Forms::Cursors::Hand;
			} else {
				this->Cursor = System::Windows::Forms::Cursors::Default;
			}
			

			if(changed) this->Invalidate();

			//__super::OnMouseMove(e);
		}

	


	private: System::Windows::Forms::PictureBox *  midiLight;
	private: System::Boolean buttonsSelected[];
	private: System::Int32 displayMode;
	private: mt32emu_display_controls::LCDDisplay *  lcdDisplay1;
	private: mt32emu_display_controls::DisplayKnob *  displayKnob1;
	private: MT32Emu::MemParams::SystemArea * systemParam;


    
	

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
			System::Resources::ResourceManager *  resources = new System::Resources::ResourceManager(__typeof(mt32emu_display_controls::FacePlate));
			this->midiLight = new System::Windows::Forms::PictureBox();
			this->lcdDisplay1 = new mt32emu_display_controls::LCDDisplay();
			this->displayKnob1 = new mt32emu_display_controls::DisplayKnob();
			this->SuspendLayout();
			this->midiLight->BackColor = System::Drawing::Color::FromArgb((System::Byte)41, (System::Byte)42, (System::Byte)51);
			this->midiLight->Location = System::Drawing::Point(295, 74);
			this->midiLight->Name = S"midiLight";
			this->midiLight->Size = System::Drawing::Size(10, 2);
			this->midiLight->TabIndex = 2;
			this->midiLight->TabStop = false;
			this->lcdDisplay1->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"lcdDisplay1.BackgroundImage")));
			this->lcdDisplay1->Location = System::Drawing::Point(77, 62);
			this->lcdDisplay1->Name = S"lcdDisplay1";
			this->lcdDisplay1->Size = System::Drawing::Size(187, 28);
			this->lcdDisplay1->TabIndex = 3;
			this->lcdDisplay1->Visible = false;
			this->displayKnob1->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"displayKnob1.BackgroundImage")));
			this->displayKnob1->Location = System::Drawing::Point(689, 40);
			this->displayKnob1->Name = S"displayKnob1";
			this->displayKnob1->Size = System::Drawing::Size(74, 69);
			this->displayKnob1->TabIndex = 4;
			this->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"$this.BackgroundImage")));
			this->Controls->Add(this->displayKnob1);
			this->Controls->Add(this->lcdDisplay1);
			this->Controls->Add(this->midiLight);
			this->Name = S"FacePlate";
			this->Size = System::Drawing::Size(800, 136);
			this->ResumeLayout(false);

		}

	public: System::Void OnPaint(/*System::Object * sender, */System::Windows::Forms::PaintEventArgs* e) {
				Graphics * g = e->Graphics;

				//g->DrawImage(this->pictureBox1->BackgroundImage, 0,0, 800, 140);
				//g->Clear(System::Drawing::Color::FromArgb(69, 69, 73));
				int i = 0;
				while(ButtonList[i][0] != -1) {
					if(this->buttonsSelected[i]) {
						g->DrawRectangle(System::Drawing::Pens::Snow, ButtonList[i][0], ButtonList[i][1], ButtonList[i][2] - ButtonList[i][0], ButtonList[i][3] - ButtonList[i][1]);
					}
					i++;
				}
				
				
			 }


	};



}