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

#include <QFileDialog>
#include <QDropEvent>

#include "MidiConverterDialog.h"
#include "Master.h"

MidiConverterDialog::MidiConverterDialog(Master *master, QWidget *parent) : QDialog(parent), ui(new Ui::MidiConverterDialog), batchMode(false) {
	ui->setupUi(this);
	loadProfileCombo();
	connect(&converter, SIGNAL(conversionFinished()), SLOT(handleConversionFinished()));
	connect(&converter, SIGNAL(midiEventProcessed(int, int)), SLOT(updateConversionProgress(int, int)));
	connect(this, SIGNAL(conversionFinished(const QString &, const QString &)), master, SLOT(showBalloon(const QString &, const QString &)));
	setAcceptDrops(true);
}

MidiConverterDialog::~MidiConverterDialog() {
	delete ui;
}

void MidiConverterDialog::on_addMidiButton_clicked() {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddMidiFileDir").toString();
	QStringList fileNames = QFileDialog::getOpenFileNames(this, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*");
	if (!fileNames.isEmpty()) {
		currentDir = QDir(fileNames.first()).absolutePath();
		Master::getInstance()->getSettings()->setValue("Master/LastAddMidiFileDir", currentDir);
		if (ui->pcmList->count() == 0) addPcmFile(currentDir);
		if (ui->pcmList->count() > 0) {
			ui->midiList->addItems(fileNames);
		}
	}
}

void MidiConverterDialog::addPcmFile(QString proposedPCMFileName) {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddPcmFileDir").toString();
	if (proposedPCMFileName.isEmpty()) {
		proposedPCMFileName = currentDir;
	} else {
		if (proposedPCMFileName.endsWith(".mid") || proposedPCMFileName.endsWith(".smf") || proposedPCMFileName.endsWith(".syx")) {
			proposedPCMFileName.truncate(proposedPCMFileName.length() - 4);
		}
		proposedPCMFileName += ".wav";
	}
	QString fileName = QFileDialog::getSaveFileName(this, NULL, proposedPCMFileName, "*.wav *.raw;;*.wav;;*.raw;;*.*");
	if (!fileName.isEmpty()) {
		currentDir = QDir(fileName).absolutePath();
		Master::getInstance()->getSettings()->setValue("Master/LastAddPcmFileDir", currentDir);
		ui->pcmList->addItem(fileName);
		ui->pcmList->setCurrentRow(ui->pcmList->count() - 1);
	}
}

void MidiConverterDialog::on_addPcmButton_clicked() {
	addPcmFile(QString());
}

void MidiConverterDialog::on_removeButton_clicked() {
	if (ui->pcmList->count() == 0) return;
	if (ui->midiList->count() > 0) {
		delete ui->midiList->takeItem(ui->midiList->currentRow());
	} else {
		delete ui->pcmList->takeItem(ui->pcmList->currentRow());
	}
}

void MidiConverterDialog::on_clearButton_clicked() {
	if (ui->midiList->count() > 0) {
		ui->midiList->clear();
	} else {
		ui->pcmList->clear();
	}
}

void MidiConverterDialog::on_moveUpButton_clicked() {
	int currentRow = ui->midiList->currentRow();
	if (currentRow < 1) return;
	QString currentFile = ui->midiList->currentItem()->text();
	QString prevFile = ui->midiList->item(currentRow - 1)->text();
	ui->midiList->currentItem()->setText(prevFile);
	ui->midiList->item(currentRow - 1)->setText(currentFile);
	ui->midiList->setCurrentRow(currentRow - 1);
}

void MidiConverterDialog::on_moveDownButton_clicked() {
	int currentRow = ui->midiList->currentRow();
	if ((currentRow == -1) || ((ui->midiList->count() - 2) < currentRow)) return;
	QString currentFile = ui->midiList->currentItem()->text();
	QString nextFile = ui->midiList->item(currentRow + 1)->text();
	ui->midiList->currentItem()->setText(nextFile);
	ui->midiList->item(currentRow + 1)->setText(currentFile);
	ui->midiList->setCurrentRow(currentRow + 1);
}

void MidiConverterDialog::on_startButton_clicked() {
	if (ui->pcmList->count() == 0) {
		if (batchMode) {
			((QWidget *)parent())->close();
			return;
		}
		enableControls(true);
		return;
	}
	enableControls(false);
	ui->pcmList->setCurrentRow(0);
	const QStringList midiFileNames = getMidiFileNames();
	if (!converter.convertMIDIFiles(ui->pcmList->currentItem()->text(), midiFileNames, ui->profileComboBox->currentText())) enableControls(true);
}

void MidiConverterDialog::on_stopButton_clicked() {
	converter.stop();
	enableControls(true);
}

void MidiConverterDialog::loadProfileCombo() {
	Master &master = *Master::getInstance();
	QStringList profiles = master.enumSynthProfiles();
	ui->profileComboBox->clear();
	ui->profileComboBox->addItems(profiles);
	for (int i = 0; i < profiles.count(); i++) {
		if (profiles[i] == master.getDefaultSynthProfileName()) {
			ui->profileComboBox->setCurrentIndex(i);
			break;
		}
	}
}

void MidiConverterDialog::on_pcmList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
	const QStringList midiFileNames = getMidiFileNames();
	ui->midiList->clear();
	if (previous != NULL) {
		previous->setData(Qt::UserRole, midiFileNames);
	}
	if (current == NULL) {
		ui->startButton->setEnabled(false);
		return;
	}
	ui->startButton->setEnabled(true);
	const QStringList newMidiFileNames = (QStringList)current->data(Qt::UserRole).value<QStringList>();
	ui->midiList->addItems(newMidiFileNames);
}

void MidiConverterDialog::handleConversionFinished() {
	if (Master::getInstance()->getSettings()->value("Master/showConnectionBalloons", "1").toBool()) {
		emit conversionFinished("MIDI file converted", ui->pcmList->currentItem()->text());
	}
	ui->midiList->clear();
	delete ui->pcmList->takeItem(ui->pcmList->currentRow());
	on_startButton_clicked();
}

void MidiConverterDialog::updateConversionProgress(int midiEventsProcessed, int midiEventsTotal) {
	double percentage = (100.0 * midiEventsProcessed) / midiEventsTotal;
	ui->progressBar->setValue((int)percentage);
}

void MidiConverterDialog::enableControls(bool enable) {
	if (enable == !ui->stopButton->isEnabled()) return;
	ui->progressBar->setEnabled(!enable);
	ui->progressBar->setValue(0);
	ui->stopButton->setEnabled(!enable);
	ui->startButton->setEnabled(enable && ui->pcmList->count() > 0);
	ui->profileComboBox->setEnabled(enable);
	ui->midiList->setEnabled(enable);
	ui->pcmList->setEnabled(enable);
	ui->addMidiButton->setEnabled(enable);
	ui->addPcmButton->setEnabled(enable);
	ui->removeButton->setEnabled(enable);
	ui->clearButton->setEnabled(enable);
	ui->moveUpButton->setEnabled(enable);
	ui->moveDownButton->setEnabled(enable);
}

void MidiConverterDialog::startConversion(const QStringList &fileList) {
	if (fileList.count() < 2) return;
	ui->pcmList->clear();
	ui->pcmList->addItem(fileList.at(0));
	ui->pcmList->setCurrentRow(0);
	ui->midiList->clear();
	ui->midiList->addItems(fileList.mid(1));
	on_startButton_clicked();
	batchMode = true;
}

const QStringList MidiConverterDialog::getMidiFileNames() {
	QStringList list;
	for (int i = 0; i < ui->midiList->count(); i++) {
		list.append(ui->midiList->item(i)->text());
	}
	return list;
}

void MidiConverterDialog::dragEnterEvent(QDragEnterEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MidiConverterDialog::dragMoveEvent(QDragMoveEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MidiConverterDialog::dropEvent(QDropEvent *e) {
	Master::isSupportedDropEvent(e);
	if (!e->isAccepted()) return;
	QList<QUrl> urls = e->mimeData()->urls();
	for (int i = 0; i < urls.size(); i++) {
		QUrl url = urls.at(i);
		if (url.scheme() != "file") continue;
		QString fileName = url.toLocalFile();
		if (ui->pcmList->count() == 0) {
			addPcmFile(fileName);
		}
		if (ui->pcmList->count() == 0) {
			e->ignore();
			return;
		}
		ui->midiList->addItem(fileName);
	}
}
