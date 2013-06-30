#ifndef AUDIO_PROPERTIES_DIALOG_H
#define AUDIO_PROPERTIES_DIALOG_H

#include <QDialog>

#include "audiodrv/AudioDriver.h"

namespace Ui {
	class AudioPropertiesDialog;
}

class AudioPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AudioPropertiesDialog(QWidget *parent = 0);
	~AudioPropertiesDialog();
	void getData(AudioDriverSettings &driverSettings);
	void setData(const AudioDriverSettings &driverSettings);
	void setCheckText(QString text);

private:
	Ui::AudioPropertiesDialog *ui;
};

#endif // AUDIO_PROPERTIES_DIALOG_H
