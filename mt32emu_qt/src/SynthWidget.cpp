#include "SynthWidget.h"
#include "ui_SynthWidget.h"

#include "ALSADriver.h"

SynthWidget::SynthWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SynthWidget)
{
	alsaDriver = NULL;
    ui->setupUi(this);
}

SynthWidget::~SynthWidget()
{
    delete ui;
	delete alsaDriver;
}

void SynthWidget::on_synthPropertiesButton_clicked()
{
	spd.exec();
}

void SynthWidget::on_startButton_clicked()
{
	if (synth.open() == 0) {
		ui->startButton->setEnabled(false);
		ui->stopButton->setEnabled(true);
		ui->statusLabel->setText("Opened!");
		if (alsaDriver == NULL) {
			alsaDriver = new ALSADriver(&synth);
		}
	} else {
		ui->statusLabel->setText("Open failed :(");
	}
}
