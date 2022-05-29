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
	void dropEvent(QDropEvent *event);
	void startPlayingFiles(const QStringList &fileList);

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);

private:
	Ui::MidiPlayerDialog *ui;
	QString standardTitle;
	SMFDriver smfDriver;
	bool stopped;
	bool sliderUpdating;
	bool paused;
	const QListWidgetItem *currentItem;

	void updateCurrentItem();

private slots:
	void on_playList_currentRowChanged(int currentRow);
	void on_playList_activated(const QModelIndex &index);
	void on_addButton_clicked();
	void on_addListButton_clicked();
	void on_removeButton_clicked();
	void on_clearButton_clicked();
	void on_saveListButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_playButton_clicked();
	void on_pauseButton_clicked();
	void on_stopButton_clicked();
	void on_fastForwardButton_pressed();
	void on_fastForwardButton_released();
	void on_fastFastForwardButton_pressed();
	void on_fastFastForwardButton_released();
	void on_tempoSpinBox_valueChanged(int newValue);
	void on_positionSlider_valueChanged();
	void on_positionSlider_sliderReleased();
	void handlePlaybackFinished(bool successful);
	void handlePlaybackTimeChanged(quint64 currentNanos, quint32 totalSeconds);
	void handleTempoSet(quint32 tempo);

signals:
	void playbackStarted(const QString &, const QString &);
};

#endif // MIDI_PLAYER_DIALOG_H
