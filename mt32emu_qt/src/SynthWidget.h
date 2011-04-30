#ifndef SYNTHWIDGET_H
#define SYNTHWIDGET_H

#include <QWidget>

#include "QSynth.h"
#include "SynthPropertiesDialog.h"

#include "ALSADriver.h"

namespace Ui {
    class SynthWidget;
}

class SynthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SynthWidget(QWidget *parent = 0);
    ~SynthWidget();

private:
    Ui::SynthWidget *ui;
	SynthPropertiesDialog spd;
	ALSADriver *alsaDriver;
	QSynth synth;

private slots:
	void on_startButton_clicked();
 void on_synthPropertiesButton_clicked();
};

#endif // SYNTHWIDGET_H
