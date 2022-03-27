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
#include <QDropEvent>

#include "MidiConverterDialog.h"
#include "Master.h"

static inline bool isSyxFileName(const QString &fileName) {
	return fileName.endsWith(".syx", Qt::CaseInsensitive);
}

static inline bool isSmfFileName(const QString &fileName) {
	return fileName.endsWith(".mid", Qt::CaseInsensitive) || fileName.endsWith(".smf", Qt::CaseInsensitive);
}

MidiConverterDialog::MidiConverterDialog(Master *master, QWidget *parent) : QDialog(parent), ui(new Ui::MidiConverterDialog), batchMode(false) {
	ui->setupUi(this);
	loadProfileCombo();
	connect(&converter, SIGNAL(conversionFinished()), SLOT(handleConversionFinished()));
	connect(&converter, SIGNAL(midiEventProcessed(int, int)), SLOT(updateConversionProgress(int, int)));
	connect(this, SIGNAL(conversionFinished(const QString &, const QString &)), master, SLOT(showBalloon(const QString &, const QString &)));
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
	ui->midiList->setDefaultDropAction(Qt::MoveAction);
#endif // (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
	setAcceptDrops(true);
}

MidiConverterDialog::~MidiConverterDialog() {
	delete ui;
}

void MidiConverterDialog::on_addMidiButton_clicked() {
	if (ui->pcmList->count() == 0) return;
	const QStringList fileNames = showAddMidiFilesDialog();
	ui->midiList->addItems(fileNames);
}

void MidiConverterDialog::on_newPcmButton_clicked() {
	const QStringList fileNames = showAddMidiFilesDialog();
	if (fileNames.isEmpty()) return;
	newPcmFile(fileNames.last());
	ui->midiList->addItems(fileNames);
	ui->midiList->setCurrentRow(ui->midiList->count() - 1);
}

void MidiConverterDialog::on_newGroupButton_clicked() {
	const QStringList fileNames = showAddMidiFilesDialog();
	if (!fileNames.isEmpty()) {
		newPcmFileGroup(fileNames);
	}
}

void MidiConverterDialog::on_addInitButton_clicked() {
	const QStringList fileNames = showAddMidiFilesDialog();
	if (fileNames.isEmpty()) return;

	for (int i = 0; i < ui->pcmList->count(); i++) {
		if (ui->pcmList->currentRow() == i) {
			ui->midiList->insertItems(0, fileNames);
		} else {
			QStringList midiFileNames = ui->pcmList->item(i)->data(Qt::UserRole).value<QStringList>();
			ui->pcmList->item(i)->setData(Qt::UserRole, fileNames + midiFileNames);
		}
	}
}

QStringList MidiConverterDialog::showAddMidiFilesDialog() {
	QSettings *settings = Master::getInstance()->getSettings();
	QString currentDir = settings->value("Master/LastAddMidiFileDir").toString();
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(settings->value("Master/qFileDialogOptions", 0).toInt());
	QStringList fileNames = QFileDialog::getOpenFileNames(this, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*",
		NULL, qFileDialogOptions);
	if (!fileNames.isEmpty()) {
		currentDir = QDir(fileNames.first()).absolutePath();
		settings->setValue("Master/LastAddMidiFileDir", currentDir);
	}
	return fileNames;
}

void MidiConverterDialog::newPcmFile(const QString &proposedPCMFileName) {
	QString pcmFileName = QFileInfo(proposedPCMFileName).fileName();
	if (isSmfFileName(pcmFileName) || isSyxFileName(pcmFileName)) {
		pcmFileName.truncate(pcmFileName.length() - 4);
	}
	pcmFileName += ".wav";

	const QDir pcmFileDir = QFileInfo(Master::getInstance()->getSettings()->value("Master/LastAddPcmFileDir").toString()).dir();
	if (pcmFileDir.exists()) {
		pcmFileName = pcmFileDir.filePath(pcmFileName);
	}

	ui->pcmList->addItem(pcmFileName);
	ui->pcmList->setCurrentRow(ui->pcmList->count() - 1);
}

void MidiConverterDialog::newPcmFileGroup(const QStringList &fileNames) {
	QStringList syxFileNames;
	foreach (QString fileName, fileNames) {
		if (isSyxFileName(fileName)) {
			syxFileNames += fileName;
		} else {
			newPcmFile(fileName);
			if (!syxFileNames.isEmpty()) {
				ui->midiList->addItems(syxFileNames);
				syxFileNames.clear();
			}
			ui->midiList->addItem(fileName);
		}
	}

	if (ui->midiList->count() > 0) {
		ui->midiList->addItems(syxFileNames);
		ui->midiList->setCurrentRow(ui->midiList->count() - 1);
	}
}

void MidiConverterDialog::on_editPcmButton_clicked() {
	if (ui->pcmList->count() == 0) return;
	QListWidgetItem *currentPcmItem = ui->pcmList->currentItem();
	QString pcmFileName = currentPcmItem->text();
	QSettings *settings = Master::getInstance()->getSettings();
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(settings->value("Master/qFileDialogOptions", 0).toInt());
	QString newFileName = QFileDialog::getSaveFileName(this, NULL, pcmFileName, "*.wav *.raw;;*.wav;;*.raw;;*.*",
		NULL, qFileDialogOptions);
	if (newFileName.isEmpty()) return;
	QString newPcmFileDir = QDir(newFileName).absolutePath();
	settings->setValue("Master/LastAddPcmFileDir", newPcmFileDir);
	currentPcmItem->setText(newFileName);
}

void MidiConverterDialog::on_removeButton_clicked() {
	if (ui->midiList->count() > 0) {
		delete ui->midiList->takeItem(ui->midiList->currentRow());
		if (ui->midiList->count() > 0) return;
	}
	if (ui->pcmList->count() > 0) {
		delete ui->pcmList->takeItem(ui->pcmList->currentRow());
		if (ui->midiList->count() > 0) {
			ui->midiList->setCurrentRow(0);
		}
	}
}

void MidiConverterDialog::on_clearButton_clicked() {
	if (ui->midiList->count() == 0) return;
	ui->midiList->clear();
	if (ui->pcmList->count() > 0) {
		delete ui->pcmList->takeItem(ui->pcmList->currentRow());
		if (ui->midiList->count() > 0) {
			ui->midiList->setCurrentRow(0);
		}
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
			emit batchConversionFinished();
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

void MidiConverterDialog::on_midiList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *) {
	bool enabled = current != NULL && !ui->stopButton->isEnabled();
	ui->removeButton->setEnabled(enabled);
	ui->clearButton->setEnabled(enabled);
	ui->moveUpButton->setEnabled(enabled);
	ui->moveDownButton->setEnabled(enabled);
}

void MidiConverterDialog::on_pcmList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
	const QStringList midiFileNames = getMidiFileNames();
	ui->midiList->clear();
	if (previous != NULL) {
		previous->setData(Qt::UserRole, midiFileNames);
	}
	bool enabled = current != NULL && !ui->stopButton->isEnabled();
	ui->startButton->setEnabled(enabled);
	ui->addMidiButton->setEnabled(enabled);
	ui->addInitButton->setEnabled(enabled);
	ui->editPcmButton->setEnabled(enabled);
	if (current == NULL) return;
	const QStringList newMidiFileNames = current->data(Qt::UserRole).value<QStringList>();
	ui->midiList->addItems(newMidiFileNames);
	if (ui->midiList->count() > 0) {
		ui->midiList->setCurrentRow(0);
	}
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
	ui->newPcmButton->setEnabled(enable);
	ui->newGroupButton->setEnabled(enable);
	ui->addMidiButton->setEnabled(enable && ui->pcmList->count() > 0);
	ui->addInitButton->setEnabled(enable && ui->pcmList->count() > 0);
	ui->editPcmButton->setEnabled(enable && ui->pcmList->count() > 0);
	ui->removeButton->setEnabled(enable && ui->midiList->count() > 0);
	ui->clearButton->setEnabled(enable && ui->midiList->count() > 0);
	ui->moveUpButton->setEnabled(enable && ui->midiList->count() > 0);
	ui->moveDownButton->setEnabled(enable && ui->midiList->count() > 0);
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

QStringList MidiConverterDialog::getMidiFileNames() {
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
	QStringList fileNames = Master::parseMidiListFromUrls(e->mimeData()->urls());
	if (fileNames.isEmpty()) {
		e->ignore();
		return;
	}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	const QPoint pos = e->pos();
#else
	const QPoint pos = e->position().toPoint();
#endif
	const bool dropToMidiList = ui->midiList->geometry().contains(pos);
	if (!dropToMidiList) {
		newPcmFileGroup(fileNames);
		return;
	}
	if (ui->pcmList->count() == 0) {
		newPcmFile(fileNames.last());
	}
	QListWidgetItem *dropItem = ui->midiList->itemAt(ui->midiList->mapFromParent(pos));
	if (dropItem != NULL) {
		int dropRow = ui->midiList->row(dropItem);
		ui->midiList->insertItems(dropRow, fileNames);
	} else {
		ui->midiList->addItems(fileNames);
		ui->midiList->setCurrentRow(ui->midiList->count() - 1);
	}
}
