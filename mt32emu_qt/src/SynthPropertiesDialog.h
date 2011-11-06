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

private slots:
	void on_romDirButton_clicked();
	void on_dacEmuComboBox_currentIndexChanged(int);
};

#endif // SYNTHPROPERTIESDIALOG_H
