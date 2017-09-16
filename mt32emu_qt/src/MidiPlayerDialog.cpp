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

#include <QFileDialog>
#include <QDragEnterEvent>

#include "MidiPlayerDialog.h"

MidiPlayerDialog::MidiPlayerDialog(Master *master, QWidget *parent) : QDialog(parent), ui(new Ui::MidiPlayerDialog), smfDriver(master), stopped(true), sliderUpdating(false), paused(false), currentItem() {
	ui->setupUi(this);
	standardTitle = windowTitle();
	ui->playButton->setEnabled(false);
	connect(&smfDriver, SIGNAL(playbackFinished()), SLOT(handlePlaybackFinished()));
	connect(&smfDriver, SIGNAL(playbackTimeChanged(quint64, quint32)), SLOT(handlePlaybackTimeChanged(quint64, quint32)));
	connect(&smfDriver, SIGNAL(tempoUpdated(quint32)), SLOT(handleTempoSet(quint32)));
	connect(this, SIGNAL(playbackStarted(const QString &, const QString &)), master, SLOT(showBalloon(const QString &, const QString &)));
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
	ui->playList->setDefaultDropAction(Qt::MoveAction);
#endif // (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
	setAcceptDrops(true);
}

MidiPlayerDialog::~MidiPlayerDialog() {
	delete ui;
}

void MidiPlayerDialog::on_playList_currentRowChanged(int currentRow) {
	ui->playButton->setEnabled(currentRow > -1);
}

void MidiPlayerDialog::on_playList_activated(const QModelIndex &) {
	paused = false;
	currentItem = ui->playList->currentItem();
	updateCurrentItem();
	on_playButton_clicked();
}

void MidiPlayerDialog::on_addButton_clicked() {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddMidiFileDir").toString();
	QStringList fileNames = QFileDialog::getOpenFileNames(this, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*");
	if (fileNames.isEmpty()) return;
	currentDir = QDir(fileNames.first()).absolutePath();
	Master::getInstance()->getSettings()->setValue("Master/LastAddMidiFileDir", currentDir);
	int row = ui->playList->currentRow();
	ui->playList->insertItems(row + 1, fileNames);
	ui->playList->setCurrentRow(row + fileNames.count());
	updateCurrentItem();
}

void MidiPlayerDialog::on_addListButton_clicked() {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddMidiFileListDir").toString();
	QString fileName = QFileDialog::getOpenFileName(this, NULL, currentDir, "Play list files (*.*)");
	if (fileName.isEmpty()) return;
	currentDir = QDir(fileName).absolutePath();
	Master::getInstance()->getSettings()->setValue("Master/LastAddMidiFileListDir", currentDir);
	QFile listFile(fileName);
	if (!listFile.open(QIODevice::ReadOnly)) return;
	QTextStream listStream(&listFile);
	int row = ui->playList->currentRow();
	while (!listStream.atEnd()) {
		QString s = listStream.readLine();
		if (s.isEmpty()) continue;
		ui->playList->insertItem(++row, s);
	}
	ui->playList->setCurrentRow(row);
	updateCurrentItem();
}

void MidiPlayerDialog::on_removeButton_clicked() {
	QListWidgetItem *item = ui->playList->takeItem(ui->playList->currentRow());
	if (currentItem == item) {
		// This ensures that playback will finish after the removed item.
		currentItem = NULL;
		updateCurrentItem();
	}
	delete item;
}

void MidiPlayerDialog::on_clearButton_clicked() {
	currentItem = NULL;
	ui->playList->clear();
	updateCurrentItem();
}

void MidiPlayerDialog::on_saveListButton_clicked() {
	static QString currentDir = NULL;
	QString fileName = QFileDialog::getSaveFileName(this, NULL, currentDir, "Play list files (*.*)");
	if (!fileName.isEmpty()) {
		currentDir = QDir(fileName).absolutePath();
		QFile listFile(fileName);
		if (!listFile.open(QIODevice::WriteOnly)) return;
		QTextStream listStream(&listFile);
		for (int i = 0; i < ui->playList->count(); i++) {
			listStream << ui->playList->item(i)->text() << endl;
		}
	}
}

void MidiPlayerDialog::on_moveUpButton_clicked() {
	int currentRow = ui->playList->currentRow();
	if (currentRow < 1) return;
	QListWidgetItem *prevItem = ui->playList->takeItem(currentRow - 1);
	ui->playList->insertItem(currentRow, prevItem);
}

void MidiPlayerDialog::on_moveDownButton_clicked() {
	int currentRow = ui->playList->currentRow();
	if ((currentRow == -1) || ((ui->playList->count() - 2) < currentRow)) return;
	QListWidgetItem *nextItem = ui->playList->takeItem(currentRow + 1);
	ui->playList->insertItem(currentRow, nextItem);
}

void MidiPlayerDialog::on_playButton_clicked() {
	if (paused) {
		paused = !paused;
		smfDriver.pause(paused);
		return;
	}
	if (currentItem == NULL) {
		currentItem = ui->playList->currentItem();
		if (currentItem == NULL) return;
		updateCurrentItem();
	}
	int initialPosition = 0;
	if (stopped) {
		initialPosition = ui->positionSlider->sliderPosition();
	} else {
		smfDriver.stop();
	}
	stopped = true;
	handlePlaybackFinished();
	if (initialPosition != 0) smfDriver.jump(initialPosition);
}

void MidiPlayerDialog::on_pauseButton_clicked() {
	if (!stopped) {
		paused = !paused;
		smfDriver.pause(paused);
	}
}

void MidiPlayerDialog::on_stopButton_clicked() {
	paused = false;
	if (!stopped) {
		stopped = true;
		smfDriver.stop();
		updateCurrentItem();
	}
	ui->tempoSpinBox->setValue(MidiParser::DEFAULT_BPM);
}

void MidiPlayerDialog::on_fastForwardButton_pressed() {
	smfDriver.setFastForwardingFactor(4);
}

void MidiPlayerDialog::on_fastForwardButton_released() {
	smfDriver.setFastForwardingFactor(0);
}

void MidiPlayerDialog::on_fastFastForwardButton_pressed() {
	smfDriver.setFastForwardingFactor(16);
}

void MidiPlayerDialog::on_fastFastForwardButton_released() {
	smfDriver.setFastForwardingFactor(0);
}

void MidiPlayerDialog::on_tempoSpinBox_valueChanged(int newValue) {
	if (stopped) return;
	smfDriver.setBPM(newValue);
}

void MidiPlayerDialog::on_positionSlider_valueChanged() {
	if (ui->positionSlider->isSliderDown() || sliderUpdating) return;
	smfDriver.jump(ui->positionSlider->value());
}

void MidiPlayerDialog::on_positionSlider_sliderReleased() {
	smfDriver.jump(ui->positionSlider->value());
}

void MidiPlayerDialog::handlePlaybackFinished() {
	ui->tempoSpinBox->setValue(MidiParser::DEFAULT_BPM);
	if (stopped) {
		stopped = false;
		ui->playList->setCurrentRow(ui->playList->row(currentItem));
	} else {
		int rowPlaying = ui->playList->row(currentItem);
		if (rowPlaying++ == -1 || ui->playList->count() <= rowPlaying) {
			currentItem = NULL;
			if (ui->playList->count() > 0) {
				ui->playList->setCurrentRow(0);
			}
			stopped = true;
			updateCurrentItem();
			return;
		}
		ui->playList->setCurrentRow(rowPlaying);
		currentItem = ui->playList->currentItem();
		updateCurrentItem();
	}
	smfDriver.start(currentItem->text());
	if (Master::getInstance()->getSettings()->value("Master/showConnectionBalloons", "1").toBool()) {
		emit playbackStarted("Playing MIDI file", QFileInfo(ui->playList->currentItem()->text()).fileName());
	}
}

void MidiPlayerDialog::handlePlaybackTimeChanged(quint64 currentNanos, quint32 totalSeconds) {
	quint32 currentSeconds = currentNanos / MasterClock::NANOS_PER_SECOND;
	QChar z = QChar('0');
	QString pos = QString("%1:%2 / %3:%4").arg(currentSeconds / 60, 2, 10, z).arg(currentSeconds % 60, 2, 10, z).arg(totalSeconds / 60, 2, 10, z).arg(totalSeconds % 60, 2, 10, z);
	ui->positionLabel->setText(pos);
	if (!ui->positionSlider->isSliderDown()) {
		sliderUpdating = true;
		ui->positionSlider->setSliderPosition((totalSeconds != 0) ? currentNanos / MasterClock::NANOS_PER_MILLISECOND / totalSeconds : 0);
		sliderUpdating = false;
	}
}

void MidiPlayerDialog::handleTempoSet(quint32 tempo) {
	ui->tempoSpinBox->setValue(tempo == 0 ? MidiParser::DEFAULT_BPM : tempo);
}

void MidiPlayerDialog::dragEnterEvent(QDragEnterEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MidiPlayerDialog::dragMoveEvent(QDragMoveEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MidiPlayerDialog::dropEvent(QDropEvent *e) {
	if (!e->isAccepted()) {
		Master::isSupportedDropEvent(e);
		if (!e->isAccepted()) return;
	}
	QStringList fileNames = Master::parseMidiListFromUrls(e->mimeData()->urls());
	if (fileNames.isEmpty()) {
		e->ignore();
		return;
	}

	const QPoint pos = e->pos();
	const bool dropToMidiList = ui->playList->geometry().contains(pos);
	if (dropToMidiList) {
		QListWidgetItem *dropItem = ui->playList->itemAt(ui->playList->mapFromParent(pos));
		if (dropItem != NULL) {
			int dropRow = ui->playList->row(dropItem);
			ui->playList->insertItems(dropRow, fileNames);
			return;
		}
	}
	ui->playList->addItems(fileNames);
	ui->playList->setCurrentRow(ui->playList->count() - 1);
	updateCurrentItem();
}

void MidiPlayerDialog::startPlayingFiles(const QStringList &fileList) {
	ui->playList->clear();
	foreach (QString fileName, fileList) {
		ui->playList->addItems(Master::parseMidiListFromPathName(fileName));
	}
	updateCurrentItem();
	on_playButton_clicked();
}

void MidiPlayerDialog::updateCurrentItem() {
	if (ui->playList->currentRow() == -1 && ui->playList->count() > 0) {
		ui->playList->setCurrentRow(0);
	}
	if (stopped && currentItem == NULL && ui->playList->count() > 0) {
		currentItem = ui->playList->item(0);
	} else if (currentItem != NULL && ui->playList->count() == 0) {
		currentItem = NULL;
	}
	QString title = standardTitle;
	if (currentItem != NULL) {
		title += " - " + currentItem->text();
	}
	setWindowTitle(title);
}
