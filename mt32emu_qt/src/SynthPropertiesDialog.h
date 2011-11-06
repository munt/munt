#ifndef SYNTHPROPERTIESDIALOG_H
#define SYNTHPROPERTIESDIALOG_H

#include <QDialog>

#include "SynthRoute.h"
#include "ROMSelectionDialog.h"

namespace Ui {
	class SynthPropertiesDialog;
}

class SynthPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SynthPropertiesDialog(QWidget *parent = 0, SynthRoute *useSynthRoute = NULL);
	~SynthPropertiesDialog();

private:
	Ui::SynthPropertiesDialog *ui;
	SynthRoute *synthRoute;
	ROMSelectionDialog rsd;
	void resetSynth();
	void restoreDefaults();

private slots:
	void on_romDirButton_clicked();
	void on_dacEmuComboBox_currentIndexChanged(int);
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_reverbCheckBox_stateChanged(int state);
	void on_outputGainSlider_valueChanged(int value);
	void on_reverbOutputGainSlider_valueChanged(int value);
	void updateReverbSettings();
};

#endif // SYNTHPROPERTIESDIALOG_H
