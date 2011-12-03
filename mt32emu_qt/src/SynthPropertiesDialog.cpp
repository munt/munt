/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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
	rsd(this)
{
	ui->setupUi(this);
	restoreDefaults();
	connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton *)), SLOT(on_buttonBox_clicked(QAbstractButton *)));
	connect(ui->reverbModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateReverbSettings()));
	connect(ui->reverbTimeSlider, SIGNAL(valueChanged(int)), SLOT(updateReverbSettings()));
	connect(ui->reverbLevelSlider, SIGNAL(valueChanged(int)), SLOT(updateReverbSettings()));

	QReportHandler *handler = synthRoute->findChild<QSynth *>()->findChild<QReportHandler *>();
	connect(handler, SIGNAL(reverbModeChanged(int)), SLOT(handleReverbModeChanged(int)));
	connect(handler, SIGNAL(reverbTimeChanged(int)), SLOT(handleReverbTimeChanged(int)));
	connect(handler, SIGNAL(reverbLevelChanged(int)), SLOT(handleReverbLevelChanged(int)));
}

SynthPropertiesDialog::~SynthPropertiesDialog()
{
	delete ui;
}

void SynthPropertiesDialog::on_romDirButton_clicked()
{
	QString s = QFileDialog::getExistingDirectory(this, "Choose ROM directory", ui->romDirLineEdit->text());
	if (s.isNull()) return;
	rsd.loadROMInfos(s);
	if (QDialog::Accepted == rsd.exec()) {
		ui->romDirLineEdit->setText(s);
		resetSynth();
	}
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

void SynthPropertiesDialog::on_buttonBox_clicked(QAbstractButton *button) {
	switch (ui->buttonBox->standardButton(button)) {
		case QDialogButtonBox::Ok:
			QDialog::accept();
			break;
		case QDialogButtonBox::Reset:
			resetSynth();
			break;
		case QDialogButtonBox::RestoreDefaults:
			restoreDefaults();
			break;
		default:
			break;
	}
}

void SynthPropertiesDialog::on_reverbCheckBox_stateChanged(int state) {
	switch (state) {
		case Qt::Unchecked:
			ui->reverbModeComboBox->setEnabled(true);
			ui->reverbTimeSlider->setEnabled(true);
			ui->reverbLevelSlider->setEnabled(true);
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
	synthRoute->setOutputGain((float)value / 100.0f);
}

void SynthPropertiesDialog::on_reverbOutputGainSlider_valueChanged(int value) {
	synthRoute->setReverbOutputGain((float)value / 100.0f);
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
	on_dacEmuComboBox_currentIndexChanged(ui->dacEmuComboBox->currentIndex());
	Qt::CheckState reverbState = ui->reverbCheckBox->checkState();
	on_reverbCheckBox_stateChanged(reverbState);
	if (reverbState != Qt::PartiallyChecked) {
		ui->reverbModeComboBox->setCurrentIndex(reverbMode);
		ui->reverbTimeSlider->setValue(reverbTime);
		ui->reverbLevelSlider->setValue(reverbLevel);
	}
	on_outputGainSlider_valueChanged(ui->outputGainSlider->value());
	on_reverbOutputGainSlider_valueChanged(ui->reverbOutputGainSlider->value());
}

void SynthPropertiesDialog::restoreDefaults() {
	QString s = Master::getInstance()->getROMDir().absolutePath();
	ui->romDirLineEdit->setText(s);
	ui->dacEmuComboBox->setCurrentIndex(0);
	ui->reverbCheckBox->setCheckState(Qt::Checked);
	ui->reverbModeComboBox->setCurrentIndex(0);
	ui->reverbTimeSlider->setValue(5);
	ui->reverbLevelSlider->setValue(3);
	ui->reverbCheckBox->setCheckState(Qt::PartiallyChecked);
	ui->outputGainSlider->setValue(100);
	ui->reverbOutputGainSlider->setValue(100);
}
