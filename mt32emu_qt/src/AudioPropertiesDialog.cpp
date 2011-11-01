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

void AudioPropertiesDialog::getData(unsigned int &chunkLen, unsigned int &audioLatency, unsigned int &midiLatency) {
	chunkLen = ui->chunkLen->text().toInt();
	audioLatency = ui->audioLatency->text().toInt();
	midiLatency = ui->midiLatency->text().toInt();
}

void AudioPropertiesDialog::setData(unsigned int chunkLen, unsigned int audioLatency, unsigned int midiLatency) {
	ui->chunkLen->setText(QString().setNum(chunkLen));
	ui->audioLatency->setText(QString().setNum(audioLatency));
	ui->midiLatency->setText(QString().setNum(midiLatency));
}
