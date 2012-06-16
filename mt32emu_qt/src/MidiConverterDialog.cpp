/* Copyright (C) 2011, 2012 Jerome Fisher, Sergey V. Mikayev
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

#include "MidiConverterDialog.h"
#include "Master.h"

MidiConverterDialog::MidiConverterDialog(Master *master, QWidget *parent) : QDialog(parent), ui(new Ui::MidiConverterDialog), stopProcessing(false) {
	ui->setupUi(this);
	connect(&converter, SIGNAL(conversionFinished()), SLOT(handleConversionFinished()));
	connect(&converter, SIGNAL(midiEventProcessed(int, int)), SLOT(updateConversionProgress(int, int)));
	connect(this, SIGNAL(conversionFinished(const QString &, const QString &)), master, SLOT(showBalloon(const QString &, const QString &)));
}

MidiConverterDialog::~MidiConverterDialog() {
	delete ui;
}

void MidiConverterDialog::on_addMidiButton_clicked() {
	static QString currentDir = NULL;
	QStringList fileNames = QFileDialog::getOpenFileNames(this, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*");
	if (!fileNames.isEmpty()) {
		currentDir = QDir(fileNames.first()).absolutePath();
		if (ui->pcmList->count() == 0) on_addPcmButton_clicked();
		if (ui->pcmList->count() > 0) {
			QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
			midiFileNames->append(fileNames);
			ui->midiList->clear();
			ui->midiList->addItems(*midiFileNames);
			ui->startButton->setEnabled(true);
		}
	}
}

void MidiConverterDialog::on_addPcmButton_clicked() {
	static QString currentDir = NULL;
	QString fileName = QFileDialog::getSaveFileName(this, NULL, currentDir, "*.wav *.raw;;*.wav;;*.raw;;*.*");
	if (!fileName.isEmpty()) {
		currentDir = QDir(fileName).absolutePath();
		QListWidgetItem *item = new QListWidgetItem;
		QStringList *midiFileNames = new QStringList;
		item->setData(Qt::UserRole, QVariant::fromValue((QObject *)midiFileNames));
		item->setText(fileName);
		ui->pcmList->addItem(item);
		ui->pcmList->setCurrentItem(item);
	}
}

void MidiConverterDialog::on_removeButton_clicked() {
	if (ui->pcmList->count() == 0) return;
	if (ui->midiList->count() > 0) {
		QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
		midiFileNames->removeAt(ui->midiList->currentRow());
		delete ui->midiList->takeItem(ui->midiList->currentRow());
	} else {
		delete ui->pcmList->takeItem(ui->pcmList->currentRow());
		ui->startButton->setEnabled(ui->pcmList->count() > 0);
	}
}

void MidiConverterDialog::on_clearButton_clicked() {
	if (ui->midiList->count() > 0) {
		QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
		midiFileNames->clear();
		ui->midiList->clear();
	} else {
		ui->pcmList->clear();
		ui->startButton->setEnabled(false);
	}
}

void MidiConverterDialog::on_moveUpButton_clicked() {
	int currentRow = ui->midiList->currentRow();
	if (currentRow < 1) return;
	QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
	midiFileNames->move(currentRow, currentRow - 1);
	ui->midiList->clear();
	ui->midiList->addItems(*midiFileNames);
	ui->midiList->setCurrentRow(currentRow - 1);
}

void MidiConverterDialog::on_moveDownButton_clicked() {
	int currentRow = ui->midiList->currentRow();
	if ((currentRow == -1) || ((ui->midiList->count() - 2) < currentRow)) return;
	QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
	midiFileNames->move(currentRow, currentRow + 1);
	ui->midiList->clear();
	ui->midiList->addItems(*midiFileNames);
	ui->midiList->setCurrentRow(currentRow + 1);
}

void MidiConverterDialog::on_startButton_clicked() {
	stopProcessing = false;
	if (ui->pcmList->count() == 0) {
		ui->midiList->clear();
		ui->startButton->setEnabled(false);
		enableControls(true);
		return;
	}
	enableControls(false);
	ui->pcmList->setCurrentRow(0);
	QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
	if (!converter.convertMIDIFile(ui->pcmList->currentItem()->text(), *midiFileNames)) enableControls(true);
}

void MidiConverterDialog::on_stopButton_clicked() {
	stopProcessing = true;
	converter.stop();
	enableControls(true);
}

void MidiConverterDialog::on_synthPropertiesButton_clicked() {
	// TODO implement
}

void MidiConverterDialog::on_pcmList_currentRowChanged(int currentRow) {
	if (currentRow == -1) return;
	QStringList *midiFileNames = (QStringList *)ui->pcmList->currentItem()->data(Qt::UserRole).value<QObject *>();
	ui->midiList->clear();
	ui->midiList->addItems(*midiFileNames);
}

void MidiConverterDialog::handleConversionFinished() {
	ui->progressBar->setValue(0);
	if (stopProcessing) return;
	if (Master::getInstance()->getSettings()->value("Master/showConnectionBalloons", "1").toBool()) {
		emit conversionFinished("MIDI file converted", ui->pcmList->currentItem()->text());
	}
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
	ui->stopButton->setEnabled(!enable);
	ui->startButton->setEnabled(enable && ui->pcmList->count() > 0);
	ui->synthPropertiesButton->setEnabled(enable);
	ui->midiList->setEnabled(enable);
	ui->pcmList->setEnabled(enable);
	ui->addMidiButton->setEnabled(enable);
	ui->addPcmButton->setEnabled(enable);
	ui->removeButton->setEnabled(enable);
	ui->clearButton->setEnabled(enable);
	ui->moveUpButton->setEnabled(enable);
	ui->moveDownButton->setEnabled(enable);
}
