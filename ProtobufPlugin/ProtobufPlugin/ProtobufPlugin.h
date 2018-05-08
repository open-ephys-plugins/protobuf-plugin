/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __PROTOBUFPLUGIN_H_91811541__
#define __PROTOBUFPLUGIN_H_91811541__

#include "resources/zmq.h"
#include "resources/zmq_utils.h"

#include <ProcessorHeaders.h>

#include "resources/ephys_edi.pb.h"

#include <list>
#include <queue>

/**
 Sends incoming TCP/IP messages from 0MQ to the events buffer

  @see GenericProcessor
*/
class ProtobufPlugin : public GenericProcessor
                    , public Thread
{
public:
	ProtobufPlugin();
	~ProtobufPlugin();

    // GenericProcessor methods
    // =========================================================================
    AudioProcessorEditor* createEditor() override;

    void process (AudioSampleBuffer& buffer) override;

    void setParameter (int parameterIndex, float newValue) override;

    void createEventChannels() override;

    void setEnabledState (bool newState) override;

    void saveCustomParametersToXml (XmlElement* parentElement) override;
    void loadCustomParametersFromXml() override;

    bool isReady() override;

    float getDefaultSampleRate() const override;
    float getDefaultBitVolts()   const override;

    // =========================================================================

    int getDefaultNumOutputs() const;


    String handleMessage(String msg);
    std::vector<String> splitString (String S, char sep);

    void run();
    void opensocket();
    bool closesocket();
    void setNewListeningPort (int port);
	void setNewListeningUrl(String url);

    int urlport;
	String url;
    String socketStatus;
    std::atomic<bool> threadRunning;

	register_for_message message;


private:
    void createZmqContext();

    //* Split network message into name/value pairs (name1=val1 name2=val2 etc) */
    StringPairArray parseNetworkMessage (String msg);

    static void* zmqcontext;
    void* router;
    bool state;
    bool shutdown;
    bool firstTime;

	void register_for_msg(String message_id);
	void handle_msg(std::string msg, String message_id);
	void send_multipart_msg(std::string part1, std::string part2, std::string part3);
	void generate_msg_header(message_header* header, String id);

    Time timer;

    std::queue<String> networkMessagesQueue;
    CriticalSection lock;
 
	const EventChannel* messageChannel{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProtobufPlugin);
};

#endif  // __PROTOBUFPLUGIN_H_91811541__
