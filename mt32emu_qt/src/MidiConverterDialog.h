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

private:
	Ui::MidiConverterDialog *ui;
	AudioFileWriter converter;
	bool stopProcessing;

	void enableControls(bool enable);

private slots:
	void on_addMidiButton_clicked();
	void on_addPcmButton_clicked();
	void on_removeButton_clicked();
	void on_clearButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_startButton_clicked();
	void on_stopButton_clicked();
	void on_synthPropertiesButton_clicked();
	void on_pcmList_currentRowChanged(int currentRow);
	void handleConversionFinished();
	void updateConversionProgress(int midiEventsProcessed, int midiEventsTotal);

signals:
	void conversionFinished(const QString &, const QString &);
};

#endif // MIDI_CONVERTER_DIALOG_H
