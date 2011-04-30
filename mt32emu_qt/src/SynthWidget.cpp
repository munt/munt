/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#include "audiodrv/AudioDriver.h"
#include "SynthWidget.h"
#include "ui_SynthWidget.h"

SynthWidget::SynthWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SynthWidget)
{
    ui->setupUi(this);
	ui->audioDeviceComboBox->addItems(synthManager.audioDriver->getDeviceNames());
	connect(&synthManager, SIGNAL(stateChanged(SynthManagerState)), SLOT(handleSynthManagerState(SynthManagerState)));
	synthManager.connect(ui->audioDeviceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setAudioDeviceIndex(int)));
}

SynthWidget::~SynthWidget()
{
    delete ui;
}

void SynthWidget::handleSynthManagerState(SynthManagerState synthManagerState) {
	switch(synthManagerState) {
	case SynthManagerState_OPEN:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(true);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Open");
		break;
	case SynthManagerState_OPENING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Opening");
		break;
	case SynthManagerState_CLOSING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Closing");
		break;
	case SynthManagerState_CLOSED:
		ui->startButton->setEnabled(true);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(true);
		ui->statusLabel->setText("Closed");
		break;
	}
}

void SynthWidget::on_synthPropertiesButton_clicked()
{
	spd.exec();
}

void SynthWidget::on_startButton_clicked()
{
	if (!synthManager.open()) {
		ui->statusLabel->setText("Open failed :(");
	}
}

void SynthWidget::on_stopButton_clicked()
{
	synthManager.close();
	ui->statusLabel->setText("Closed");
}
