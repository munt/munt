#ifndef SYNTHWIDGET_H
#define SYNTHWIDGET_H

#include <QtGui>

#include "SynthRoute.h"
#include "SynthPropertiesDialog.h"
#include "AudioPropertiesDialog.h"
#include "MidiPropertiesDialog.h"

class Master;
class AudioDevice;
class QListWidgetItem;
class SynthStateMonitor;

namespace Ui {
	class SynthWidget;
}

class SynthWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SynthWidget(Master *master, SynthRoute *useSynthRoute, const AudioDevice *audioDevice, QWidget *parent = 0);
	~SynthWidget();
	SynthRoute *getSynthRoute();

protected:
	void hideEvent(QHideEvent *hideEvent);
	void showEvent(QShowEvent *showEvent);

private:
	SynthRoute *synthRoute;
	Ui::SynthWidget *ui;
	SynthStateMonitor *synthStateMonitor;
	SynthPropertiesDialog spd;
	AudioPropertiesDialog apd;
	MidiPropertiesDialog mpd;

	static const QIcon &getSynthDetailsIcon(bool visible);

	void refreshAudioDeviceList(Master *master, const AudioDevice *useAudioDevice);
	int findMIDISession(MidiSession *midiSession);
	MidiSession *getSelectedMIDISession();
	void setEmuModeText();

private slots:
	void on_startButton_clicked();
	void on_audioPropertiesButton_clicked();
	void on_stopButton_clicked();
	void on_refreshButton_clicked();
	void on_pinCheckBox_stateChanged(int state);
	void on_audioDeviceComboBox_currentIndexChanged(int audioDeviceIndex);
	void on_midiList_itemSelectionChanged();
	void on_midiAdd_clicked();
	void on_midiRemove_clicked();
	void on_midiProperties_clicked();
	void on_midiRecord_clicked();
	void on_audioRecord_clicked();
	void on_masterVolumeSlider_valueChanged(int newValue);
	void on_detailsButton_clicked();
	void handleSynthRouteState(SynthRouteState state);
	void handleSynthRoutePinned();
	void handleMIDISessionAdded(MidiSession *midiSession);
	void handleMIDISessionRemoved(MidiSession *midiSession);
	void handleMIDISessionNameChanged(MidiSession *midiSession);
	void handleMasterVolumeChanged(int volume);
};

#endif // SYNTHWIDGET_H
