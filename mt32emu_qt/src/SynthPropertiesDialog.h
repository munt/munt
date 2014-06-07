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

protected:
	void showEvent(QShowEvent *showEvent);

private:
	Ui::SynthPropertiesDialog *ui;
	SynthRoute *synthRoute;
	SynthProfile synthProfile;
	ROMSelectionDialog rsd;
	void resetSynth();
	void restoreDefaults();
	void loadSynthProfile();
	void saveSynthProfile();
	void refreshProfileCombo();
	QString getROMSetDescription();

private slots:
	void on_changeROMSetButton_clicked();
	void on_midiDelayEmuComboBox_currentIndexChanged(int);
	void on_dacEmuComboBox_currentIndexChanged(int);
	void on_reverbCompatibilityComboBox_currentIndexChanged(int);
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_profileComboBox_currentIndexChanged(int);
	void on_reverbCheckBox_stateChanged(int state);
	void on_outputGainSlider_valueChanged(int value);
	void on_reverbOutputGainSlider_valueChanged(int value);
	void on_reverseStereoCheckBox_stateChanged(int state);
	void updateReverbSettings();
	void handleReverbModeChanged(int mode);
	void handleReverbTimeChanged(int time);
	void handleReverbLevelChanged(int level);
};

#endif // SYNTHPROPERTIESDIALOG_H
