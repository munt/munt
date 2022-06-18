#ifndef MIDI_PROPERTIES_DIALOG_H
#define MIDI_PROPERTIES_DIALOG_H

#include <QDialog>

namespace Ui {
	class MidiPropertiesDialog;
}

class MidiPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MidiPropertiesDialog(QWidget *parent = 0);
	~MidiPropertiesDialog();
	int getCurrentMidiPortIndex();
	QString getMidiPortName();
	void setMidiList(QStringList useMidiPortList, int selectedIndex = -1);
	void setMidiPortListEnabled(bool enabled);
	void setMidiPortName(QString name);
	void setMidiPortNameEditorEnabled(bool enabled);

private:
	Ui::MidiPropertiesDialog *ui;

private slots:
	void on_midiPortList_itemSelectionChanged();
};

#endif // MIDI_PROPERTIES_DIALOG_H
