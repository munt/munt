/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "Master.h"
#include "SynthPropertiesDialog.h"
#include "ROMSelectionDialog.h"
#include "ui_SynthPropertiesDialog.h"

SynthPropertiesDialog::SynthPropertiesDialog(QWidget *parent, SynthRoute *useSynthRoute) :
	QDialog(parent),
	ui(new Ui::SynthPropertiesDialog),
	synthRoute(useSynthRoute),
	rsd(synthProfile, this)
{
	ui->setupUi(this);
	if (ui->formLayout->fieldGrowthPolicy() == QFormLayout::FieldsStayAtSizeHint) {
		ui->formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
	}
	refreshProfileCombo("");
	loadSynthProfile();

	connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton *)), SLOT(on_buttonBox_clicked(QAbstractButton *)));
	connect(ui->reverbModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateReverbSettings()));
	connect(ui->reverbTimeSlider, SIGNAL(valueChanged(int)), SLOT(updateReverbSettings()));
	connect(ui->reverbLevelSlider, SIGNAL(valueChanged(int)), SLOT(updateReverbSettings()));

	synthRoute->connectReportHandler(SIGNAL(reverbModeChanged(int)), this, SLOT(handleReverbModeChanged(int)));
	synthRoute->connectReportHandler(SIGNAL(reverbTimeChanged(int)), this, SLOT(handleReverbTimeChanged(int)));
	synthRoute->connectReportHandler(SIGNAL(reverbLevelChanged(int)), this, SLOT(handleReverbLevelChanged(int)));
}

SynthPropertiesDialog::~SynthPropertiesDialog() {
	delete ui;
}

void SynthPropertiesDialog::showEvent(QShowEvent *) {
	loadSynthProfile();
}

void SynthPropertiesDialog::on_changeROMSetButton_clicked() {
	rsd.exec();
	ui->romSetLabel->setText(getROMSetDescription());
}

void SynthPropertiesDialog::on_midiDelayEmuComboBox_currentIndexChanged(int index) {
	synthRoute->setMIDIDelayMode((MT32Emu::MIDIDelayMode)index);
}

void SynthPropertiesDialog::on_dacEmuComboBox_currentIndexChanged(int index) {
	switch (index) {
	case 0:
		synthRoute->setDACInputMode(MT32Emu::DACInputMode_NICE);
		break;
	case 1:
		synthRoute->setDACInputMode(MT32Emu::DACInputMode_GENERATION1);
		break;
	case 2:
		synthRoute->setDACInputMode(MT32Emu::DACInputMode_GENERATION2);
		break;
	}
}

void SynthPropertiesDialog::on_maxPartialsSpinBox_valueChanged(int value) {
	synthRoute->setPartialCount(value);
}

void SynthPropertiesDialog::on_analogComboBox_currentIndexChanged(int index) {
	synthRoute->setAnalogOutputMode(MT32Emu::AnalogOutputMode(2 - index));
}

void SynthPropertiesDialog::on_rendererTypeComboBox_currentIndexChanged(int index) {
	synthRoute->setRendererType(MT32Emu::RendererType(index));
}

void SynthPropertiesDialog::on_reverbCompatibilityComboBox_currentIndexChanged(int index) {
	synthRoute->setReverbCompatibilityMode((ReverbCompatibilityMode)index);
}

void SynthPropertiesDialog::on_buttonBox_clicked(QAbstractButton *button) {
	switch (ui->buttonBox->standardButton(button)) {
		case QDialogButtonBox::Close:
			QDialog::accept();
			break;
		case QDialogButtonBox::Reset:
			resetSynth();
			break;
		case QDialogButtonBox::RestoreDefaults:
			restoreDefaults();
			break;
		case QDialogButtonBox::Save:
			saveSynthProfile();
			break;
		default:
			break;
	}
}

void SynthPropertiesDialog::on_profileComboBox_currentIndexChanged(int) {
	Master &master = *Master::getInstance();
	QString name = ui->profileComboBox->currentText();
	master.loadSynthProfile(synthProfile, name);
	synthRoute->setSynthProfile(synthProfile, name);
	ui->profileCheckBox->setChecked(name == master.getDefaultSynthProfileName());
	loadSynthProfile(false);
}

void SynthPropertiesDialog::on_reverbCheckBox_stateChanged(int state) {
	switch (state) {
		case Qt::Unchecked:
			ui->reverbModeComboBox->setEnabled(false);
			ui->reverbTimeSlider->setEnabled(false);
			ui->reverbLevelSlider->setEnabled(false);
			synthRoute->setReverbEnabled(false);
			synthRoute->setReverbOverridden(true);
			break;
		case Qt::Checked:
			ui->reverbModeComboBox->setEnabled(true);
			ui->reverbTimeSlider->setEnabled(true);
			ui->reverbLevelSlider->setEnabled(true);
			synthRoute->setReverbEnabled(true);
			synthRoute->setReverbOverridden(true);
			break;
		case Qt::PartiallyChecked:
			ui->reverbModeComboBox->setEnabled(false);
			ui->reverbTimeSlider->setEnabled(false);
			ui->reverbLevelSlider->setEnabled(false);
			synthRoute->setReverbEnabled(true);
			synthRoute->setReverbOverridden(false);
			break;
	}
}

void SynthPropertiesDialog::on_outputGainSlider_valueChanged(int value) {
	double gain = value / 100.0;
	ui->outputGainSpinBox->setValue(gain);
	synthRoute->setOutputGain(float(gain));
}

void SynthPropertiesDialog::on_outputGainSpinBox_editingFinished() {
	double value = ui->outputGainSpinBox->value();
	ui->outputGainSlider->setValue(int(value * 100 + 0.5));
	synthRoute->setOutputGain(float(value));
}

void SynthPropertiesDialog::on_reverbOutputGainSlider_valueChanged(int value) {
	double gain = value / 100.0;
	ui->reverbOutputGainSpinBox->setValue(gain);
	synthRoute->setReverbOutputGain(float(gain));
}

void SynthPropertiesDialog::on_reverbOutputGainSpinBox_editingFinished() {
	double value = ui->reverbOutputGainSpinBox->value();
	ui->reverbOutputGainSlider->setValue(int(value * 100 + 0.5));
	synthRoute->setReverbOutputGain(float(value));
}
void SynthPropertiesDialog::on_reverseStereoCheckBox_stateChanged(int state) {
	synthRoute->setReversedStereoEnabled(state == Qt::Checked);
}

void SynthPropertiesDialog::on_assignChannels2_10Button_clicked() {
	synthRoute->resetMIDIChannelsAssignment(false);
}

void SynthPropertiesDialog::on_assignChannels1_8_10Button_clicked() {
	synthRoute->resetMIDIChannelsAssignment(true);
}

void SynthPropertiesDialog::on_niceAmpRampCheckBox_stateChanged(int state) {
	synthRoute->setNiceAmpRampEnabled(state == Qt::Checked);
}

void SynthPropertiesDialog::on_engageChannel1CheckBox_stateChanged(int state) {
	synthRoute->setInitialMIDIChannelsAssignment(state == Qt::Checked);
}

void SynthPropertiesDialog::on_nicePanningCheckBox_stateChanged(int state) {
	synthRoute->setNicePanningEnabled(state == Qt::Checked);
}

void SynthPropertiesDialog::on_nicePartialMixingCheckBox_stateChanged(int state) {
	synthRoute->setNicePartialMixingEnabled(state == Qt::Checked);
}

void SynthPropertiesDialog::on_displayCompatibilityComboBox_currentIndexChanged(int index) {
	synthRoute->setDisplayCompatibilityMode(DisplayCompatibilityMode(index));
}

void SynthPropertiesDialog::updateReverbSettings() {
	if (ui->reverbCheckBox->checkState() == Qt::PartiallyChecked) return;
	synthRoute->setReverbSettings(ui->reverbModeComboBox->currentIndex(),
		ui->reverbTimeSlider->value(),
		ui->reverbLevelSlider->value());
}

void SynthPropertiesDialog::handleReverbModeChanged(int mode) {
	ui->reverbModeComboBox->setCurrentIndex(mode);
}

void SynthPropertiesDialog::handleReverbTimeChanged(int time) {
	ui->reverbTimeSlider->setValue(time);
}

void SynthPropertiesDialog::handleReverbLevelChanged(int level) {
	ui->reverbLevelSlider->setValue(level);
}

void SynthPropertiesDialog::resetSynth() {
	int reverbMode = ui->reverbModeComboBox->currentIndex();
	int reverbTime = ui->reverbTimeSlider->value();
	int reverbLevel = ui->reverbLevelSlider->value();
	synthRoute->reset();
	on_midiDelayEmuComboBox_currentIndexChanged(ui->midiDelayEmuComboBox->currentIndex());
	on_dacEmuComboBox_currentIndexChanged(ui->dacEmuComboBox->currentIndex());
	on_reverbCompatibilityComboBox_currentIndexChanged(ui->reverbCompatibilityComboBox->currentIndex());
	Qt::CheckState reverbState = ui->reverbCheckBox->checkState();
	if (reverbState != Qt::PartiallyChecked) {
		ui->reverbModeComboBox->setCurrentIndex(reverbMode);
		ui->reverbTimeSlider->setValue(reverbTime);
		ui->reverbLevelSlider->setValue(reverbLevel);
	}
	on_reverbCheckBox_stateChanged(reverbState);
	on_outputGainSlider_valueChanged(ui->outputGainSlider->value());
	on_reverbOutputGainSlider_valueChanged(ui->reverbOutputGainSlider->value());
	on_reverseStereoCheckBox_stateChanged(ui->reverseStereoCheckBox->checkState());
}

void SynthPropertiesDialog::restoreDefaults() {
	ui->midiDelayEmuComboBox->setCurrentIndex(1);
	ui->dacEmuComboBox->setCurrentIndex(0);
	ui->maxPartialsSpinBox->setValue(MT32Emu::DEFAULT_MAX_PARTIALS);
	ui->analogComboBox->setCurrentIndex(0);
	ui->rendererTypeComboBox->setCurrentIndex(0);
	ui->reverbCheckBox->setCheckState(Qt::Checked);
	ui->reverbCompatibilityComboBox->setCurrentIndex(0);
	ui->reverbModeComboBox->setCurrentIndex(0);
	ui->reverbTimeSlider->setValue(5);
	ui->reverbLevelSlider->setValue(3);
	ui->reverbCheckBox->setCheckState(Qt::PartiallyChecked);
	ui->outputGainSlider->setValue(100);
	ui->reverbOutputGainSlider->setValue(100);
	ui->reverseStereoCheckBox->setCheckState(Qt::Unchecked);
	ui->niceAmpRampCheckBox->setCheckState(Qt::Checked);
	ui->nicePanningCheckBox->setCheckState(Qt::Unchecked);
	ui->nicePartialMixingCheckBox->setCheckState(Qt::Unchecked);
	ui->engageChannel1CheckBox->setCheckState(Qt::Unchecked);
	ui->displayCompatibilityComboBox->setCurrentIndex(0);
}

void SynthPropertiesDialog::loadSynthProfile(bool reloadFromSynthRoute) {
	if (reloadFromSynthRoute) synthRoute->getSynthProfile(synthProfile);
	ui->romSetLabel->setText(getROMSetDescription());
	rsd.loadROMInfos();
	ui->midiDelayEmuComboBox->setCurrentIndex(synthProfile.midiDelayMode);
	ui->dacEmuComboBox->setCurrentIndex(synthProfile.emuDACInputMode == MT32Emu::DACInputMode_NICE ? MT32Emu::DACInputMode_NICE : synthProfile.emuDACInputMode - 1);
	ui->maxPartialsSpinBox->setValue(synthProfile.partialCount);
	ui->analogComboBox->setCurrentIndex(2 - synthProfile.analogOutputMode);
	ui->rendererTypeComboBox->setCurrentIndex(synthProfile.rendererType);
	ui->reverbCompatibilityComboBox->setCurrentIndex(synthProfile.reverbCompatibilityMode);
	ui->reverbCheckBox->setCheckState(Qt::Checked);
	ui->reverbModeComboBox->setCurrentIndex(synthProfile.reverbMode);
	ui->reverbTimeSlider->setValue(synthProfile.reverbTime);
	ui->reverbLevelSlider->setValue(synthProfile.reverbLevel);
	if (synthProfile.reverbOverridden) {
		ui->reverbCheckBox->setCheckState(synthProfile.reverbEnabled ? Qt::Checked : Qt::Unchecked);
	} else {
		ui->reverbCheckBox->setCheckState(Qt::PartiallyChecked);
	}
	ui->outputGainSlider->setValue(synthProfile.outputGain * 100);
	ui->reverbOutputGainSlider->setValue(synthProfile.reverbOutputGain * 100);
	ui->reverseStereoCheckBox->setCheckState(synthProfile.reversedStereoEnabled ? Qt::Checked : Qt::Unchecked);
	ui->niceAmpRampCheckBox->setCheckState(synthProfile.niceAmpRamp ? Qt::Checked : Qt::Unchecked);
	ui->nicePanningCheckBox->setCheckState(synthProfile.nicePanning ? Qt::Checked : Qt::Unchecked);
	ui->nicePartialMixingCheckBox->setCheckState(synthProfile.nicePartialMixing ? Qt::Checked : Qt::Unchecked);
	ui->engageChannel1CheckBox->setCheckState(synthProfile.engageChannel1OnOpen ? Qt::Checked : Qt::Unchecked);
	ui->displayCompatibilityComboBox->setCurrentIndex(synthProfile.displayCompatibilityMode);
}

void SynthPropertiesDialog::saveSynthProfile() {
	SynthProfile newSynthProfile;
	synthRoute->getSynthProfile(newSynthProfile);
	newSynthProfile.romDir = synthProfile.romDir;
	newSynthProfile.controlROMFileName = synthProfile.controlROMFileName;
	newSynthProfile.controlROMFileName2 = synthProfile.controlROMFileName2;
	newSynthProfile.pcmROMFileName = synthProfile.pcmROMFileName;
	newSynthProfile.pcmROMFileName2 = synthProfile.pcmROMFileName2;
	Master &master = *Master::getInstance();
	QString name = ui->profileComboBox->currentText();
	master.storeSynthProfile(newSynthProfile, name);
	master.loadSynthProfile(synthProfile, name);
	synthRoute->setSynthProfile(synthProfile, name);
	if (ui->profileCheckBox->isChecked()) master.setDefaultSynthProfileName(name);
	ui->romSetLabel->setText(getROMSetDescription());
	refreshProfileCombo(name);
}

void SynthPropertiesDialog::refreshProfileCombo(QString name) {
	QStringList profiles = Master::getInstance()->enumSynthProfiles();
	ui->profileComboBox->blockSignals(true);
	ui->profileComboBox->clear();
	ui->profileComboBox->addItems(profiles);
	if (name.isEmpty()) name = Master::getInstance()->getDefaultSynthProfileName();
	for (int i = 0; i < profiles.count(); i++) {
		if (profiles[i] == name) {
			ui->profileComboBox->setCurrentIndex(i);
			ui->profileCheckBox->setChecked(profiles[i] == Master::getInstance()->getDefaultSynthProfileName());
			break;
		}
	}
	ui->profileComboBox->blockSignals(false);
}

QString SynthPropertiesDialog::getROMSetDescription() {
	MT32Emu::FileStream file;
	if (file.open(Master::getROMPathNameLocal(synthProfile.romDir, synthProfile.controlROMFileName))) {
		const MT32Emu::ROMInfo *romInfo = MT32Emu::ROMInfo::getROMInfo(&file);
		if (romInfo != NULL) {
			QString des = romInfo->description;
			MT32Emu::ROMInfo::freeROMInfo(romInfo);
			return des;
		}
	}
	return "Unknown";
}
