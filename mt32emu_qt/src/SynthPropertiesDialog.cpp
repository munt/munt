#include <QtGui>

#include "SynthPropertiesDialog.h"
#include "ui_SynthPropertiesDialog.h"

SynthPropertiesDialog::SynthPropertiesDialog(QWidget *parent) :
		QDialog(parent),
		ui(new Ui::SynthPropertiesDialog)
{
	ui->setupUi(this);
}

SynthPropertiesDialog::~SynthPropertiesDialog()
{
	delete ui;
}

void SynthPropertiesDialog::on_romDirButton_clicked()
{
	QString s = QFileDialog::getExistingDirectory(this, "Choose ROM directory", ui->romDirLineEdit->text());
	ui->romDirLineEdit->setText(s);
}
