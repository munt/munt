#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for envelopeRender
	/// </summary>
	public __gc class envelopeRender : public System::Windows::Forms::UserControl
	{
	public:
		[Browsable(true)]
		__property bool get_isCentered() {
			return this->Centered;
		}
		__property void set_MyProperty( bool value ) {
			this->Centered = value;
		}
		[Browsable(false)]

		envelopeRender(void)
		{
			InitializeComponent();
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
		System::Boolean Centered;

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>		
		void InitializeComponent(void)
		{
			// 
			// envelopeRender
			// 
			this->Name = S"envelopeRender";
			this->Size = System::Drawing::Size(120, 120);

		}
	};
}