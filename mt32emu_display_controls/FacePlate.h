#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


int ButtonList[][4]= {
	{394,44, 412,64},
	{432,44, 452,64},
	{472,44, 492,64},

	{394,84, 412,104},
	{432,84, 452,104},
	{472,84, 492,104},
	
	{522,44, 574,64},
	{595,44, 647,64},

	{522,84, 574,104},
	{595,84, 647,104},
	{-1,-1, -1,-1}};






namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for FacePlate
	/// </summary>
	public __gc class FacePlate : public System::Windows::Forms::UserControl
	{
	public: 
		FacePlate(void)
		{
			buttonsSelected = new System::Boolean[32];
			InitializeComponent();
			this->pictureBox1->Paint += new System::Windows::Forms::PaintEventHandler(this, &FacePlate::facePlate_Paint);
			this->pictureBox1->MouseMove += new System::Windows::Forms::MouseEventHandler(this, &FacePlate::OnMouseMoveHandler);
			
		}

	System::Void setLCDText(System::String * lcdText) {
				this->lcdDisplay->set_Text(lcdText);
			}
	System::Void workMidiLight(System::Drawing::Color useColor) {
				this->midiLight->set_BackColor(useColor);
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

		
		void OnMouseMoveHandler(System::Object * sender, System::Windows::Forms::MouseEventArgs * e) {
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
				this->pictureBox1->Cursor = System::Windows::Forms::Cursors::Hand;
			} else {
				this->pictureBox1->Cursor = System::Windows::Forms::Cursors::Default;
			}
			

			if(changed) this->Refresh();

			__super::OnMouseMove(e);
		}

	
	private: System::Windows::Forms::PictureBox *  pictureBox1;
	private: System::Windows::Forms::Label *  lcdDisplay;
	private: System::Windows::Forms::PictureBox *  midiLight;
	private: System::Boolean buttonsSelected[];
	

    
	

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
			this->pictureBox1 = new System::Windows::Forms::PictureBox();
			this->lcdDisplay = new System::Windows::Forms::Label();
			this->midiLight = new System::Windows::Forms::PictureBox();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			this->pictureBox1->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"pictureBox1.BackgroundImage")));
			this->pictureBox1->Location = System::Drawing::Point(0, 0);
			this->pictureBox1->Name = S"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(800, 136);
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			// 
			// lcdDisplay
			// 
			this->lcdDisplay->BackColor = System::Drawing::Color::FromArgb((System::Byte)67, (System::Byte)61, (System::Byte)66);
			this->lcdDisplay->Font = new System::Drawing::Font(S"LcdD", 15.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, (System::Byte)0);
			this->lcdDisplay->ForeColor = System::Drawing::Color::Lime;
			this->lcdDisplay->Location = System::Drawing::Point(80, 64);
			this->lcdDisplay->Name = S"lcdDisplay";
			this->lcdDisplay->Size = System::Drawing::Size(176, 24);
			this->lcdDisplay->TabIndex = 1;
			// 
			// midiLight
			// 
			this->midiLight->BackColor = System::Drawing::Color::FromArgb((System::Byte)41, (System::Byte)42, (System::Byte)51);
			this->midiLight->Location = System::Drawing::Point(295, 74);
			this->midiLight->Name = S"midiLight";
			this->midiLight->Size = System::Drawing::Size(10, 2);
			this->midiLight->TabIndex = 2;
			this->midiLight->TabStop = false;
			// 
			// FacePlate
			// 
			this->Controls->Add(this->midiLight);
			this->Controls->Add(this->lcdDisplay);
			this->Controls->Add(this->pictureBox1);
			this->Name = S"FacePlate";
			this->Size = System::Drawing::Size(800, 136);
			this->ResumeLayout(false);

		}

	private: System::Void facePlate_Paint(System::Object * sender, System::Windows::Forms::PaintEventArgs* e) {
				Graphics * g = e->Graphics;

				g->DrawImage(this->pictureBox1->BackgroundImage, 0,0, 800, 140);
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