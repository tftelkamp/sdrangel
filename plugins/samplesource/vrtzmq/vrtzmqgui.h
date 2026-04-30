///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2022 Jon Beniston, M7RCE <jon@beniston.com>                     //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_VRTZMQGUI_H
#define INCLUDE_VRTZMQGUI_H

#include <device/devicegui.h>
#include <QTimer>
#include <QWidget>

#include "util/messagequeue.h"

#include "vrtzmqsettings.h"
#include "vrtzmqinput.h"

class DeviceUISet;

namespace Ui {
	class VRTZMQGui;
	class VRTZMQSampleRates;
}

class VRTZMQGui : public DeviceGUI {
	Q_OBJECT

public:
	explicit VRTZMQGui(DeviceUISet *deviceUISet, QWidget* parent = nullptr);
	~VRTZMQGui() final;

	void resetToDefaults() final;
	QByteArray serialize() const final;
	bool deserialize(const QByteArray& data) final;
	MessageQueue *getInputMessageQueue() final { return &m_inputMessageQueue; }
    // void setReplayTime(float time) override;

private:
	Ui::VRTZMQGui* ui;

    bool m_doApplySettings;
	bool m_forceSettings;
	VRTZMQSettings m_settings;
    QList<QString> m_settingsKeys;
    bool m_sampleRateMode; //!< true: device, false: base band sample rate update mode
	QTimer m_updateTimer;
	std::vector<int> m_gains;
	VRTZMQInput* m_sampleSource;
    int m_sampleRate;
    quint64 m_deviceCenterFrequency; //!< Center frequency in device
	MessageQueue m_inputMessageQueue;

	void displayGains();
    void displaySampleRate();
    void displayFcTooltip();
	void displaySettings();
	void displayReplayLength();
	void displayReplayOffset();
	void displayReplayStep();
	void sendSettings();
	void updateSampleRateAndFrequency();
    void blockApplySettings(bool block);
	bool handleMessage(const Message& message);
    void makeUIConnections() const;

private slots:
    void handleInputMessages();
	void on_centerFrequency_changed(quint64 value);
	void on_sampleRate_changed(quint64 value);
	void on_HostName_editingFinished();
	void on_Instance_valueChanged(int value);
	void on_Port_valueChanged(int value);
	void on_Channel_valueChanged(int value);
	void on_UsePort_stateChanged(int state);
	void on_dcOffset_toggled(bool checked);
	void on_iqImbalance_toggled(bool checked);
	void on_decim_currentIndexChanged(int index);
	void on_fcPos_currentIndexChanged(int index);
	void on_checkBox_stateChanged(int state);
	void on_startStop_toggled(bool checked);
    void openDeviceSettingsDialog(const QPoint& p);
	void updateHardware();
	void updateStatus();
};

#endif // INCLUDE_VRTZMQGUI_H
