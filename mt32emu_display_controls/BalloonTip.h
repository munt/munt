#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace System::Globalization;
using namespace System::Text;


#include <string.h>

#pragma pack(1)
enum NotifyCommand 
{
	Add = 0,
	Modify = 1,
	Delete = 2,
	SetFocus = 3,
	SetVersion = 4
};
enum NotifyFlags
{
	Message = 1,
	Icon = 2,
	Tip = 4,
	State = 8,
	Info = 16,
	Guid = 32
};

struct NotifyIconData
{
	unsigned long cbSize;
	unsigned int  *hWnd;
	unsigned int  uID;
	unsigned int  uFlags;
	unsigned int  uCallbackMessage;
	unsigned int  *hIcon;
	unsigned char szTip[128];
	unsigned long dwState;
	unsigned long dwStateMask;
	unsigned char szInfo[256];
	unsigned int  uTimeoutOrVersion;
	unsigned char szInfoTitle[64];
	unsigned long dwInfoFlags;
};


#pragma pack()

namespace mt32emu_display_controls
{
	/// <summary> 
	/// Summary for BalloonTip
	/// </summary>
	__gc public class BalloonTip :  public System::ComponentModel::Component
	{

	public:

		BalloonTip(void)
		{
			InitializeComponent();
		}
		BalloonTip(System::ComponentModel::IContainer *container) : components(0)
		{
			/// <summary>
			/// Required for Windows.Forms Class Composition Designer support
			/// </summary>

			container->Add(this);
			InitializeComponent();
		}


		[DllImport(S"shell32.dll")]
		static System::Int32 Shell_NotifyIcon(NotifyCommand cmd, System::IntPtr NotifyStruct);

		[DllImport(S"kernel32.dll")]
		static System::UInt32 GetCurrentThreadId();
		__delegate System::Int32 EnumThreadWndProc(int * hWnd, System::UInt32 lParam);

		[DllImport(S"user32.dll")]
		static System::Int32 EnumThreadWindows(System::UInt32 threadId, EnumThreadWndProc * callBack, System::UInt32 param);

		[DllImport(S"user32.dll")]
		static System::Int32 GetClassName(int * hWnd, System::Text::StringBuilder * className, System::Int32 maxCount);

	public: System::Void ShowBallon(int iconId, char * title, char * text, int timeout)
		{

			Encoding * ascii = Encoding::ASCII;

			int threadId = GetCurrentThreadId();
			EnumThreadWndProc * cb = new EnumThreadWndProc(this, &BalloonTip::FindNotifyWindowCallback);
			m_foundNotifyWindow = false;
			EnumThreadWindows(threadId, cb, 0);
			if(m_foundNotifyWindow)
			{
				NotifyIconData data;
				data.cbSize = sizeof(NotifyIconData);
				data.hWnd = (unsigned int *)m_notifyWindow;
				data.uID = iconId;
				data.uFlags = Info;
				data.dwInfoFlags = 0;
				data.uTimeoutOrVersion = timeout;
				strcpy((char *)&data.szInfo[0], text);
				strcpy((char *)&data.szInfoTitle[0], title);

				Shell_NotifyIcon(Modify, &data);
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
				
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container *components;
		int * m_notifyWindow;
		bool m_foundNotifyWindow;

		System::Int32 FindNotifyWindowCallback(int * hWnd, System::UInt32 lParam)
		{
			System::Text::StringBuilder * buffer = new System::Text::StringBuilder(256);
			GetClassName(hWnd, buffer, buffer->Capacity);

			if(buffer->ToString()->Equals(S"WindowsForms10.Window.0.app4"))
			{
				m_notifyWindow = hWnd;
				m_foundNotifyWindow = true;
				return 0;
			}
			return 1;
		}

		
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