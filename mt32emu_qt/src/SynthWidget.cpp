/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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

#include "audiodrv/AudioDriver.h"
#include "SynthWidget.h"
#include "ui_SynthWidget.h"
#include "Master.h"
#include "MidiSession.h"
#include "font_6x8.h"
#include "SynthStateMonitor.h"

SynthWidget::SynthWidget(Master *master, SynthRoute *useSynthRoute, const AudioDevice *useAudioDevice, QWidget *parent) :
	QWidget(parent),
	synthRoute(useSynthRoute),
	ui(new Ui::SynthWidget),
	spd(parent, useSynthRoute),
	apd(parent),
	mpd(parent),
	adjustSize(false)
{
	ui->setupUi(this);

	setEmuModeText();

	lcdWidget = new LCDWidget(synthRoute, ui->frame);
	lcdWidget->setObjectName("lcdWidget");
	ui->lcdLayout->addWidget(lcdWidget);

	ui->detailsFrame->setVisible(false);
	synthStateMonitor = new SynthStateMonitor(ui, synthRoute);
	if (master->getSettings()->value("Master/showSynthDetails", "0").toBool()) on_detailsButton_clicked();

	refreshAudioDeviceList(master, useAudioDevice);
	ui->pinCheckBox->setChecked(master->isPinned(synthRoute));

	ui->midiAdd->setEnabled(master->canCreateMidiPort());

	connect(synthRoute, SIGNAL(stateChanged(SynthRouteState)), SLOT(handleSynthRouteState(SynthRouteState)));
	connect(synthRoute, SIGNAL(midiSessionAdded(MidiSession *)), SLOT(handleMIDISessionAdded(MidiSession *)));
	connect(synthRoute, SIGNAL(midiSessionRemoved(MidiSession *)), SLOT(handleMIDISessionRemoved(MidiSession *)));
	connect(synthRoute, SIGNAL(midiSessionNameChanged(MidiSession *)), SLOT(handleMIDISessionNameChanged(MidiSession *)));
	connect(master, SIGNAL(synthRoutePinned()), SLOT(handleSynthRoutePinned()));
	connect(ui->synthPropertiesButton, SIGNAL(clicked()), &spd, SLOT(exec()));

	QReportHandler *handler = synthRoute->findChild<QSynth *>()->findChild<QReportHandler *>();
	connect(handler, SIGNAL(masterVolumeChanged(int)), SLOT(handleMasterVolumeChanged(int)));

	handleSynthRouteState(synthRoute->getState());
}

SynthWidget::~SynthWidget() {
	delete synthStateMonitor;
	delete lcdWidget;
	delete ui;
}

SynthRoute *SynthWidget::getSynthRoute() {
	return synthRoute;
}

void SynthWidget::refreshAudioDeviceList(Master *master, const AudioDevice *useAudioDevice) {
	disconnect(ui->audioDeviceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audioDeviceComboBox_currentIndexChanged(int)));
	ui->audioDeviceComboBox->clear();
	int audioDeviceIndex = 0;
	QListIterator<AudioDevice *> audioDeviceIt(master->getAudioDevices());
	while(audioDeviceIt.hasNext()) {
		AudioDevice *audioDevice = audioDeviceIt.next();
		ui->audioDeviceComboBox->addItem(audioDevice->driver->name + ": " + audioDevice->name, qVariantFromValue(audioDevice));
		if (useAudioDevice != NULL && useAudioDevice->driver->id == audioDevice->driver->id
				&& useAudioDevice->name == audioDevice->name) ui->audioDeviceComboBox->setCurrentIndex(audioDeviceIndex);
		audioDeviceIndex++;
	}
	ui->audioPropertiesButton->setEnabled(ui->audioDeviceComboBox->count() != 0);
	connect(ui->audioDeviceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(on_audioDeviceComboBox_currentIndexChanged(int)));
}

void SynthWidget::on_refreshButton_clicked() {
	AudioDevice *currentDevice = qVariantValue<AudioDevice *> (ui->audioDeviceComboBox->itemData(ui->audioDeviceComboBox->currentIndex()));
	refreshAudioDeviceList(Master::getInstance(), currentDevice);
}

void SynthWidget::on_audioDeviceComboBox_currentIndexChanged(int audioDeviceIndex) {
	AudioDevice *currentAudioDevice = ui->audioDeviceComboBox->itemData(audioDeviceIndex).value<AudioDevice *>();
	if (currentAudioDevice != NULL) {
		synthRoute->setAudioDevice(currentAudioDevice);
		Master::getInstance()->setDefaultAudioDevice(currentAudioDevice->driver->id, currentAudioDevice->name);
	}
}

void SynthWidget::handleSynthRouteState(SynthRouteState SynthRouteState) {
	switch(SynthRouteState) {
	case SynthRouteState_OPEN:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(true);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Open");
		break;
	case SynthRouteState_OPENING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Opening");
		break;
	case SynthRouteState_CLOSING:
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(false);
		ui->statusLabel->setText("Closing");
		break;
	case SynthRouteState_CLOSED:
		ui->startButton->setEnabled(true);
		ui->stopButton->setEnabled(false);
		ui->audioDeviceComboBox->setEnabled(true);
		ui->statusLabel->setText("Closed");
		break;
	}
	setEmuModeText();
}

void SynthWidget::on_audioPropertiesButton_clicked()
{
	unsigned int chunkLen;
	unsigned int audioLatency;
	unsigned int midiLatency;
	bool advancedTiming;

	AudioDevice *device = ui->audioDeviceComboBox->itemData(ui->audioDeviceComboBox->currentIndex()).value<AudioDevice *>();
	device->driver->getAudioSettings(&chunkLen, &audioLatency, &midiLatency, &advancedTiming);
	apd.setCheckText((device->driver->id == "waveout") ? "Use ring buffer renderer" : "Use advanced timing");
	apd.setData(chunkLen, audioLatency, midiLatency, advancedTiming);
	if (QDialog::Accepted == apd.exec()) {
		apd.getData(chunkLen, audioLatency, midiLatency, advancedTiming);
		const_cast<AudioDriver *> (device->driver)->setAudioSettings(&chunkLen, &audioLatency, &midiLatency, &advancedTiming);
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
	ui->midiAdd->setEnabled(Master::getInstance()->canCreateMidiPort());
}

void SynthWidget::handleMIDISessionRemoved(MidiSession *midiSession) {
	delete ui->midiList->takeItem(findMIDISession(midiSession));
	ui->midiAdd->setEnabled(Master::getInstance()->canCreateMidiPort());
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
		ui->midiProperties->setEnabled(master->canSetMidiPortProperties(midiSession));
	} else {
		ui->midiRemove->setEnabled(false);
		ui->midiProperties->setEnabled(false);
	}
}

void SynthWidget::on_midiAdd_clicked() {
	Master::getInstance()->createMidiPort(&mpd, synthRoute);
}

void SynthWidget::on_midiRemove_clicked() {
	Master::getInstance()->deleteMidiPort(getSelectedMIDISession());
}

void SynthWidget::on_midiProperties_clicked() {
	Master::getInstance()->setMidiPortProperties(&mpd, getSelectedMIDISession());
}

void SynthWidget::on_midiRecord_clicked() {
	MidiRecorder &recorder = *synthRoute->getMidiRecorder();
	if (recorder.isRecording()) {
		ui->midiRecord->setText("Record");
		recorder.stopRecording();
		static QString currentDir = NULL;
		QString fileName = QFileDialog::getSaveFileName(NULL, NULL, currentDir, "Standard MIDI files (*.mid)");
		if (!fileName.isEmpty()) {
			currentDir = QDir(fileName).absolutePath();
			recorder.saveSMF(fileName, MasterClock::NANOS_PER_MILLISECOND);
		}
	} else {
		ui->midiRecord->setText("Stop");
		recorder.startRecording();
	}
}

void SynthWidget::on_masterVolumeSlider_valueChanged(int newValue) {
	synthRoute->setMasterVolume(newValue);
}

void SynthWidget::handleMasterVolumeChanged(int volume) {
	ui->masterVolumeSlider->setValue(volume);
}

void SynthWidget::paintEvent(QPaintEvent *paintEvent) {
	QWidget::paintEvent(paintEvent);
	if (adjustSize) {
		int oldHeight = height();
		((QWidget*)spd.parent())->adjustSize();
		int newHeight = height();
		if (newHeight != oldHeight) adjustSize = false;
	}
}

void SynthWidget::hideEvent(QHideEvent *) {
	synthStateMonitor->connectSignals(false);
	ui->detailsFrame->setVisible(false);
}

void SynthWidget::showEvent(QShowEvent *) {
	bool monitorVisible = Master::getInstance()->getSettings()->value("Master/showSynthDetails", "0").toBool();
	ui->detailsFrame->setVisible(monitorVisible);
	ui->detailsButton->setIcon(monitorVisible ? QIcon(":/images/DetailsHide.gif") : QIcon(":/images/Details.gif"));
	synthStateMonitor->connectSignals(monitorVisible);
}

void SynthWidget::on_detailsButton_clicked() {
	bool newVisible = !ui->detailsFrame->isVisible();
	ui->detailsFrame->setVisible(newVisible);
	ui->detailsButton->setIcon(newVisible ? QIcon(":/images/DetailsHide.gif") : QIcon(":/images/Details.gif"));
	synthStateMonitor->connectSignals(newVisible);
	Master::getInstance()->getSettings()->setValue("Master/showSynthDetails", newVisible);
	adjustSize = adjustSize || !newVisible;
}

void SynthWidget::setEmuModeText() {
	QString emuMode;
	SynthProfile synthProfile;
	synthRoute->getSynthProfile(synthProfile);
	if (synthProfile.controlROMImage == NULL) emuMode = "Unknown";
	else emuMode = synthProfile.controlROMImage->getROMInfo()->description;
	ui->synthEmuModeLabel->setText(emuMode + " Emulation Mode");
}

LCDWidget::LCDWidget(SynthRoute *useSynthRoute, QWidget *parent) :
	QWidget(parent),
	synthRoute(useSynthRoute),
	offBackground(":/images/LCDOff.gif"),
	onBackground(":/images/LCDOn.gif"),
	drawMaskedChars(true),
	masterVolume(100)
{
	setMinimumSize(254, 40);
	handlePartStateReset();

	QSynth *qsynth = synthRoute->findChild<QSynth *>();
	QReportHandler *handler = qsynth->findChild<QReportHandler *>();
	connect(handler, SIGNAL(lcdMessageDisplayed(const QString)), SLOT(setLCDText(const QString)));
	connect(handler, SIGNAL(masterVolumeChanged(int)), SLOT(handleMasterVolumeChanged(int)));
	connect(handler, SIGNAL(partStateChanged(int, bool)), SLOT(handlePartStateChanged(int, bool)));
	connect(qsynth, SIGNAL(partStateReset()), SLOT(handlePartStateReset()));
}

void LCDWidget::setLCDText(const QString useText)
{
	QString text;
	if (useText.isEmpty()) {
		lcdText = QString().sprintf("1 2 3 4 5 R |vol:%3d", masterVolume).toAscii();
		drawMaskedChars = true;
	} else {
		lcdText = useText.toAscii();
		drawMaskedChars = false;
	}
	update();
}

void LCDWidget::handleMasterVolumeChanged(int volume) {
	masterVolume = volume;
	setLCDText("");
}

void LCDWidget::handlePartStateChanged(int partNum, bool isActive) {
	if (partNum == 8) {
		partNum = 5; // mapping for the rhythm channel
	} else if (partNum > 4) {
		return;
	}
	maskedChar[partNum << 1] = isActive;
	setLCDText("");
}

void LCDWidget::handlePartStateReset()
{
	for (int i = 0; i < 20; i++) maskedChar[i] = false;
	setLCDText();
}

void LCDWidget::paintEvent(QPaintEvent *)
{
	QPainter lcdPainter(this);
	if (synthRoute->getState() != SynthRouteState_OPEN) {
		lcdPainter.drawPixmap(0, 0, offBackground);
		return;
	}
	lcdPainter.drawPixmap(0, 0, onBackground);
	lcdPainter.translate(7, 9);

	QColor bgColor(98, 127, 0);
	QColor fgColor(232, 254, 0);

	int xat, xstart, yat;
	xstart = 0;
	yat = 0;

	for (int i = 0; i < 20; i++) {
		unsigned char c;
		c = 0x20;
		if (i < lcdText.size()) {
			c = lcdText[i];
		}

		// Don't render characters we don't have mapped
		if (c < 0x20) c = 0x20;
		if (c > 0x7f) c = 0x20;

		c -= 0x20;

		yat = 1;
		for (int t = 0; t < 8; t++) {
			xat = xstart;
			unsigned char fval;
			if (maskedChar[i] && (t != 7) && drawMaskedChars) {
				fval = 0x1f;
			} else {
				fval = Font_6x8[c][t];
			}
			for (int m = 4; m >= 0; --m) {
				if ((fval >> m) & 1) {
					lcdPainter.fillRect(xat, yat, 2, 2, fgColor);
				} else {
					lcdPainter.fillRect(xat, yat, 2, 2, bgColor);
				}
				xat += 2;
			}
			yat += 2;
			if (t == 6) yat += 2;
		}
		xstart += 12;
	}
}
