/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

MidiPlayerDialog::MidiPlayerDialog(Master *master, QWidget *parent) : QDialog(parent), ui(new Ui::MidiPlayerDialog), smfDriver(master), advancePlayList(false), rowPlaying(-1) {
	ui->setupUi(this);
	ui->playButton->setEnabled(false);
	connect(&smfDriver, SIGNAL(playbackFinished()), SLOT(handlePlaybackFinished()));
	connect(&smfDriver, SIGNAL(playbackTimeChanged(quint64, quint32)), SLOT(handlePlaybackTimeChanged(quint64, quint32)));
	connect(&smfDriver, SIGNAL(tempoUpdated(quint32)), SLOT(handleTempoSet(quint32)));
	connect(this, SIGNAL(playbackStarted(const QString &, const QString &)), master, SLOT(showBalloon(const QString &, const QString &)));
	setAcceptDrops(true);
}

MidiPlayerDialog::~MidiPlayerDialog() {
	delete ui;
}

void MidiPlayerDialog::on_playList_currentRowChanged(int currentRow) {
	ui->playButton->setEnabled(currentRow > -1);
}

void MidiPlayerDialog::on_playList_doubleClicked(const QModelIndex &) {
	on_playButton_clicked();
}

void MidiPlayerDialog::on_addButton_clicked() {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddMidiFileDir").toString();
	QStringList fileNames = QFileDialog::getOpenFileNames(this, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*");
	if (!fileNames.isEmpty()) {
		currentDir = QDir(fileNames.first()).absolutePath();
		Master::getInstance()->getSettings()->setValue("Master/LastAddMidiFileDir", currentDir);
		int row = ui->playList->currentRow();
		ui->playList->insertItems(++row, fileNames);
		ui->playList->setCurrentRow(row);
	}
}

void MidiPlayerDialog::on_addListButton_clicked() {
	static QString currentDir = Master::getInstance()->getSettings()->value("Master/LastAddMidiFileListDir").toString();
	QString fileName = QFileDialog::getOpenFileName(this, NULL, currentDir, "Play list files (*.*)");
	if (!fileName.isEmpty()) {
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
	}
}

void MidiPlayerDialog::on_removeButton_clicked() {
	delete ui->playList->takeItem(ui->playList->currentRow());
}

void MidiPlayerDialog::on_clearButton_clicked() {
	ui->playList->clear();
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
	QString currentFile = ui->playList->currentItem()->text();
	QString prevFile = ui->playList->item(currentRow - 1)->text();
	ui->playList->currentItem()->setText(prevFile);
	ui->playList->item(currentRow - 1)->setText(currentFile);
	ui->playList->setCurrentRow(currentRow - 1);
}

void MidiPlayerDialog::on_moveDownButton_clicked() {
	int currentRow = ui->playList->currentRow();
	if ((currentRow == -1) || ((ui->playList->count() - 2) < currentRow)) return;
	QString currentFile = ui->playList->currentItem()->text();
	QString nextFile = ui->playList->item(currentRow + 1)->text();
	ui->playList->currentItem()->setText(nextFile);
	ui->playList->item(currentRow + 1)->setText(currentFile);
	ui->playList->setCurrentRow(currentRow + 1);
}

void MidiPlayerDialog::on_playButton_clicked() {
	int initialPosition = 0;
	if (rowPlaying != -1) {
		smfDriver.stop();
	} else {
		initialPosition = ui->positionSlider->sliderPosition();
	}
	rowPlaying = ui->playList->currentRow();
	advancePlayList = (rowPlaying == -1);
	handlePlaybackFinished();
	if (initialPosition != 0) smfDriver.jump(initialPosition);
}

void MidiPlayerDialog::on_stopButton_clicked() {
	if (rowPlaying != -1) {
		smfDriver.stop();
	}
	rowPlaying = -1;
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
	if (advancePlayList) {
		if (ui->playList->count() <= ++rowPlaying) {
			rowPlaying = -1;
			ui->playList->clearSelection();
			return;
		}
		ui->playList->setCurrentRow(rowPlaying);
	}
	advancePlayList = true;
	smfDriver.start(ui->playList->currentItem()->text());
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
	QList<QUrl> urls = e->mimeData()->urls();
	for (int i = 0; i < urls.size(); i++) {
		QUrl url = urls.at(i);
		if (url.scheme() != "file") continue;
		QString fileName = url.toLocalFile();
		addPathName(fileName);
		ui->playList->setCurrentRow(ui->playList->count() - 1);
	}
}

void MidiPlayerDialog::addPathName(const QString &fileName) {
	QDir dir = QDir(fileName);
	if (dir.exists()) {
		if (dir.isReadable()) {
			QStringList fileNames = dir.entryList(QStringList() << "*.mid" << "*.smf" << "*.syx");
			foreach (QString fileName, fileNames) {
				ui->playList->addItem(dir.absolutePath() + "/" + fileName);
			}
		}
	} else {
		if (fileName.endsWith(".pls", Qt::CaseInsensitive)) {
			QFile listFile(fileName);
			if (!listFile.open(QIODevice::ReadOnly)) return;
			QTextStream listStream(&listFile);
			while (!listStream.atEnd()) {
				QString s = listStream.readLine();
				if (s.isEmpty()) continue;
				ui->playList->addItem(s);
			}
		} else {
			ui->playList->addItem(fileName);
		}
	}
}

void MidiPlayerDialog::startPlayingFiles(const QStringList &fileList) {
	ui->playList->clear();
	foreach (QString fileName, fileList) {
		addPathName(fileName);
	}
	ui->playList->setCurrentRow(0);
	on_playButton_clicked();
}
