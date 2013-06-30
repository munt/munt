#ifndef MIDI_CONVERTER_DIALOG_H
#define MIDI_CONVERTER_DIALOG_H

#include <QDialog>
#include "ui_MidiConverterDialog.h"
#include "AudioFileWriter.h"

class Master;

namespace Ui {
	class MidiConverterDialog;
}

class MidiConverterDialog : public QDialog {
	Q_OBJECT

public:
	explicit MidiConverterDialog(Master *master, QWidget *parent = 0);
	~MidiConverterDialog();
	void startConversion(const QStringList &fileList);

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

private:
	Ui::MidiConverterDialog *ui;
	AudioFileWriter converter;
	bool batchMode;

	void enableControls(bool enable);
	void loadProfileCombo();
	const QStringList getMidiFileNames();

private slots:
	void on_addMidiButton_clicked();
	void on_addPcmButton_clicked(QString proposedPCMFileName = NULL);
	void on_removeButton_clicked();
	void on_clearButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_startButton_clicked();
	void on_stopButton_clicked();
	void on_pcmList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
	void handleConversionFinished();
	void updateConversionProgress(int midiEventsProcessed, int midiEventsTotal);

signals:
	void conversionFinished(const QString &, const QString &);
};

#endif // MIDI_CONVERTER_DIALOG_H
