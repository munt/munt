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
	/// Summary for mt32emu_display_controlsControl
	/// </summary>
	///
	/// WARNING: If you change the name of this class, you will need to change the 
	///          'Resource File Name' property for the managed resource compiler tool 
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	public __gc class mt32emu_display_controlsControl : public System::Windows::Forms::UserControl
	{	
	public:
		mt32emu_display_controlsControl(void)	
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

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			// 
			// mt32emu_display_controlsControl
			// 
			this->Name = S"mt32emu_display_controlsControl";
			this->Size = System::Drawing::Size(824, 200);

		}
	};
}


