#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Diagnostics;

 
namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for ControlInterface
	/// </summary>
	__gc public class ControlInterface :  public System::ComponentModel::Component
	{
	public:
		ControlInterface(void)
		{
			InitializeComponent();
		}
		ControlInterface(System::ComponentModel::IContainer *container) : components(0)
		{
			/// <summary>
			/// Required for Windows.Forms Class Composition Designer support
			/// </summary>

			container->Add(this);
			InitializeComponent();
		}

		System::Void SetVolume(int channel, int volume) {
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
		System::ComponentModel::Container *components;
		
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>		
		void InitializeComponent(void)
		{
			components = new System::ComponentModel::Container();
		}
	};
}