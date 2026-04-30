///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014-2015 John Greb <hexameron@spam.no>                         //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2018 beta-tester <alpha-beta-release@gmx.net>                   //
// Copyright (C) 2022-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#include <string.h>
#include <errno.h>

#include <QDebug>
#include <QNetworkReply>
#include <QBuffer>

#include "vrtzmqinput.h"
#include "device/deviceapi.h"
#include "vrtzmqthread.h"
#include "dsp/dspcommands.h"
#ifdef ANDROID
#include "util/android.h"
#endif

MESSAGE_CLASS_DEFINITION(VRTZMQInput::MsgConfigureVRTZMQ, Message)
MESSAGE_CLASS_DEFINITION(VRTZMQInput::MsgStartStop, Message)

VRTZMQInput::VRTZMQInput(DeviceAPI *deviceAPI) :
    m_deviceAPI(deviceAPI),
	m_settings(),
	m_VRTZMQThread(nullptr),
	m_deviceDescription("VRTZMQ"),
	m_running(false)
{
    m_sampleFifo.setLabel(m_deviceDescription);
    openDevice();

    m_deviceAPI->setNbSourceStreams(1);
}

VRTZMQInput::~VRTZMQInput()
{
    qDebug("VRTZMQInput::~VRTZMQInput");
  
    if (m_running) {
        stop();
    }

    closeDevice();
}

void VRTZMQInput::destroy()
{
    delete this;
}

bool VRTZMQInput::openDevice()
{

    if (!m_sampleFifo.setSize(20000000)) // ???
    {
        qCritical("VRTZMQInput::openDevice: Could not allocate SampleFifo");
        return false;
    }

    m_sampleFifo.setWrittenSignalRateDivider(32); // ???

    // VRT
    context = zmq_ctx_new();

    return true;
}

void VRTZMQInput::init()
{
    applySettings(m_settings, QList<QString>(), true);
}

bool VRTZMQInput::start()
{
	QMutexLocker mutexLocker(&m_mutex);

    if (m_running) {
        return true;
    }

    uint16_t main_port;
    if (m_settings.m_useport) 
        main_port = m_settings.m_port;
    else
        main_port = DEFAULT_MAIN_PORT + MAX_CHANNELS*m_settings.m_instance;

    int hwm = 10000;
    subscriber = zmq_socket(context, ZMQ_SUB);
    int rc = zmq_setsockopt (subscriber, ZMQ_RCVHWM, &hwm, sizeof hwm);
    assert(rc == 0);

	std::string connect_string = "tcp://" + m_settings.m_host.toStdString() + ":" + std::to_string(main_port);
    qDebug("VRTZMQInput::start ZMQ connect string: %s", connect_string.c_str());
    rc = zmq_connect(subscriber, connect_string.c_str());
    assert(rc == 0);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    m_VRTZMQThread = new VRTZMQThread(m_deviceAPI, subscriber, &m_sampleFifo, m_settings);
    connect(m_VRTZMQThread, &QThread::finished, m_VRTZMQThread, &QObject::deleteLater);
	m_VRTZMQThread->startWork();
	
    m_running = true;

	mutexLocker.unlock();

	applySettings(m_settings, QList<QString>(), true);

    qDebug("VRTZMQInput::start");

	return true;
}

void VRTZMQInput::closeDevice()
{
    // VRT
    zmq_close(subscriber);
    zmq_ctx_destroy(context);

    m_deviceDescription.clear();
}

void VRTZMQInput::stop()
{
	QMutexLocker mutexLocker(&m_mutex);

    if (!m_running) {
        return;
    }

    zmq_close(subscriber);

    qDebug("VRTZMQInput::stop");

	if (m_VRTZMQThread)
	{
		m_VRTZMQThread->stopWork();
		m_VRTZMQThread = nullptr; // Deleted automatically when thread finishes
	}

	m_running = false;
}

QByteArray VRTZMQInput::serialize() const
{
    return m_settings.serialize();
}

bool VRTZMQInput::deserialize(const QByteArray& data)
{
    bool success = true;

    if (!m_settings.deserialize(data))
    {
        m_settings.resetToDefaults();
        success = false;
    }

    MsgConfigureVRTZMQ* message = MsgConfigureVRTZMQ::create(m_settings, QList<QString>(), true);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue)
    {
        MsgConfigureVRTZMQ* messageToGUI = MsgConfigureVRTZMQ::create(m_settings, QList<QString>(), true);
        m_guiMessageQueue->push(messageToGUI);
    }

    return success;
}

const QString& VRTZMQInput::getDeviceDescription() const
{
	return m_deviceDescription;
}

int VRTZMQInput::getSampleRate() const
{
	return (0);
}

quint64 VRTZMQInput::getCenterFrequency() const
{
	return 0;
}

void VRTZMQInput::setCenterFrequency(qint64 centerFrequency)
{
   
}

bool VRTZMQInput::handleMessage(const Message& message)
{
    if (MsgConfigureVRTZMQ::match(message))
    {
        auto& conf = (const MsgConfigureVRTZMQ&) message;
        qDebug() << "VRTZMQInput::handleMessage: MsgConfigureVRTZMQ";

        bool success = applySettings(conf.getSettings(), conf.getSettingsKeys(), conf.getForce());

        if (!success)
        {
            qDebug("VRTZMQInput::handleMessage: config error");
        }

        return true;
    }
    else if (MsgStartStop::match(message))
    {
        auto& cmd = (const MsgStartStop&) message;
        qDebug() << "VRTZMQInput::handleMessage: MsgStartStop: " << (cmd.getStartStop() ? "start" : "stop");

        if (cmd.getStartStop())
        {
            if (m_deviceAPI->initDeviceEngine()) {
                m_deviceAPI->startDeviceEngine();
            }
        }
        else
        {
            m_deviceAPI->stopDeviceEngine();
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool VRTZMQInput::applySettings(const VRTZMQSettings& settings, const QList<QString>& settingsKeys, bool force)
{
    qDebug() << "VRTZMQInput::applySettings: force: " << force << settings.getDebugString(settingsKeys, force);
    bool forwardChange = false;

    if (settingsKeys.contains("dcBlock") || settingsKeys.contains("iqImbalance") || force)
    {
        m_deviceAPI->configureCorrections(settings.m_dcBlock, settings.m_iqImbalance);
        qDebug("VRTZMQInput::applySettings: corrections: DC block: %s IQ imbalance: %s",
                settings.m_dcBlock ? "true" : "false",
                settings.m_iqImbalance ? "true" : "false");
    }

    if (m_VRTZMQThread)
    {
        MsgConfigureVRTZMQ* message = MsgConfigureVRTZMQ::create(settings, settingsKeys, force);
        m_VRTZMQThread->getInputMessageQueue()->push(message);
    }

    if (force) {
        m_settings = settings;
    } else {
        m_settings.applySettings(settingsKeys, settings);
    }

    // if (forwardChange)
    // {
    //     int sampleRate = m_settings.m_devSampleRate/(1<<m_settings.m_log2Decim);
    //     auto *notif = new DSPSignalNotification(sampleRate, m_settings.m_centerFrequency);
    //     m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);
    // }

    return true;
}
