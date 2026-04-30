///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015, 2017-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
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

#include <QtGlobal>
#include "util/simpleserializer.h"
#include "vrtzmqsettings.h"

VRTZMQSettings::VRTZMQSettings()
{
	resetToDefaults();
}

void VRTZMQSettings::resetToDefaults()
{
	m_log2Decim = 0;
	m_fcPos = FC_POS_CENTER;
    m_title = "VRT-ZMQ";
    m_host = "127.0.0.1";
    m_instance = 0;
    m_channel = 0;
    m_port = 50100;
    m_useport = false;

    m_dcBlock = false;
    m_iqImbalance = false;

}

QByteArray VRTZMQSettings::serialize() const
{
	SimpleSerializer s(1);

	s.writeU32(2, m_log2Decim);
	s.writeS32(3, (int) m_fcPos);
    s.writeString(4, m_title);
    s.writeString(5, m_host);
    s.writeU32(6, m_instance);
    s.writeU32(7, m_channel);
    s.writeU32(8, m_port);
    s.writeBool(9, m_useport);
    s.writeBool(10, m_dcBlock);
    s.writeBool(11, m_iqImbalance);

	return s.final();
}

bool VRTZMQSettings::deserialize(const QByteArray& data)
{
	SimpleDeserializer d(data);

	if (!d.isValid())
	{
		resetToDefaults();
		return false;
	}

	if (d.getVersion() == 1)
	{
		int intval;

		d.readU32(2, &m_log2Decim, 0);
		d.readS32(3, &intval, 0);
		m_fcPos = (fcPos_t) intval;
        d.readString(4, &m_title, "VRT-ZMQ");
        d.readString(5, &m_host, "127.0.0.1");
        d.readU32(6, &m_instance, 0);
        d.readU32(7, &m_channel, 0);
        d.readU32(8, &m_port, 0);
        d.readBool(9, &m_useport, 0);
        d.readBool(10, &m_dcBlock, false);
        d.readBool(11, &m_iqImbalance, false);

		return true;
	}
	else
	{
		resetToDefaults();
		return false;
	}
}

void VRTZMQSettings::applySettings(const QStringList& settingsKeys, const VRTZMQSettings& settings)
{

    if (settingsKeys.contains("log2Decim")) {
        m_log2Decim = settings.m_log2Decim;
    }
    if (settingsKeys.contains("fcPos")) {
        m_fcPos = settings.m_fcPos;
    }
    if (settingsKeys.contains("title")) {
        m_title = settings.m_title;
    }
    if (settingsKeys.contains("host")) {
        m_host = settings.m_host;
    }
    if (settingsKeys.contains("instance")) {
        m_instance = settings.m_instance;
    }
    if (settingsKeys.contains("channel")) {
        m_channel = settings.m_channel;
    }
    if (settingsKeys.contains("port")) {
        m_port = settings.m_port;
    }
    if (settingsKeys.contains("useport")) {
        m_useport = settings.m_useport;
    }
    if (settingsKeys.contains("dcBlock")) {
        m_dcBlock = settings.m_dcBlock;
    }
    if (settingsKeys.contains("iqImbalance")) {
        m_iqImbalance = settings.m_iqImbalance;
    }

}

QString VRTZMQSettings::getDebugString(const QStringList& settingsKeys, bool force) const
{
    std::ostringstream ostr;

    if (settingsKeys.contains("title") || force) {
        ostr << " m_title: " << m_title.toStdString();
    }
    if (settingsKeys.contains("log2Decim") || force) {
        ostr << " m_log2Decim: " << m_log2Decim;
    }
    if (settingsKeys.contains("fcPos") || force) {
        ostr << " m_fcPos: " << m_fcPos;
    }
    if (settingsKeys.contains("host") || force) {
        ostr << " m_host: " << m_host.toStdString();
    }
    if (settingsKeys.contains("instance") || force) {
        ostr << " m_instance: " << m_instance;
    }
    if (settingsKeys.contains("channel") || force) {
        ostr << " m_channel: " << m_channel;
    }
    if (settingsKeys.contains("port") || force) {
        ostr << " m_port: " << m_port;
    }
    if (settingsKeys.contains("useport") || force) {
        ostr << " m_useport: " << m_useport;
    }
    if (settingsKeys.contains("dcBlock") || force) {
        ostr << " m_dcBlock: " << m_dcBlock;
    }
    if (settingsKeys.contains("iqImbalance") || force) {
        ostr << " m_iqImbalance: " << m_iqImbalance;
    }

    return QString(ostr.str().c_str());
}
