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
	/// Summary for PartialDisplay
	/// </summary>
	public __gc class PartialDisplay : public System::Windows::Forms::UserControl
	{
	public: 
		PartialDisplay(void)
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
	private: System::Windows::Forms::GroupBox *  partialStatus;
	private: System::Windows::Forms::Label *  label1;

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
			this->partialStatus = new System::Windows::Forms::GroupBox();
			this->label1 = new System::Windows::Forms::Label();
			this->partialStatus->SuspendLayout();
			this->SuspendLayout();
			// 
			// partialStatus
			// 
			this->partialStatus->BackColor = System::Drawing::Color::FromArgb((System::Byte)56, (System::Byte)56, (System::Byte)60);
			this->partialStatus->Controls->Add(this->label1);
			this->partialStatus->ForeColor = System::Drawing::SystemColors::ControlLightLight;
			this->partialStatus->Location = System::Drawing::Point(0, 0);
			this->partialStatus->Name = S"partialStatus";
			this->partialStatus->Size = System::Drawing::Size(608, 240);
			this->partialStatus->TabIndex = 13;
			this->partialStatus->TabStop = false;
			this->partialStatus->Text = S"Partial Status";
			// 
			// label1
			// 
			this->label1->Location = System::Drawing::Point(8, 112);
			this->label1->Name = S"label1";
			this->label1->Size = System::Drawing::Size(592, 16);
			this->label1->TabIndex = 0;
			this->label1->Text = S"Partial status tab not yet implemented.";
			this->label1->TextAlign = System::Drawing::ContentAlignment::TopCenter;
			// 
			// PartialDisplay
			// 
			this->Controls->Add(this->partialStatus);
			this->Name = S"PartialDisplay";
			this->Size = System::Drawing::Size(608, 240);
			this->partialStatus->ResumeLayout(false);
			this->ResumeLayout(false);

		}
	};
}