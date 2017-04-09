/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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

#include "AudioPropertiesDialog.h"
#include "ui_AudioPropertiesDialog.h"

AudioPropertiesDialog::AudioPropertiesDialog(QWidget *parent) :
		QDialog(parent),
		ui(new Ui::AudioPropertiesDialog)
{
	ui->setupUi(this);
}

AudioPropertiesDialog::~AudioPropertiesDialog()
{
	delete ui;
}

void AudioPropertiesDialog::getData(AudioDriverSettings &driverSettings) {
	driverSettings.sampleRate = ui->sampleRate->currentText().toUInt();
	driverSettings.srcQuality = MT32Emu::SamplerateConversionQuality(ui->srcQuality->currentIndex());
	driverSettings.chunkLen = ui->chunkLen->text().toInt();
	driverSettings.audioLatency = ui->audioLatency->text().toInt();
	driverSettings.midiLatency = ui->midiLatency->text().toInt();
	driverSettings.advancedTiming = ui->advancedTiming->isChecked();
}

void AudioPropertiesDialog::setData(const AudioDriverSettings &driverSettings) {
	int ix = (driverSettings.sampleRate <= 0) ? 0 : ui->sampleRate->findText(QString().setNum(driverSettings.sampleRate));
	if (ix < 0) {
		ui->sampleRate->addItem(QString().setNum(driverSettings.sampleRate));
		ui->sampleRate->setCurrentIndex(ui->sampleRate->count() - 1);
	} else {
		ui->sampleRate->setCurrentIndex(ix);
	}
	ui->srcQuality->setCurrentIndex(driverSettings.srcQuality);
	ui->chunkLen->setText(QString().setNum(driverSettings.chunkLen));
	ui->audioLatency->setText(QString().setNum(driverSettings.audioLatency));
	ui->midiLatency->setText(QString().setNum(driverSettings.midiLatency));
	ui->advancedTiming->setChecked(driverSettings.advancedTiming);
}

void AudioPropertiesDialog::setCheckText(QString text) {
	ui->advancedTiming->setText(text);
}
