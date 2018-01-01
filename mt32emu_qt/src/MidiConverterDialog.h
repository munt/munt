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
	AudioFileRenderer converter;
	bool batchMode;

	void enableControls(bool enable);
	void loadProfileCombo();
	QStringList getMidiFileNames();
	QStringList showAddMidiFilesDialog();
	void newPcmFile(const QString &proposedPCMFileName);
	void newPcmFileGroup(const QStringList &fileNames);

private slots:
	void on_newPcmButton_clicked();
	void on_newGroupButton_clicked();
	void on_addMidiButton_clicked();
	void on_addInitButton_clicked();
	void on_editPcmButton_clicked();
	void on_removeButton_clicked();
	void on_clearButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_startButton_clicked();
	void on_stopButton_clicked();
	void on_midiList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
	void on_pcmList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
	void handleConversionFinished();
	void updateConversionProgress(int midiEventsProcessed, int midiEventsTotal);

signals:
	void conversionFinished(const QString &, const QString &);
	void batchConversionFinished();
};

#endif // MIDI_CONVERTER_DIALOG_H
