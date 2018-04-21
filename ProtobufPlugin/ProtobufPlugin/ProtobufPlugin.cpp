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

#include <stdio.h>
#include <string.h>
#include "ProtobufPlugin.h"
#include "ProtobufPluginEditor.h"


const int MAX_MESSAGE_LENGTH = 64000;


#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif



/*********************************************/
void* ProtobufPlugin::zmqcontext = nullptr;

ProtobufPlugin::ProtobufPlugin()
    : GenericProcessor  ("Protobuf Module")
    , Thread            ("ProtobufThread")
    , state             (false)
{
    setProcessorType (PROCESSOR_TYPE_SOURCE);

    createZmqContext();

    firstTime = true;
    responder = nullptr;
    urlport = 5556;
    threadRunning = false;

    opensocket();

    sendSampleCount = false; // disable updating the continuous buffer sample counts,
    // since this processor only sends events
    shutdown = false;
}


void ProtobufPlugin::setNewListeningPort(int port)
{
    // first, close existing thread.
    closesocket();

    // allow some time for thread to quit
#ifdef WIN32
    Sleep (300);
#else
    usleep (300 * 1000);
#endif

    urlport = port;
    opensocket();
}


ProtobufPlugin::~ProtobufPlugin()
{
    shutdown = true;
    closesocket();
}


bool ProtobufPlugin::closesocket()
{
    std::cout << "Disabling network node" << std::endl;

    if (threadRunning)
    {
		lock.enter();
        zmq_close (responder);
        zmq_ctx_destroy (zmqcontext); // this will cause the thread to exit
        zmqcontext = nullptr;
		lock.exit();

		if (!stopThread(500))
		{
			std::cerr << "Network thread timeout. Forcing thread termination, system could be lefr in an unstable state" << std::endl;
		}

        if (! shutdown)
            createZmqContext();// and this will take care that processor graph doesn't attempt to delete the context again
	
    }

    return true;
}


void ProtobufPlugin::createEventChannels()
{
	//EventChannel* chan = new EventChannel(EventChannel::TEXT, 1, MAX_MESSAGE_LENGTH, CoreServices::getGlobalSampleRate(), this);
	//chan->setName("Network messages");
	//chan->setDescription("Messages received through the network events module");
	//chan->setIdentifier("external.network.rawData");
	//chan->addEventMetaData(new MetaDataDescriptor(MetaDataDescriptor::INT64, 1, "Software timestamp",
	//	"OS high resolution timer count when the event was received", "timestamp.software"));
	//eventChannelArray.add(chan);
	//messageChannel = chan;
}


AudioProcessorEditor* ProtobufPlugin::createEditor()
{
    editor = new ProtobufPluginEditor (this, true);

    return editor;
}


void ProtobufPlugin::setParameter(int parameterIndex, float newValue)
{
    /*
       editor->updateParameterButtons(parameterIndex);

       Parameter& p =  parameters.getReference(parameterIndex);
       p.setValue(newValue, 0);

       threshold = newValue;
    */
    //std::cout << float(p[0]) << std::endl;
}


String ProtobufPlugin::handleMessage(String msg)
{
	/**

    String s = msg.getString();

    StringArray inputs = StringArray::fromTokens (s, " ");
    String cmd = String (inputs[0]);

    const MessageManagerLock mmLock;
    if (cmd.compareIgnoreCase ("StartAcquisition") == 0)
    {
        if (! CoreServices::getAcquisitionStatus())
        {
            CoreServices::setAcquisitionStatus (true);
        }
        return String ("StartedAcquisition");
    }
    else if (cmd.compareIgnoreCase ("StopAcquisition") == 0)
    {
        if (CoreServices::getAcquisitionStatus())
        {
            CoreServices::setAcquisitionStatus (false);
        }
        return String ("StoppedAcquisition");
    }
    else if (String ("StartRecord").compareIgnoreCase (cmd) == 0)
    {
        if (! CoreServices::getRecordingStatus() 
            && CoreServices::getAcquisitionStatus())
        {
            if (s.contains ("="))
            {
                String params = s.substring (cmd.length());
                StringPairArray dict = parseNetworkMessage (params);

                StringArray keys = dict.getAllKeys();
                for (int i = 0; i < keys.size(); ++i)
                {
                    String key   = keys[i];
                    String value = dict[key];

                    if (key.compareIgnoreCase ("CreateNewDir") == 0)
                    {
                        if (value.compareIgnoreCase ("1") == 0)
                        {
                            CoreServices::createNewRecordingDir();
                        }
                    }
                    else if (key.compareIgnoreCase ("RecDir") == 0)
                    {
                        CoreServices::setRecordingDirectory (value);
                    }
                    else if (key.compareIgnoreCase ("PrependText") == 0)
                    {
                        CoreServices::setPrependTextToRecordingDir (value);
                    }
                    else if (key.compareIgnoreCase ("AppendText") == 0)
                    {
                        CoreServices::setAppendTextToRecordingDir (value);
                    }
                }
            }

 
            CoreServices::setRecordingStatus (true);
            return String ("StartedRecording");
        }
    }
    else if (String ("StopRecord").compareIgnoreCase (cmd) == 0)
    {
        if (CoreServices::getRecordingStatus())
        {
            CoreServices::setRecordingStatus (false);
            return String ("StoppedRecording");
        }
    }
    else if (cmd.compareIgnoreCase ("IsAcquiring") == 0)
    {
        String status = CoreServices::getAcquisitionStatus() ? String ("1") : String ("0");
        return status;
    }
    else if (cmd.compareIgnoreCase ("IsRecording") == 0)
    {
        String status = CoreServices::getRecordingStatus() ? String ("1") : String ("0");
        return status;
    }
    else if (cmd.compareIgnoreCase ("GetRecordingPath") == 0)
    {
        File file = CoreServices::RecordNode::getRecordingPath();
        String msg (file.getFullPathName());
        return msg;
    }
    else if (cmd.compareIgnoreCase ("GetRecordingNumber") == 0)
    {
        String status;
        status += (CoreServices::RecordNode::getRecordingNumber() + 1);
        return status;
    }
    else if (cmd.compareIgnoreCase ("GetExperimentNumber") == 0)
    {
        String status;
        status += CoreServices::RecordNode::getExperimentNumber();
        return status;
	}

	std::string cmd2 = cmd.toStdString();

	request_system_status rss;
	if (rss.ParseFromString(cmd2))
	{
		// do the thing
	}

	*/
    return String ("NotHandled");
}


void ProtobufPlugin::process(AudioSampleBuffer& buffer)
{
    setTimestampAndSamples(CoreServices::getGlobalTimestamp(),0);

    lock.enter();
    //while (! networkMessagesQueue.empty())
    //{
    //    String msg = networkMessagesQueue.front();
    //    postTimestamppedStringToMidiBuffer (msg);
        
    //    networkMessagesQueue.pop();
    //}

    lock.exit();
}


void ProtobufPlugin::opensocket()
{
    startThread();
}


void ProtobufPlugin::run()
{

    responder = zmq_socket (zmqcontext, ZMQ_REP);
    String url= String ("tcp://*:") + String (urlport);
    int rc = zmq_bind (responder, url.toRawUTF8());

    if (rc != 0)
    {
        // failed to open socket?
        std::cout << "Failed to open socket: " << zmq_strerror (zmq_errno()) << std::endl;
        return;
    }

    threadRunning = true;
    unsigned char* buffer = new unsigned char[MAX_MESSAGE_LENGTH];
    int result = -1;

    while (threadRunning)
    {
        result = zmq_recv (responder, buffer, MAX_MESSAGE_LENGTH - 1, 0);  // blocking

        if (result < 0) // will only happen when responder dies.
            break;

		//String msg(buffer);

        if (result > 0)
        {
			//handleMessage(msg);

			CoreServices::sendStatusMessage("Network event received");

            //lock.enter();
            //networkMessagesQueue.push (Msg);
            //lock.exit();

            //std::cout << "Received message!" << std::endl;
            // handle special messages
            //String response = handleSpecialMessages (Msg);
			String response = "hi.";

            zmq_send (responder, response.getCharPointer(), response.length(), 0);
        }
        else
        {
            String zeroMessageError = "Recieved Zero Message?!?!?";
            //std::cout << "Received Zero Message!" << std::endl;

            //zmq_send (responder, zeroMessageError.getCharPointer(), zeroMessageError.length(), 0);
        }
    }

    zmq_close (responder);

    delete[] buffer;
    threadRunning = false;

    return;
}


int ProtobufPlugin::getDefaultNumOutputs() const
{
    return 0;
}

bool ProtobufPlugin::isReady()
{
    return true;
}


float ProtobufPlugin::getDefaultSampleRate() const
{
    return 30000.0f;
}


float ProtobufPlugin::getDefaultBitVolts() const
{
    return 0.05f;
}

void ProtobufPlugin::setEnabledState(bool newState)
{
    isEnabled = newState;
}


void ProtobufPlugin::saveCustomParametersToXml(XmlElement* parentElement)
{
    XmlElement* mainNode = parentElement->createNewChildElement ("NETWORKEVENTS");
    mainNode->setAttribute ("port", urlport);
}


void ProtobufPlugin::loadCustomParametersFromXml()
{
    if (parametersAsXml != nullptr)
    {
        forEachXmlChildElement (*parametersAsXml, mainNode)
        {
            if (mainNode->hasTagName ("NETWORKEVENTS"))
            {
                setNewListeningPort (mainNode->getIntAttribute("port"));
            }
        }
    }
}


void ProtobufPlugin::createZmqContext()
{
	lock.enter();
    if (zmqcontext == nullptr)
        zmqcontext = zmq_ctx_new(); //<-- this is only available in version 3+
	lock.exit();
}


StringPairArray ProtobufPlugin::parseNetworkMessage(String msg)
{
    StringArray splitted;
    splitted.addTokens (msg, "=", "");

    StringPairArray dict = StringPairArray();
    String key   = "";
    String value = "";

    for (int i = 0; i < splitted.size() - 1; ++i)
    {
        String s1 = splitted[i];
        String s2 = splitted[i + 1];

        /** Get key */
        if (! key.isEmpty())
        {
            if (s1.contains (" "))
            {
                int i1 = s1.lastIndexOf (" ");
                key = s1.substring (i1 + 1);
            }
            else
            {
                key = s1;
            }
        }
        else
        {
            key = s1.trim();
        }

        /** Get value */
        if (i < splitted.size() - 2)
        {
            int i1 = s2.lastIndexOf (" ");
            value = s2.substring (0, i1);
        }
        else
        {
            value = s2;
        }

        dict.set (key, value);
    }

    return dict;
}
