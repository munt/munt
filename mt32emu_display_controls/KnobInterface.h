#pragma once



using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;


namespace mt32emu_display_controls
{
	public __gc __interface KnobInterface
	{
		System::Void knobUpdated(int newValue);
		System::Void requestInfo(int addr, int len);
		System::Void sendInfo(int addr, int len, char * buf);
		System::Void DoEvents();
	};
};
