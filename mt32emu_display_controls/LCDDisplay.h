#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Globalization;
using namespace System::Text;

extern unsigned char Font_6x8[96][8];

namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for LCDDisplay
	/// </summary>
	public __gc class LCDDisplay : public System::Windows::Forms::UserControl
	{
	public: 
		LCDDisplay(void)
		{
			InitializeComponent();
			currentText = S"";
			backPen = new System::Drawing::SolidBrush(Color::FromArgb(98, 127, 0));
			frontPen = new System::Drawing::SolidBrush(Color::FromArgb(212, 234, 0));
			offBuffer = new Bitmap(680, 51);
			doubleBuffer = new Bitmap(171, 15);

		}

		System::Void setDisplayText(System::String * useText) {
			currentText = useText;
			this->Invalidate();
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
		       
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container* components;
		System::String * currentText;
		System::Drawing::SolidBrush * backPen;
		System::Drawing::SolidBrush * frontPen;
		Bitmap * offBuffer;
		Bitmap * doubleBuffer;

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>		
		void InitializeComponent(void)
		{
			System::Resources::ResourceManager *  resources = new System::Resources::ResourceManager(__typeof(mt32emu_display_controls::LCDDisplay));
			// 
			// LCDDisplay
			// 
			this->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"$this.BackgroundImage")));
			this->Name = S"LCDDisplay";
			this->Size = System::Drawing::Size(187, 28);

		}

	public: System::Void OnPaint(System::Windows::Forms::PaintEventArgs* e) {
				Graphics * dispGraph = e->Graphics;

				//dispGraph->DrawImage(this->BackgroundImage, 0,0, 187, 28);

				Encoding * ascii = Encoding::ASCII;

				Byte asciiString[] = ascii->GetBytes(this->currentText);

				Graphics * g = Graphics::FromImage(this->offBuffer);
				Graphics * dbg = Graphics::FromImage(this->doubleBuffer);
				//Bitmap * offBuffer = new Bitmap(151, 15);

				int xat, xstart, yat, i;
				xstart = 0;
				yat = 0;

				for(i=0;i<20;i++) {
					int t,m;
					unsigned char c;
					c = 0x20;
					if(i<currentText->Length) {
						c = asciiString[i];
					}

					// Don't render characters we don't have mapped
					if(c<0x20) c = 0x20;
					if(c>0x7f) c = 0x20;

					c -= 0x20;

					yat = 1;
					for(t=0;t<8;t++) {
						xat = xstart;
						unsigned char fval = Font_6x8[c][t];
						for(m=4;m>=0;--m) {
							if((fval >> m) & 1) {
								g->FillRectangle(this->frontPen, xat, yat, 5, 5);
							} else {
								g->FillRectangle(this->backPen, xat, yat, 5, 5);
							}

							xat+=6;
						}
						yat+=6;
						if(t==6) yat+=6;
					}
					xstart+=34;
				}
				
				dbg->InterpolationMode = System::Drawing::Drawing2D::InterpolationMode::HighQualityBicubic;
				dbg->Clear(Color::Transparent);
				dbg->DrawImage(offBuffer,0,0,171,15);
				dbg->Flush();

				dispGraph->InterpolationMode = System::Drawing::Drawing2D::InterpolationMode::NearestNeighbor;
				dispGraph->DrawImage(doubleBuffer, 7, 7, 171, 15);
				
			 }


	};
}