/* Copyright (C) 2011 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace mt32emu_win32drv_GUI {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			cbDACInputMode->SelectedIndex = 0;
			cbReverbMode->SelectedIndex = 0;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^  bClose;
	private: System::Windows::Forms::Button^  bRemove;

	private: System::Windows::Forms::Button^  bApply;





	private: System::Windows::Forms::NumericUpDown^  nudSampleRate;

	private: System::Windows::Forms::NumericUpDown^  nudVolume;
	private: System::Windows::Forms::NumericUpDown^  nudLatency;





	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;

	private: System::Windows::Forms::Label^  label4;

	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::CheckBox^  chbResetEnabled;


	private: System::Windows::Forms::ComboBox^  cbDACInputMode;
	private: System::Windows::Forms::CheckBox^  chbReverbEnabled;




	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::CheckBox^  chbReverbOverridden;


	private: System::Windows::Forms::NumericUpDown^  nudReverbGain;

	private: System::Windows::Forms::Button^  bInstall;
	private: System::Windows::Forms::NumericUpDown^  nudReverbTime;
	private: System::Windows::Forms::NumericUpDown^  nudReverbLevel;
	private: System::Windows::Forms::ComboBox^  cbReverbMode;




	protected: 

	protected: 

	protected: 

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->bClose = (gcnew System::Windows::Forms::Button());
			this->bRemove = (gcnew System::Windows::Forms::Button());
			this->bApply = (gcnew System::Windows::Forms::Button());
			this->nudSampleRate = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudVolume = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudLatency = (gcnew System::Windows::Forms::NumericUpDown());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->chbResetEnabled = (gcnew System::Windows::Forms::CheckBox());
			this->cbDACInputMode = (gcnew System::Windows::Forms::ComboBox());
			this->chbReverbEnabled = (gcnew System::Windows::Forms::CheckBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->chbReverbOverridden = (gcnew System::Windows::Forms::CheckBox());
			this->nudReverbGain = (gcnew System::Windows::Forms::NumericUpDown());
			this->bInstall = (gcnew System::Windows::Forms::Button());
			this->nudReverbTime = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudReverbLevel = (gcnew System::Windows::Forms::NumericUpDown());
			this->cbReverbMode = (gcnew System::Windows::Forms::ComboBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudSampleRate))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudVolume))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudLatency))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbGain))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbTime))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbLevel))->BeginInit();
			this->SuspendLayout();
			// 
			// bClose
			// 
			this->bClose->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->bClose->Location = System::Drawing::Point(365, 236);
			this->bClose->Name = L"bClose";
			this->bClose->Size = System::Drawing::Size(87, 23);
			this->bClose->TabIndex = 15;
			this->bClose->Text = L"Close";
			this->bClose->UseVisualStyleBackColor = true;
			this->bClose->Click += gcnew System::EventHandler(this, &Form1::bClose_Click);
			// 
			// bRemove
			// 
			this->bRemove->DialogResult = System::Windows::Forms::DialogResult::OK;
			this->bRemove->Location = System::Drawing::Point(254, 236);
			this->bRemove->Name = L"bRemove";
			this->bRemove->Size = System::Drawing::Size(87, 23);
			this->bRemove->TabIndex = 14;
			this->bRemove->Text = L"Remove Driver";
			this->bRemove->UseVisualStyleBackColor = true;
			this->bRemove->Click += gcnew System::EventHandler(this, &Form1::bRemove_Click);
			// 
			// bApply
			// 
			this->bApply->Location = System::Drawing::Point(32, 236);
			this->bApply->Name = L"bApply";
			this->bApply->Size = System::Drawing::Size(87, 23);
			this->bApply->TabIndex = 12;
			this->bApply->Text = L"Apply";
			this->bApply->UseVisualStyleBackColor = true;
			this->bApply->Click += gcnew System::EventHandler(this, &Form1::bApply_Click);
			// 
			// nudSampleRate
			// 
			this->nudSampleRate->Increment = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudSampleRate->Location = System::Drawing::Point(137, 17);
			this->nudSampleRate->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {96000, 0, 0, 0});
			this->nudSampleRate->Name = L"nudSampleRate";
			this->nudSampleRate->Size = System::Drawing::Size(123, 20);
			this->nudSampleRate->TabIndex = 0;
			this->nudSampleRate->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {32000, 0, 0, 0});
			// 
			// nudVolume
			// 
			this->nudVolume->Location = System::Drawing::Point(137, 126);
			this->nudVolume->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudVolume->Name = L"nudVolume";
			this->nudVolume->Size = System::Drawing::Size(123, 20);
			this->nudVolume->TabIndex = 4;
			this->nudVolume->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {100, 0, 0, 0});
			// 
			// nudLatency
			// 
			this->nudLatency->Location = System::Drawing::Point(137, 52);
			this->nudLatency->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudLatency->Name = L"nudLatency";
			this->nudLatency->Size = System::Drawing::Size(123, 20);
			this->nudLatency->TabIndex = 1;
			this->nudLatency->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {50, 0, 0, 0});
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(11, 21);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(87, 13);
			this->label1->TabIndex = 16;
			this->label1->Text = L"Sample Rate, Hz";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(11, 56);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(64, 13);
			this->label2->TabIndex = 17;
			this->label2->Text = L"Latency, ms";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(11, 94);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(86, 13);
			this->label4->TabIndex = 19;
			this->label4->Text = L"DAC Input Mode";
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(11, 130);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(64, 13);
			this->label5->TabIndex = 20;
			this->label5->Text = L"Output Gain";
			// 
			// chbResetEnabled
			// 
			this->chbResetEnabled->AutoSize = true;
			this->chbResetEnabled->Checked = true;
			this->chbResetEnabled->CheckState = System::Windows::Forms::CheckState::Checked;
			this->chbResetEnabled->Location = System::Drawing::Point(37, 206);
			this->chbResetEnabled->Name = L"chbResetEnabled";
			this->chbResetEnabled->Size = System::Drawing::Size(120, 17);
			this->chbResetEnabled->TabIndex = 9;
			this->chbResetEnabled->Text = L"Enable Synth Reset";
			this->chbResetEnabled->UseVisualStyleBackColor = true;
			// 
			// cbDACInputMode
			// 
			this->cbDACInputMode->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->cbDACInputMode->FormattingEnabled = true;
			this->cbDACInputMode->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"NICE", L"PURE", L"GENERATION1", L"GENERATION2"});
			this->cbDACInputMode->Location = System::Drawing::Point(137, 90);
			this->cbDACInputMode->Name = L"cbDACInputMode";
			this->cbDACInputMode->Size = System::Drawing::Size(123, 21);
			this->cbDACInputMode->TabIndex = 3;
			// 
			// chbReverbEnabled
			// 
			this->chbReverbEnabled->AutoSize = true;
			this->chbReverbEnabled->Checked = true;
			this->chbReverbEnabled->CheckState = System::Windows::Forms::CheckState::Checked;
			this->chbReverbEnabled->Location = System::Drawing::Point(202, 206);
			this->chbReverbEnabled->Name = L"chbReverbEnabled";
			this->chbReverbEnabled->Size = System::Drawing::Size(92, 17);
			this->chbReverbEnabled->TabIndex = 10;
			this->chbReverbEnabled->Text = L"Enable reverb";
			this->chbReverbEnabled->UseVisualStyleBackColor = true;
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(11, 165);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(102, 13);
			this->label6->TabIndex = 21;
			this->label6->Text = L"Reverb Output Gain";
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(287, 94);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(72, 13);
			this->label7->TabIndex = 22;
			this->label7->Text = L"Reverb Mode";
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(287, 130);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(68, 13);
			this->label8->TabIndex = 23;
			this->label8->Text = L"Reverb Time";
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Location = System::Drawing::Point(287, 165);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(71, 13);
			this->label9->TabIndex = 24;
			this->label9->Text = L"Reverb Level";
			// 
			// chbReverbOverridden
			// 
			this->chbReverbOverridden->AutoSize = true;
			this->chbReverbOverridden->Location = System::Drawing::Point(339, 206);
			this->chbReverbOverridden->Name = L"chbReverbOverridden";
			this->chbReverbOverridden->Size = System::Drawing::Size(115, 17);
			this->chbReverbOverridden->TabIndex = 11;
			this->chbReverbOverridden->Text = L"Use custom reverb";
			this->chbReverbOverridden->UseVisualStyleBackColor = true;
			// 
			// nudReverbGain
			// 
			this->nudReverbGain->Location = System::Drawing::Point(137, 161);
			this->nudReverbGain->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudReverbGain->Name = L"nudReverbGain";
			this->nudReverbGain->Size = System::Drawing::Size(123, 20);
			this->nudReverbGain->TabIndex = 5;
			this->nudReverbGain->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {100, 0, 0, 0});
			// 
			// bInstall
			// 
			this->bInstall->DialogResult = System::Windows::Forms::DialogResult::OK;
			this->bInstall->Location = System::Drawing::Point(143, 236);
			this->bInstall->Name = L"bInstall";
			this->bInstall->Size = System::Drawing::Size(87, 23);
			this->bInstall->TabIndex = 13;
			this->bInstall->Text = L"Install Driver";
			this->bInstall->UseVisualStyleBackColor = true;
			this->bInstall->Click += gcnew System::EventHandler(this, &Form1::bInstall_Click);
			// 
			// nudReverbTime
			// 
			this->nudReverbTime->Location = System::Drawing::Point(369, 126);
			this->nudReverbTime->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {7, 0, 0, 0});
			this->nudReverbTime->Name = L"nudReverbTime";
			this->nudReverbTime->Size = System::Drawing::Size(101, 20);
			this->nudReverbTime->TabIndex = 7;
			this->nudReverbTime->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {5, 0, 0, 0});
			// 
			// nudReverbLevel
			// 
			this->nudReverbLevel->Location = System::Drawing::Point(369, 161);
			this->nudReverbLevel->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {7, 0, 0, 0});
			this->nudReverbLevel->Name = L"nudReverbLevel";
			this->nudReverbLevel->Size = System::Drawing::Size(101, 20);
			this->nudReverbLevel->TabIndex = 8;
			this->nudReverbLevel->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {3, 0, 0, 0});
			// 
			// cbReverbMode
			// 
			this->cbReverbMode->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->cbReverbMode->FormattingEnabled = true;
			this->cbReverbMode->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"0: Room", L"1: Hall", L"2: Plate", L"3: Tap-delay"});
			this->cbReverbMode->Location = System::Drawing::Point(369, 90);
			this->cbReverbMode->Name = L"cbReverbMode";
			this->cbReverbMode->Size = System::Drawing::Size(101, 21);
			this->cbReverbMode->TabIndex = 6;
			// 
			// Form1
			// 
			this->AcceptButton = this->bApply;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->bClose;
			this->ClientSize = System::Drawing::Size(489, 283);
			this->Controls->Add(this->chbReverbOverridden);
			this->Controls->Add(this->label9);
			this->Controls->Add(this->label8);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->chbReverbEnabled);
			this->Controls->Add(this->cbReverbMode);
			this->Controls->Add(this->cbDACInputMode);
			this->Controls->Add(this->chbResetEnabled);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->nudLatency);
			this->Controls->Add(this->nudReverbGain);
			this->Controls->Add(this->nudReverbLevel);
			this->Controls->Add(this->nudReverbTime);
			this->Controls->Add(this->nudVolume);
			this->Controls->Add(this->nudSampleRate);
			this->Controls->Add(this->bApply);
			this->Controls->Add(this->bInstall);
			this->Controls->Add(this->bRemove);
			this->Controls->Add(this->bClose);
			this->Name = L"Form1";
			this->Text = L"MT32Emu Driver GUI";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudSampleRate))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudVolume))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudLatency))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbGain))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbTime))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudReverbLevel))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

int LoadIntValue(char *key, int nDefault, int maxValue) {
	int value = GetPrivateProfileIntA("mt32emu", key, nDefault, "mt32emu.ini");
	if (value < 0) {
		value =-value;
	}
	if (value > maxValue) {
		value = maxValue;
	}
	return value;
}

int StoreIntValue(char *key, int value) {
	char str[255];
	sprintf_s(str, 255,"%i", value);
	return WritePrivateProfileStringA("mt32emu", key, str, "mt32emu.ini");
}

void LoadSettings() {
	nudSampleRate->Value = LoadIntValue("SampleRate", 32000, 96000);
	nudLatency->Value = LoadIntValue("Latency", 60, 1000);
	chbResetEnabled->Checked = (LoadIntValue("ResetEnabled", 1, 1) != 0);
	chbReverbEnabled->Checked = (LoadIntValue("ReverbEnabled", 1, 1) != 0);
	chbReverbOverridden->Checked = (LoadIntValue("ReverbOverridden", 0, 1) != 0);
	cbReverbMode->SelectedIndex = LoadIntValue("ReverbMode", 0, 3);
	nudReverbTime->Value = LoadIntValue("ReverbTime", 5, 7);
	nudReverbLevel->Value = LoadIntValue("ReverbLevel", 3, 7);
	nudVolume->Value = LoadIntValue("OutputGain", 100, 1000);
	nudReverbGain->Value = LoadIntValue("ReverbOutputGain", 100, 1000);
	cbDACInputMode->SelectedIndex = LoadIntValue("DACInputMode", 3, 3);
}

void StoreSettings() {
	StoreIntValue("SampleRate", (int)nudSampleRate->Value);
	StoreIntValue("Latency", (int)nudLatency->Value);
	StoreIntValue("ResetEnabled", (int)chbResetEnabled->Checked);
	StoreIntValue("ReverbEnabled", (int)chbReverbEnabled->Checked);
	StoreIntValue("ReverbOverridden", (int)chbReverbOverridden->Checked);
	StoreIntValue("ReverbMode", (int)cbReverbMode->SelectedIndex);
	StoreIntValue("ReverbTime", (int)nudReverbTime->Value);
	StoreIntValue("ReverbLevel", (int)nudReverbLevel->Value);
	StoreIntValue("OutputGain", (int)nudVolume->Value);
	StoreIntValue("ReverbOutputGain", (int)nudReverbGain->Value);
	StoreIntValue("DACInputMode", (int)cbDACInputMode->SelectedIndex);
}

void InstallDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	char mt32emuDriverName[] = "mt32emu.dll";
	char drvName[] = "MIDI0";
	DWORD len, res;
	int freeDriver = -1;
	int mt32emuDriver = -1;
	for (int i = 0; i < 10; i++) {
		len = 255;
		if (i) {
			drvName[4] = '0' + i;
		} else {
			drvName[4] = 0;
		}
		res = RegQueryValueExA(hReg, drvName, NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			if ((freeDriver == -1) && (res == ERROR_FILE_NOT_FOUND)) {
				freeDriver = i;
			}
			continue;
		}
		if (!_stricmp(str, mt32emuDriverName)) {
			mt32emuDriver = i;
		}
	}
	if (mt32emuDriver != -1) {
		MessageBoxA(NULL, "Driver already installed.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	if (freeDriver == -1) {
		MessageBoxA(NULL, "Cannot install driver. There is no MIDI ports available.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	if (freeDriver) {
		drvName[4] = '0' + freeDriver;
	} else {
		drvName[4] = 0;
	}
	res = RegSetValueExA(hReg, drvName, NULL, REG_SZ, (LPBYTE)mt32emuDriverName, sizeof(mt32emuDriverName));
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, "Cannot register driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	MessageBoxA(NULL, "Driver successfully installed.", "Information", MB_OK | MB_ICONINFORMATION);
	RegCloseKey(hReg);
}

void RemoveDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	char mt32emuDriverName[] = "mt32emu.dll";
	char drvName[] = "MIDI0";
	DWORD len, res;
	int mt32emuDriver = -1;
	for (int i = 0; i < 10; i++) {
		len = 255;
		if (i) {
			drvName[4] = '0' + i;
		} else {
			drvName[4] = 0;
		}
		res = RegQueryValueExA(hReg, drvName, NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			continue;
		}
		if (!_stricmp(str, mt32emuDriverName)) {
			mt32emuDriver = i;
			res = RegDeleteValueA(hReg, drvName);
			if (res != ERROR_SUCCESS) {
				MessageBoxA(NULL, "Cannot uninstall driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
				RegCloseKey(hReg);
				return;
			}
		}
	}
	if (mt32emuDriver == -1) {
		MessageBoxA(NULL, "Cannot uninstall driver. There is no driver found.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	MessageBoxA(NULL, "Driver successfully uninstalled.", "Information", MB_OK | MB_ICONINFORMATION);
	RegCloseKey(hReg);
}

private: System::Void bClose_Click(System::Object^  sender, System::EventArgs^  e) {
					 this->Close();
				 }
private: System::Void bApply_Click(System::Object^  sender, System::EventArgs^  e) {
					 StoreSettings();
				 }
private: System::Void bInstall_Click(System::Object^  sender, System::EventArgs^  e) {
					 InstallDriver();
				 }
private: System::Void bRemove_Click(System::Object^  sender, System::EventArgs^  e) {
					 RemoveDriver();
				 }
private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
					 LoadSettings();
				 }
};
}