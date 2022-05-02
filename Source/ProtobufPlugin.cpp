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
    #include <process.h>
#else
    #include <unistd.h>
#endif


/*********************************************/
void* ProtobufPlugin::zmqcontext = nullptr;

ProtobufPlugin::ProtobufPlugin()
    : GenericProcessor  ("Protobuf Module")
    , Thread            ("ProtobufThread")
{

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	// # self.context = zmq.Context()
    createZmqContext();

    firstTime = true;
    router = nullptr;
    urlport = 9928;
	url = "10.128.50.68";
    threadRunning = false;

    sendSampleCount = false; // disable updating the continuous buffer sample counts,
    // since this processor only sends events
    shutdown = false;
    
    isEnabled = false;
}

int ProtobufPlugin::getListeningPort ()
{
    return urlport;
}

String ProtobufPlugin::getListeningUrl ()
{
    return url;
}

void ProtobufPlugin::setNewListeningPort(int port)
{
    // first, close existing thread.
    bool shutdown_ok = closesocket();

    // allow some time for thread to quit

	if (shutdown_ok)
	{
#ifdef WIN32
		Sleep(300);
#else
		usleep(300 * 1000);
#endif
		urlport = port;
		opensocket();

		ProtobufPluginEditor* e = (ProtobufPluginEditor*)getEditor();
		e->refreshValues();
	}

}

void ProtobufPlugin::setNewListeningUrl(String _url)
{
	// first, close existing thread.
	bool shutdown_ok = closesocket();

	if (shutdown_ok)
	{
	// allow some time for thread to quit
#ifdef WIN32
	Sleep(300);
#else
	usleep(300 * 1000);
#endif

	url = _url;
	opensocket();

	ProtobufPluginEditor* e = (ProtobufPluginEditor*)getEditor();
	e->refreshValues();
	}
}


ProtobufPlugin::~ProtobufPlugin()
{
    shutdown = true;
    closesocket();
}


bool ProtobufPlugin::closesocket()
{
    std::cout << "Closing socket" << std::endl;

    if (threadRunning)
    {
		if (!stopThread(500))
		{
			std::cerr << "Failed to stop thread." << std::endl;
			return false;
		}
		else {
			std::cout << "Successfully shut down thread" << std::endl;

			if (shutdown)
			{
				std::cout << "Destroying context" << std::endl;
				zmq_close(router);
				zmq_ctx_destroy(zmqcontext);
			}
		}
    }

    return true;
}


AudioProcessorEditor* ProtobufPlugin::createEditor()
{
    
    editor = std::make_unique<ProtobufPluginEditor> (this);

    return editor.get();

}


void ProtobufPlugin::opensocket()
{
	std::cout << "Opening socket." << std::endl;
    
	if (!threadRunning)
		startThread();
}

void ProtobufPlugin::register_for_msg(String msg_id)
{
	// # io.register_for_message('request_system_status', handle_system_status)

	message_header* header = message.mutable_header(); // = message.header;
	generate_msg_header(header, msg_id);
	
	//message.set_allocated_header(header);
	message.set_message_id(msg_id.toStdString());

	std::cout << "  " << message.message_id() << std::endl;
	
	std::string resp1 = String("router").toStdString();
	std::string message_id = String("register_for_message").toStdString();
	std::string message_string = message.SerializeAsString();

	send_multipart_msg(resp1, message_id, message_string);

	

}

void ProtobufPlugin::send_multipart_msg(std::string part1, std::string part2, std::string part3)
{
	int rc = zmq_send(router, part1.c_str(), part1.length(), ZMQ_SNDMORE);
	rc = zmq_send(router, part2.c_str(), part2.length(), ZMQ_SNDMORE);
	rc = zmq_send(router, part3.c_str(), part3.length(), 0);

#ifdef WIN32
	Sleep(20);
#else
	usleep(20 * 1000);
#endif
}

void ProtobufPlugin::generate_msg_header(message_header* header, String id)
{
	header->set_host(SystemStats::getComputerName().getCharPointer());
	header->set_process(String("Open_Ephys").getCharPointer());
	header->set_timestamp(float(Time::currentTimeMillis()));
	header->set_message_id(id.toStdString());
}

void ProtobufPlugin::handle_msg(std::string msg, String message_id)
{
	std::cout << "Handling that message." << std::endl;

	//String message_id;

	if (message_id.equalsIgnoreCase("request_system_info"))
	{
		request_system_info rsi;
		rsi.ParseFromString(msg);

		String id = String("system_info");
		CoreServices::sendStatusMessage(String("Message: request_system_info."));

		system_info info;
		std::string version_string = String("version string").toStdString();
		std::string revision_string = String("hi.").toStdString();
		info.set_software_revision(version_string.c_str());
		info.set_hardware_revision(revision_string.c_str());

		message_header* header = info.mutable_header();
		generate_msg_header(header, id);

		std::string resp1 = String("router").toStdString();
		std::string resp2 = id.toStdString();
		std::string message_string = info.SerializeAsString();

		send_multipart_msg(resp1, resp2, message_string);

	}
	else if (message_id.equalsIgnoreCase("request_system_status")) // request_system_status
	{
		request_system_status rss;
		rss.ParseFromString(msg);

		String id = String("system_status");

		CoreServices::sendStatusMessage(String("Message: request_system_status."));

		system_status status;
		status.set_status(system_status_status_type_READY);

		message_header* header = status.mutable_header();
		generate_msg_header(header, id);

		std::string resp1 = String("router").toStdString();
		std::string resp2 = id.toStdString();
		std::string message_string = status.SerializeAsString();

		send_multipart_msg(resp1, resp2, message_string);
	}
	else if (message_id.equalsIgnoreCase("acquisition")) // acquisition
	{

		acquisition acq;
		acq.ParseFromString(msg);

		if (acq.command() == 0)
		{
			// stop acquisition
			CoreServices::sendStatusMessage(String("Message: stop acquisition."));
			CoreServices::setAcquisitionStatus(false);
		}
		else {
			// start acquisition
			CoreServices::sendStatusMessage(String("Message: start acquisition."));
			CoreServices::setAcquisitionStatus(true);
		}
	}
	else if (message_id.equalsIgnoreCase("recording")) // recording
	{

		recording rec;
		rec.ParseFromString(msg);

		if (rec.command() == 0)
		{
			// stop recording
			CoreServices::sendStatusMessage(String("Message: stop recording."));
			CoreServices::setRecordingStatus(false);
			//return String("StartedRecording");
		}
		else {
			CoreServices::sendStatusMessage(String("Message: start recording."));
			CoreServices::setRecordingStatus(true);
			// start recording
		}
	}
	else if (message_id.equalsIgnoreCase("set_data_file_path")) // set data file path
	{

		set_data_file_path sdfp;
		sdfp.ParseFromString(msg);

		CoreServices::sendStatusMessage(String("Message: set_data_file_path."));
		String path = String(sdfp.path());
		
        CoreServices::setRecordingDirectoryPrependText(path);
		CoreServices::createNewRecordingDirectory();

	}
	else {
		CoreServices::sendStatusMessage(String("Message: not recognized."));
	}

}

void ProtobufPlugin::run()
{
	// # io = ZMQHandler(messages)
	// 
    router = zmq_socket (zmqcontext, ZMQ_ROUTER);
	int timeoutvalue = 100;
	int probe_router = 1;
	int lingervalue = 0;
    String identitystring = String("OpenEphys_") + SystemStats::getComputerName();
#ifdef WIN32
    identitystring += "_" + String(_getpid());
#endif
	std::string identity = identitystring.toStdString();
	zmq_setsockopt(router, ZMQ_RCVTIMEO, &timeoutvalue, sizeof timeoutvalue);
	zmq_setsockopt(router, ZMQ_IDENTITY, identity.c_str(), identity.length());
	zmq_setsockopt(router, ZMQ_PROBE_ROUTER, &probe_router, sizeof probe_router);
	zmq_setsockopt(router, ZMQ_LINGER, &lingervalue, sizeof lingervalue);
    String full_url = String ("tcp://") + String(url) + ":" + String (urlport);

	std::cout << "Connecting to " << full_url << std::endl;
    int rc = zmq_connect (router, full_url.toRawUTF8());

	if (rc != 0)
	{
		// failed to open socket?
		std::cout << "Failed to open socket: " << zmq_strerror(zmq_errno()) << std::endl;
		zmq_close(router);
		return;
	}

	// allow time for connection to register
#ifdef WIN32
	Sleep(500);
#else
	usleep(500 * 1000);
#endif

	std::cout << "Registering for messages:" << std::endl;
	register_for_msg("set_data_file_path");
	register_for_msg("request_system_info");
	register_for_msg("request_system_status");
	register_for_msg("acquisition");
	register_for_msg("recording");
    // # io.register_for_message('request_system_info', handle_system_info)
	// # io.register_for_message('set_data_file_path', handle_set_data_file_path)

	// set up polling
	zmq_pollitem_t item[2];
	item[0].socket = router;
	item[0].events = ZMQ_POLLIN;
	item[1].socket = router;
	item[1].events = ZMQ_POLLIN;

    threadRunning = true;
    unsigned char* buffer = new unsigned char[MAX_MESSAGE_LENGTH];
    int result = -1;

	while (!threadShouldExit())
	{
		int rc = zmq_poll(item, 2, 100);

		if (item[0].revents & ZMQ_POLLIN)
		{
			std::cout << "Got a message!" << std::endl;

			int messageNum = 0;
			String message_id;
			int64_t more = 0;
			size_t more_size = sizeof more;

			do {
				rc = zmq_recv(router, buffer, MAX_MESSAGE_LENGTH - 1, 0);
				
				std::string s(reinterpret_cast<char const*> (buffer));
				std::cout << "Received: " << std::endl;
				std::cout << s << std::endl;
				memset(buffer, 0, MAX_MESSAGE_LENGTH);
	
				//std::cout << messageNum << std::endl;
				//std::cout << s << std::endl;
				//std::cout << " " << std::endl;

				if (messageNum == 0) // client
				{
					
				}
				else if (messageNum == 1) // message_id
				{
					message_id = String(s);
				}
				else if (messageNum == 2)
				{
					handle_msg(s, message_id);
				}

				messageNum++;

				rc = zmq_getsockopt(router, ZMQ_RCVMORE, &more, &more_size);

			} while (more);

		}
    }

    //zmq_close (router);

    delete[] buffer;
    threadRunning = false;

	zmq_close(router);

    return;
}




void ProtobufPlugin::saveCustomParametersToXml(XmlElement* xml)
{
    xml->setAttribute ("port", urlport);
    xml->setAttribute("url", url);
}


void ProtobufPlugin::loadCustomParametersFromXml(XmlElement* xml)
{
    url = xml->getStringAttribute("url");
    setNewListeningPort (xml->getIntAttribute("port"));
}

void ProtobufPlugin::process(AudioBuffer<float>& buffer)
{

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
