#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "SynthWidget.h"

namespace Ui {
    class MainWindow;
}

class Master;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	explicit MainWindow(Master *master, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
	void on_actionAbout_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
};

#endif // MAINWINDOW_H
