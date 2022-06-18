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

#include <QFileDialog>

#include "audiodrv/AudioDriver.h"
#include "SynthWidget.h"
#include "ui_SynthWidget.h"
#include "Master.h"
#include "MidiSession.h"
#include "SynthStateMonitor.h"

static void updateMidiAddActionEnabled(Ui::SynthWidget *ui) {
	ui->midiAdd->setEnabled(ui->pinCheckBox->isEnabled() && Master::getInstance()->canCreateMidiPort());
}

static bool isAudioRecordingSupported(Ui::SynthWidget *ui) {
	const AudioDevice *currentAudioDevice = ui->audioDeviceComboBox->itemData(ui->audioDeviceComboBox->currentIndex()).value<const AudioDevice *>();
	return currentAudioDevice != NULL && currentAudioDevice->driver.id != "jackaudio";
}

SynthWidget::SynthWidget(Master *master, SynthRoute *useSynthRoute, bool pinnable, const AudioDevice *useAudioDevice, QWidget *parent) :
	QWidget(parent),
	synthRoute(useSynthRoute),
	ui(new Ui::SynthWidget),
	spd(parent, useSynthRoute),
	apd(parent),
	mpd(parent)
{
	ui->setupUi(this);

	setEmuModeText();

	synthStateMonitor = new SynthStateMonitor(ui, synthRoute);

	bool detailsVisible = master->getSettings()->value("Master/showSynthDetails", "0").toBool();
	ui->detailsFrame->setVisible(detailsVisible);
	ui->detailsButton->setIcon(SynthWidget::getSynthDetailsIcon(detailsVisible));

	refreshAudioDeviceList(master, useAudioDevice);
	ui->pinCheckBox->setChecked(master->isPinned(synthRoute));
	ui->pinCheckBox->setEnabled(pinnable);

	master->configureMidiPropertiesDialog(mpd);
	updateMidiAddActionEnabled(ui);

	connect(synthRoute, SIGNAL(stateChanged(SynthRouteState)), SLOT(handleSynthRouteState(SynthRouteState)));
	connect(synthRoute, SIGNAL(midiSessionAdded(MidiSession *)), SLOT(handleMIDISessionAdded(MidiSession *)));
	connect(synthRoute, SIGNAL(midiSessionRemoved(MidiSession *)), SLOT(handleMIDISessionRemoved(MidiSession *)));
	connect(synthRoute, SIGNAL(midiSessionNameChanged(MidiSession *)), SLOT(handleMIDISessionNameChanged(MidiSession *)));
	connect(master, SIGNAL(synthRoutePinned()), SLOT(handleSynthRoutePinned()));
	connect(ui->synthPropertiesButton, SIGNAL(clicked()), &spd, SLOT(exec()));

	synthRoute->connectReportHandler(SIGNAL(masterVolumeChanged(int)), this, SLOT(handleMasterVolumeChanged(int)));

	handleSynthRouteState(synthRoute->getState());
}

SynthWidget::~SynthWidget() {
	delete synthStateMonitor;
	delete ui;
}

SynthRoute *SynthWidget::getSynthRoute() {
	return synthRoute;
}

void SynthWidget::refreshAudioDeviceList(Master *master, const AudioDevice *useAudioDevice) {
	disconnect(ui->audioDeviceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audioDeviceComboBox_currentIndexChanged(int)));
	ui->audioDeviceComboBox->clear();
	int audioDeviceIndex = 0;
	QListIterator<const AudioDevice *> audioDeviceIt(master->getAudioDevices());
	while(audioDeviceIt.hasNext()) {
		const AudioDevice *audioDevice = audioDeviceIt.next();
		ui->audioDeviceComboBox->addItem(audioDevice->driver.name + ": " + audioDevice->name, QVariant::fromValue(audioDevice));
		if (useAudioDevice != NULL && useAudioDevice->driver.id == audioDevice->driver.id
				&& useAudioDevice->name == audioDevice->name) ui->audioDeviceComboBox->setCurrentIndex(audioDeviceIndex);
		audioDeviceIndex++;
	}
	ui->audioPropertiesButton->setEnabled(ui->audioDeviceComboBox->count() != 0);
	connect(ui->audioDeviceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(on_audioDeviceComboBox_currentIndexChanged(int)));
}

void SynthWidget::on_refreshButton_clicked() {
	const AudioDevice *currentDevice = ui->audioDeviceComboBox->itemData(ui->audioDeviceComboBox->currentIndex()).value<const AudioDevice *>();
	refreshAudioDeviceList(Master::getInstance(), currentDevice);
}

void SynthWidget::on_audioDeviceComboBox_currentIndexChanged(int audioDeviceIndex) {
	const AudioDevice *currentAudioDevice = ui->audioDeviceComboBox->itemData(audioDeviceIndex).value<const AudioDevice *>();
	if (currentAudioDevice != NULL) {
		synthRoute->setAudioDevice(currentAudioDevice);
		Master::getInstance()->setDefaultAudioDevice(currentAudioDevice->driver.id, currentAudioDevice->name);
	}
}

void SynthWidget::handleSynthRouteState(SynthRouteState SynthRouteState) {
	switch(SynthRouteState) {
	case SynthRouteState_OPEN:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(true);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->refreshButton->setEnabled(false);
		ui->audioPropertiesButton->setEnabled(false);
		ui->audioRecord->setEnabled(isAudioRecordingSupported(ui));
		ui->statusLabel->setText("Open");
		break;
	case SynthRouteState_OPENING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->refreshButton->setEnabled(false);
		ui->audioPropertiesButton->setEnabled(false);
		ui->audioRecord->setEnabled(false);
		ui->midiRecord->setDisabled(synthRoute->isExclusiveMidiModeEnabled());
		ui->statusLabel->setText("Opening");
		break;
	case SynthRouteState_CLOSING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->refreshButton->setEnabled(false);
		ui->audioPropertiesButton->setEnabled(false);
		ui->audioRecord->setEnabled(false);
		if (synthRoute->isRecordingAudio()) on_audioRecord_clicked();
		ui->statusLabel->setText("Closing");
		break;
	case SynthRouteState_CLOSED:
		ui->startButton->setEnabled(true);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(true);
		ui->refreshButton->setEnabled(true);
		ui->audioPropertiesButton->setEnabled(true);
		ui->audioRecord->setEnabled(false);
		ui->statusLabel->setText("Closed");
		break;
	}
	setEmuModeText();
}

void SynthWidget::on_audioPropertiesButton_clicked()
{
	const AudioDevice *device = ui->audioDeviceComboBox->itemData(ui->audioDeviceComboBox->currentIndex()).value<const AudioDevice *>();
	const AudioDriverSettings &driverSettings = device->driver.getAudioSettings();
	apd.setCheckText((device->driver.id == "waveout") ? "Use ring buffer renderer" : "Use advanced timing");
	apd.setData(driverSettings);
	if (QDialog::Accepted == apd.exec()) {
		AudioDriverSettings newDriverSettings;
		apd.getData(newDriverSettings);
		device->driver.setAudioSettings(newDriverSettings);
	}
}

void SynthWidget::on_startButton_clicked()
{
	if (!synthRoute->open()) {
		ui->statusLabel->setText("Open failed :(");
	}
}

void SynthWidget::on_stopButton_clicked()
{
	synthRoute->close();
	ui->statusLabel->setText("Closed");
}

void SynthWidget::handleSynthRoutePinned() {
	ui->pinCheckBox->setChecked(Master::getInstance()->isPinned(synthRoute));
}

void SynthWidget::on_pinCheckBox_stateChanged(int state)
{
	Master *master = Master::getInstance();
	if (state == Qt::Checked) {
		master->setPinned(synthRoute);
	} else if (state == Qt::Unchecked && master->isPinned(synthRoute)) {
		master->setPinned(NULL);
	}
}

int SynthWidget::findMIDISession(MidiSession *midiSession) {
	for (int i = 0; i < ui->midiList->count(); i++) {
		QListWidgetItem *item = ui->midiList->item(i);
		if (item->data(Qt::UserRole) == QVariant::fromValue((QObject *)midiSession)) return i;
	}
	return -1;
}

void SynthWidget::handleMIDISessionAdded(MidiSession *midiSession) {
	QListWidgetItem *item = new QListWidgetItem(midiSession->getName(), ui->midiList);
	item->setData(Qt::UserRole, QVariant::fromValue((QObject *)midiSession));
	ui->midiList->addItem(item);
	updateMidiAddActionEnabled(ui);
}

void SynthWidget::handleMIDISessionRemoved(MidiSession *midiSession) {
	delete ui->midiList->takeItem(findMIDISession(midiSession));
	updateMidiAddActionEnabled(ui);
}

void SynthWidget::handleMIDISessionNameChanged(MidiSession *midiSession) {
	QListWidgetItem *item = ui->midiList->item(findMIDISession(midiSession));
	item->setText(midiSession->getName());
}

MidiSession *SynthWidget::getSelectedMIDISession() {
	QListWidgetItem *item = ui->midiList->currentItem();
	return (item == NULL) ? NULL : (MidiSession *)item->data(Qt::UserRole).value<QObject *>();
}

void SynthWidget::on_midiList_itemSelectionChanged() {
	Master *master = Master::getInstance();
	MidiSession *midiSession = getSelectedMIDISession();
	if (midiSession != NULL) {
		ui->midiRemove->setEnabled(master->canDeleteMidiPort(midiSession));
		ui->midiProperties->setEnabled(master->canReconnectMidiPort(midiSession));
	} else {
		ui->midiRemove->setEnabled(false);
		ui->midiProperties->setEnabled(false);
	}
}

void SynthWidget::on_midiAdd_clicked() {
	Master::getInstance()->createMidiPort(mpd, synthRoute);
}

void SynthWidget::on_midiRemove_clicked() {
	Master::getInstance()->deleteMidiPort(getSelectedMIDISession());
}

void SynthWidget::on_midiProperties_clicked() {
	Master::getInstance()->reconnectMidiPort(mpd, getSelectedMIDISession());
}

void SynthWidget::on_midiRecord_clicked() {
	if (synthRoute->isRecordingMidi()) {
		bool isMidiDataRecorded = synthRoute->stopRecordingMidi();
		ui->midiRecord->setText("Record");
		if (isMidiDataRecorded) {
			static QString currentDir = NULL;
			QFileDialog::Options qFileDialogOptions = QFileDialog::Options(Master::getInstance()->getSettings()->value("Master/qFileDialogOptions", 0).toInt());
			QString fileName = QFileDialog::getSaveFileName(this, NULL, currentDir, "Standard MIDI files (*.mid)",
				NULL, qFileDialogOptions);
			if (!fileName.isEmpty()) currentDir = QDir(fileName).absolutePath();
			synthRoute->saveRecordedMidi(fileName, MasterClock::NANOS_PER_MILLISECOND);
		}
	} else {
		ui->midiRecord->setText("Stop");
		synthRoute->startRecordingMidi();
	}
}

void SynthWidget::on_audioRecord_clicked() {
	if (synthRoute->isRecordingAudio()) {
		ui->audioRecord->setText("Record");
		synthRoute->stopRecordingAudio();
	} else {
		static QString currentDir = NULL;
		QFileDialog::Options qFileDialogOptions = QFileDialog::Options(Master::getInstance()->getSettings()->value("Master/qFileDialogOptions", 0).toInt());
		QString fileName = QFileDialog::getSaveFileName(this, NULL, currentDir, "*.wav *.raw;;*.wav;;*.raw;;*.*",
			NULL, qFileDialogOptions);
		if (!fileName.isEmpty()) {
			currentDir = QDir(fileName).absolutePath();
			ui->audioRecord->setText("Stop");
			synthRoute->startRecordingAudio(fileName);
		}
	}
}

void SynthWidget::on_masterVolumeSlider_valueChanged(int newValue) {
	synthRoute->setMasterVolume(newValue);
}

void SynthWidget::handleMasterVolumeChanged(int volume) {
	ui->masterVolumeSlider->setValue(volume);
}

void SynthWidget::hideEvent(QHideEvent *) {
	synthStateMonitor->enableMonitor(false);
}

void SynthWidget::showEvent(QShowEvent *) {
	if (synthRoute->getState() == SynthRouteState_OPEN) {
		synthStateMonitor->enableMonitor(true);
	}
}

void SynthWidget::on_detailsButton_clicked() {
	bool newVisible = !ui->detailsFrame->isVisible();
	ui->detailsFrame->setVisible(newVisible);
	ui->detailsButton->setIcon(SynthWidget::getSynthDetailsIcon(newVisible));
	Master::getInstance()->getSettings()->setValue("Master/showSynthDetails", newVisible);

	// Adjust size of main window
	int heightDelta = ui->detailsFrame->height() + layout()->spacing();
	if (!newVisible) {
		heightDelta = -heightDelta;

		// Enforce relayout of widget hierarchy up to the main window to allow its shrinking
		QWidget *p = this;
		while (p != NULL) {
			p->updateGeometry();
			p = p->parentWidget();
		}
		layout()->activate();
		window()->layout()->activate();
	}

	// OK, widgets' layouts updated, now resize main window
	QSize newSize = window()->size();
	newSize.rheight() += heightDelta;
	window()->resize(newSize);
}

void SynthWidget::setEmuModeText() {
	QString emuMode;
	const MT32Emu::ROMImage *controlROMImage = NULL;
	const MT32Emu::ROMImage *pcmROMImage = NULL;
	synthRoute->getROMImages(controlROMImage, pcmROMImage);
	emuMode = controlROMImage == NULL ? "Unknown" : controlROMImage->getROMInfo()->description;
	ui->synthEmuModeLabel->setText("Emulation Mode: " + emuMode);
}

const QIcon &SynthWidget::getSynthDetailsIcon(bool visible) {
	static const QIcon SYNTH_DETAILS_VISIBLE(":/images/DetailsHide.gif");
	static const QIcon SYNTH_DETAILS_HIDDEN(":/images/Details.gif");

	return visible ? SYNTH_DETAILS_VISIBLE : SYNTH_DETAILS_HIDDEN;
}
