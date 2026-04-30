///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014-2015 John Greb <hexameron@spam.no>                         //
// Copyright (C) 2015-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2021-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
// Copyright (C) 2021 Kacper Michajłow <kasper93@gmail.com>                      //
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

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

#include "vrtzmqgui.h"

#include "device/deviceapi.h"
#include "device/deviceuiset.h"

#include "ui_vrtzmqgui.h"
#include "gui/colormapper.h"
#include "gui/glspectrum.h"
#include "mainspectrum/mainspectrumgui.h"
#include "gui/basicdevicesettingsdialog.h"
#include "gui/dialogpositioner.h"
#include "dsp/dspcommands.h"

VRTZMQGui::VRTZMQGui(DeviceUISet *deviceUISet, QWidget* parent) :
	DeviceGUI(parent),
	ui(new Ui::VRTZMQGui),
	m_doApplySettings(true),
	m_forceSettings(true),
	m_settings(),
    m_sampleRateMode(true),
	m_sampleSource(nullptr)
{
    m_deviceUISet = deviceUISet;
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_sampleSource = (VRTZMQInput*) m_deviceUISet->m_deviceAPI->getSampleSource();

    ui->setupUi(getContents());
    sizeToContents();
    getContents()->setStyleSheet("#VRTZMQGui { background-color: rgb(64, 64, 64); }");
    m_helpURL = "plugins/samplesource/vrtzmq/readme.md";
	ui->centerFrequency->setColorMapper(ColorMapper(ColorMapper::GrayGold));

    ui->sampleRate->setColorMapper(ColorMapper(ColorMapper::GrayGreenYellow));

    ui->HostName->clear();
    ui->HostName->setText(m_settings.m_host);

    ui->Instance->setValue(m_settings.m_instance);
    ui->Channel->setValue(m_settings.m_channel);
    ui->Port->setValue(m_settings.m_port);
    ui->UsePort->setChecked(m_settings.m_useport);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(openDeviceSettingsDialog(const QPoint &)));
	connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updateHardware()));
	connect(deviceUISet->m_deviceAPI, &DeviceAPI::stateChanged, this, &VRTZMQGui::updateStatus);
	updateStatus();

	displaySettings();
    makeUIConnections();
    m_resizer.enableChildMouseTracking();

	connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()), Qt::QueuedConnection);
    m_sampleSource->setMessageQueueToGUI(&m_inputMessageQueue);
}

VRTZMQGui::~VRTZMQGui()
{
    qDebug("VRTZMQGui::~VRTZMQGui");
	delete ui;
    qDebug("VRTZMQGui::~VRTZMQGui: end");
}

void VRTZMQGui::resetToDefaults()
{
	m_settings.resetToDefaults();
	displaySettings();
    m_forceSettings = true;
	sendSettings();
}

void VRTZMQGui::on_dcOffset_toggled(bool checked)
{
	m_settings.m_dcBlock = checked;
    m_settingsKeys.append("dcBlock");
	sendSettings();
}

void VRTZMQGui::on_iqImbalance_toggled(bool checked)
{
	m_settings.m_iqImbalance = checked;
    m_settingsKeys.append("iqImbalance");
	sendSettings();
}

QByteArray VRTZMQGui::serialize() const
{
    return m_settings.serialize();
}

bool VRTZMQGui::deserialize(const QByteArray& data)
{
    if(m_settings.deserialize(data))
    {
        displaySettings();
        m_forceSettings = true;
        sendSettings();
        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

bool VRTZMQGui::handleMessage(const Message& message)
{
	if (VRTZMQInput::MsgConfigureVRTZMQ::match(message))
	{
        auto& cfg = (const VRTZMQInput::MsgConfigureVRTZMQ&) message;

        if (cfg.getForce()) {
            m_settings = cfg.getSettings();
        } else {
            m_settings.applySettings(cfg.getSettingsKeys(), cfg.getSettings());
        }

        blockApplySettings(true);
        displaySettings();
        blockApplySettings(false);
        return true;
	}
	else if (VRTZMQInput::MsgStartStop::match(message))
    {
        auto& notif = (const VRTZMQInput::MsgStartStop&) message;
        blockApplySettings(true);
        ui->startStop->setChecked(notif.getStartStop());
        blockApplySettings(false);

        return true;
    }
	else
	{
		return false;
	}
}

void VRTZMQGui::handleInputMessages()
{
    Message* message;

    while ((message = m_inputMessageQueue.pop()) != nullptr)
    {
        qDebug("VRTZMQGui::handleInputMessages: message: %s", message->getIdentifier());

        if (DSPSignalNotification::match(*message))
        {
            auto* notif = (const DSPSignalNotification*) message;
            m_sampleRate = notif->getSampleRate();
            m_deviceCenterFrequency = notif->getCenterFrequency();
            qDebug("VRTZMQGui::handleInputMessages: DSPSignalNotification: SampleRate:%d, CenterFrequency:%llu", notif->getSampleRate(), notif->getCenterFrequency());
            updateSampleRateAndFrequency();

            delete message;
        }
        else
        {
            if (handleMessage(*message))
            {
                delete message;
            }
        }
    }
}

void VRTZMQGui::updateSampleRateAndFrequency()
{
    m_deviceUISet->getSpectrum()->setSampleRate(m_sampleRate);
    m_deviceUISet->getSpectrum()->setCenterFrequency(m_deviceCenterFrequency);
    displaySampleRate();
}

void VRTZMQGui::displaySampleRate()
{
    ui->sampleRate->blockSignals(true);
    displayFcTooltip();

    {
        ui->sampleRateMode->setStyleSheet("QToolButton { background:rgb(60,60,60); }");
        ui->sampleRateMode->setText("SR");

        ui->sampleRate->setValue(m_sampleRate);
        ui->sampleRate->setToolTip("Device to host sample rate (S/s)");
        ui->deviceRateText->setToolTip("Baseband sample rate (S/s)");
        uint32_t basebandSampleRate = m_sampleRate; ///(1<<m_settings.m_log2Decim);
        ui->deviceRateText->setText(tr("%1k").arg(QString::number((float) basebandSampleRate / 1000.0f, 'g', 5)));

        ui->centerFrequency->setValue(m_deviceCenterFrequency);
        
    }

    ui->sampleRate->blockSignals(false);
}

void VRTZMQGui::displayFcTooltip()
{
    int32_t fShift = DeviceSampleSource::calculateFrequencyShift(
        m_settings.m_log2Decim,
        (DeviceSampleSource::fcPos_t) m_settings.m_fcPos,
        m_sampleRate,
        DeviceSampleSource::FrequencyShiftScheme::FSHIFT_STD
    );
    ui->fcPos->setToolTip(tr("Relative position of device center frequency: %1 kHz").arg(QString::number((float) fShift / 1000.0f, 'g', 5)));
}

void VRTZMQGui::displaySettings()
{
    setTitle(m_settings.m_title);
    getDeviceUISet()->m_mainSpectrumGUI->setTitle(m_settings.m_title);
	ui->centerFrequency->setValue(m_deviceCenterFrequency / 1000);
	displaySampleRate();
    ui->HostName->setText(m_settings.m_host);
    ui->Channel->setValue(m_settings.m_channel);
    ui->Port->setValue(m_settings.m_port);
    ui->Instance->setValue(m_settings.m_instance);
    ui->UsePort->setChecked(m_settings.m_useport);
	ui->dcOffset->setChecked(m_settings.m_dcBlock);
	ui->iqImbalance->setChecked(m_settings.m_iqImbalance);
	ui->decim->setCurrentIndex(m_settings.m_log2Decim);
	ui->fcPos->setCurrentIndex((int) m_settings.m_fcPos);
}

void VRTZMQGui::sendSettings()
{
	if (!m_updateTimer.isActive()) {
		m_updateTimer.start(100);
	}
}

void VRTZMQGui::on_centerFrequency_changed(quint64 value)
{
	// m_settings.m_centerFrequency = value * 1000;
    // m_settingsKeys.append("centerFrequency");
	// sendSettings();
}

void VRTZMQGui::on_decim_currentIndexChanged(int index)
{
	if ((index <0) || (index > 6)) {
		return;
	}

	m_settings.m_log2Decim = index;
    displaySampleRate();

    m_settingsKeys.append("log2Decim");
	sendSettings();
}

void VRTZMQGui::on_fcPos_currentIndexChanged(int index)
{
    m_settings.m_fcPos = (VRTZMQSettings::fcPos_t) (index < 0 ? 0 : index > 2 ? 2 : index);
    displayFcTooltip();
    m_settingsKeys.append("fcPos");
    sendSettings();
}


void VRTZMQGui::on_startStop_toggled(bool checked)
{
    if (m_doApplySettings)
    {
        VRTZMQInput::MsgStartStop *message = VRTZMQInput::MsgStartStop::create(checked);
        m_sampleSource->getInputMessageQueue()->push(message);
    }
}


void VRTZMQGui::updateHardware()
{
    if (m_doApplySettings)
    {
        VRTZMQInput::MsgConfigureVRTZMQ* message = VRTZMQInput::MsgConfigureVRTZMQ::create(m_settings, m_settingsKeys, m_forceSettings);
        m_sampleSource->getInputMessageQueue()->push(message);
        m_forceSettings = false;
        m_settingsKeys.clear();
        m_updateTimer.stop();
    }
}

void VRTZMQGui::updateStatus()
{
    int state = m_deviceUISet->m_deviceAPI->state();

    switch(state)
    {
        case DeviceAPI::StNotStarted:
            ui->startStop->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
            break;
        case DeviceAPI::StIdle:
            ui->startStop->setStyleSheet("QToolButton { background-color : blue; }");
            break;
        case DeviceAPI::StRunning:
            ui->startStop->setStyleSheet("QToolButton { background-color : green; }");
            break;
        case DeviceAPI::StError:
            ui->startStop->setStyleSheet("QToolButton { background-color : red; }");
            QMessageBox::information(this, tr("Message"), m_deviceUISet->m_deviceAPI->errorMessage());
            break;
        default:
            break;
    }
}

void VRTZMQGui::blockApplySettings(bool block)
{
    m_doApplySettings = !block;
}

void VRTZMQGui::on_checkBox_stateChanged(int state)
{

}

void VRTZMQGui::on_sampleRate_changed(quint64 value)
{

}

void VRTZMQGui::openDeviceSettingsDialog(const QPoint& p)
{
    if (m_contextMenuType == ContextMenuDeviceSettings)
    {
        BasicDeviceSettingsDialog dialog(this);

        dialog.setDefaultTitle(getDefaultTitle());
        dialog.setTitle(m_settings.m_title);

        dialog.move(p);
        new DialogPositioner(&dialog, false);
        dialog.exec();

        if (dialog.result() == QDialog::Accepted)
        {
           
            m_settings.m_title = dialog.getTitle();
            setTitle(m_settings.m_title);
            getDeviceUISet()->m_mainSpectrumGUI->setTitle(m_settings.m_title);
            m_settingsKeys.append("title");

            sendSettings();
        }
    }

    resetContextMenuType();
}

void VRTZMQGui::on_HostName_editingFinished()
{
    QString text = ui->HostName->text().trimmed();
    if (text != m_settings.m_host)
    {
        m_settings.m_host = text;
        m_settingsKeys.append("host");
        sendSettings();
    }
}

void VRTZMQGui::on_Instance_valueChanged(int value)
{
    m_settings.m_instance = value;
    m_settingsKeys.append("instance");
    sendSettings();
}

void VRTZMQGui::on_Port_valueChanged(int value)
{
    m_settings.m_port = value;
    m_settingsKeys.append("port");
    sendSettings();
}

void VRTZMQGui::on_Channel_valueChanged(int value)
{
    m_settings.m_channel = value;
    m_settingsKeys.append("channel");
    sendSettings();
}

void VRTZMQGui::on_UsePort_stateChanged(int state)
{
    m_settings.m_useport = (state == Qt::Checked);
    m_settingsKeys.append("useport");
    sendSettings();
}

void VRTZMQGui::makeUIConnections() const
{
    QObject::connect(ui->centerFrequency, &ValueDial::changed, this, &VRTZMQGui::on_centerFrequency_changed);
    QObject::connect(ui->sampleRate, &ValueDial::changed, this, &VRTZMQGui::on_sampleRate_changed);
    QObject::connect(ui->HostName, &QLineEdit::editingFinished, this, &VRTZMQGui::on_HostName_editingFinished);
    QObject::connect(ui->Port, QOverload<int>::of(&QSpinBox::valueChanged), this, &VRTZMQGui::on_Port_valueChanged);
    QObject::connect(ui->Instance, QOverload<int>::of(&QSpinBox::valueChanged), this, &VRTZMQGui::on_Instance_valueChanged);
    QObject::connect(ui->Channel, QOverload<int>::of(&QSpinBox::valueChanged), this, &VRTZMQGui::on_Channel_valueChanged);
    QObject::connect(ui->Port, QOverload<int>::of(&QSpinBox::valueChanged), this, &VRTZMQGui::on_Port_valueChanged);
    QObject::connect(ui->UsePort, &QCheckBox::stateChanged, this, &VRTZMQGui::on_UsePort_stateChanged);
    QObject::connect(ui->iqImbalance, &ButtonSwitch::toggled, this, &VRTZMQGui::on_iqImbalance_toggled);
    QObject::connect(ui->decim, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VRTZMQGui::on_decim_currentIndexChanged);
    QObject::connect(ui->fcPos, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VRTZMQGui::on_fcPos_currentIndexChanged);
    QObject::connect(ui->startStop, &ButtonSwitch::toggled, this, &VRTZMQGui::on_startStop_toggled);
}
