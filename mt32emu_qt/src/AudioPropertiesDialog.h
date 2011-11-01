#ifndef AUDIO_PROPERTIES_DIALOG_H
#define AUDIO_PROPERTIES_DIALOG_H

#include <QDialog>

namespace Ui {
	class AudioPropertiesDialog;
}

class AudioPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AudioPropertiesDialog(QWidget *parent = 0);
	~AudioPropertiesDialog();
	void getData(unsigned int &chunkLen, unsigned int &audioLatency, unsigned int &midiLatency);
	void setData(unsigned int chunkLen, unsigned int audioLatency, unsigned int midiLatency);

private:
	Ui::AudioPropertiesDialog *ui;
};

#endif // AUDIO_PROPERTIES_DIALOG_H
