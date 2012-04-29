#ifndef MIDI_PLAYER_DIALOG_H
#define MIDI_PLAYER_DIALOG_H

#include <QDialog>
#include "ui_MidiPlayerDialog.h"
#include "mididrv/SMFDriver.h"

class Master;

namespace Ui {
	class MidiPlayerDialog;
}

class MidiPlayerDialog : public QDialog {
	Q_OBJECT

public:
	explicit MidiPlayerDialog(Master *master, QWidget *parent = 0);
	~MidiPlayerDialog();

private:
	Ui::MidiPlayerDialog *ui;
	SMFDriver smfDriver;
	bool advancePlayList;
	int rowPlaying;

private slots:
	void on_addButton_clicked();
	void on_removeButton_clicked();
	void on_clearButton_clicked();
	void on_playButton_clicked();
	void on_stopButton_clicked();
	void handlePlaybackFinished();
	void handlePlaybackTimeChanged(quint64 currentNanos, quint32 totalSeconds);
};

#endif // MIDI_PLAYER_DIALOG_H
