#pragma once

#include "KnobInterface.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;


namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for DisplayKnob
	/// </summary>
	public __gc class DisplayKnob : public System::Windows::Forms::UserControl
	{
	public: 
		DisplayKnob(void)
		{
			InitializeComponent();
			bitBuffer = new Bitmap(50, 50);
			angle = 0;
			targetangle = 0;
		}
		int getValue() {
			return (int)(angle / 3);
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
		void updateMouse(MouseEventArgs *e) {
			// Center is at 41, 36
			if(e->Button == MouseButtons::Left) {
				double newangle = this->calcAngle(e->X, e->Y, 41, 36) + 60;
				while(newangle > 359)
					newangle -= 359;
				if(newangle <= 300) 
					targetangle = newangle;

				if(Math::Abs(targetangle - angle) < 10) {
					angle = targetangle;
					notifyChange();
					this->Invalidate();
				} else {
					timer1->Enabled = true;
				}

			}
		}
		void OnMouseMove(/*System::Object * sender, */System::Windows::Forms::MouseEventArgs * e) {
			updateMouse(e);
		}
		void OnMouseDown(/*System::Object * sender, */System::Windows::Forms::MouseEventArgs * e) {
			updateMouse(e);
		}
		void OnMouseWheel(System::Windows::Forms::MouseEventArgs * e) {
			double newangle = angle + (e->Delta / 3);
			if(newangle < 0) newangle = 0;
			if (newangle > 300) newangle = 300;
			
			targetangle = newangle;

			if(Math::Abs(targetangle - angle) < 10) {
				angle = targetangle;
				notifyChange();
				this->Invalidate();
			} else {
				timer1->Enabled = true;
			}
		}
	private: System::Windows::Forms::ImageList *  imageList1;
	private: System::ComponentModel::IContainer *  components;
	private: System::Drawing::Bitmap* bitBuffer;
	private: System::Windows::Forms::Timer *  timer1;

	private: System::Double angle;
	private: System::Double targetangle;

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
			System::Resources::ResourceManager *  resources = new System::Resources::ResourceManager(__typeof(mt32emu_display_controls::DisplayKnob));
			this->imageList1 = new System::Windows::Forms::ImageList(this->components);
			this->timer1 = new System::Windows::Forms::Timer(this->components);
			// 
			// imageList1
			// 
			this->imageList1->ColorDepth = System::Windows::Forms::ColorDepth::Depth32Bit;
			this->imageList1->ImageSize = System::Drawing::Size(50, 50);
			this->imageList1->ImageStream = (__try_cast<System::Windows::Forms::ImageListStreamer *  >(resources->GetObject(S"imageList1.ImageStream")));
			this->imageList1->TransparentColor = System::Drawing::Color::Transparent;
			// 
			// timer1
			// 
			this->timer1->Interval = 10;
			this->timer1->Tick += new System::EventHandler(this, &DisplayKnob::timer1_Tick);
			// 
			// DisplayKnob
			// 
			this->BackgroundImage = (__try_cast<System::Drawing::Image *  >(resources->GetObject(S"$this.BackgroundImage")));
			this->Cursor = System::Windows::Forms::Cursors::Hand;
			this->Name = S"DisplayKnob";
			this->Size = System::Drawing::Size(74, 69);

		}
	public: System::Void OnPaint(System::Windows::Forms::PaintEventArgs* e) {
				Graphics * dispGraph = e->Graphics;
				Graphics * bufGraph = Graphics::FromImage(bitBuffer);


				
				//bufGraph->Clear(Color::Transparent);
				Matrix * X = new Matrix();
				X->RotateAt((float)angle, PointF(25,25));
				bufGraph->Transform = X;
				
				bufGraph->DrawImage(this->imageList1->Images->Item[0], 0, 0, 50, 50);

				dispGraph->DrawImage(bitBuffer, 16, 11, 50, 50);
			}

	private: System::Double calcAngle(int x1, int y1, int x2, int y2) {
				 double dx = x2 - x1;
				 double dy = y2 - y1;

				 double ang = 0;
				 if(dx == 0) {
					 if(dy == 0) 
						 ang = 0;
					 else if (dy > 0)
						 ang = System::Math::PI / 2.0;
					 else
						 ang = System::Math::PI * 3.0 / 2.0;
				 }
				 else if (dy == 0) {
					 if(dx > 0)
						 ang = 0;
					 else
						 ang = System::Math::PI;
				 }
				 else {
					 if (dx < 0)
						 ang = System::Math::Atan(dy / dx) + System::Math::PI;
					 else if (dy < 0)
						 ang = System::Math::Atan(dy / dx) + (2 * System::Math::PI);
					 else
						 ang = System::Math::Atan(dy / dx);
				 }

				 ang = ang * 180 / System::Math::PI;

				 return ang;
			 }

private: System::Void notifyChange() {
			 mt32emu_display_controls::KnobInterface *ki = dynamic_cast<mt32emu_display_controls::KnobInterface *>(this->Parent);
			 ki->knobUpdated((int)(angle/3));
		 }


private: System::Void timer1_Tick(System::Object *  sender, System::EventArgs *  e)
		 {
			 if (Math::Abs(targetangle - angle) < 5) {
				 angle = targetangle;
			 } else {
				 if(targetangle > angle) {
					 angle += 5;
				 } else {
					 angle -= 5;
				 }
			 }

			 if(angle == targetangle) 
				 timer1->Enabled = false;
			 notifyChange();

			 this->Invalidate();


		 }

};

		

}