/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

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

#include "ProtobufPluginEditor.h"
#include "ProtobufPlugin.h"

#include <stdio.h>

ProtobufPluginEditor::ProtobufPluginEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors = true)
    : GenericEditor(parentNode, useDefaultParameterEditors)

{
	desiredWidth = 180;

	ProtobufPlugin *p = (ProtobufPlugin *)getProcessor();

    portLabel = new Label("Port", "Port:");
	portLabel->setBounds(20, 95, 140, 25);
	addAndMakeVisible(portLabel);
	
	urlLabel = new Label("URL", "URL:");
	urlLabel->setBounds(20,65, 140, 25);
	addAndMakeVisible(urlLabel);

	restartConnection = new UtilityButton("Restart Connection",Font("Default", 15, Font::plain));
    restartConnection->setBounds(20,30,150,18);
    restartConnection->addListener(this);
    addAndMakeVisible(restartConnection);

	portEditor = new Label("Port", String(p->urlport));
	portEditor->setBounds(70, 95, 80, 18);
	portEditor->setFont(Font("Default", 15, Font::plain));
	portEditor->setColour(Label::textColourId, Colours::white);
	portEditor->setColour(Label::backgroundColourId, Colours::grey);
	portEditor->setEditable(true);
	portEditor->addListener(this);
	addAndMakeVisible(portEditor);

	urlEditor = new Label("URL", String(p->url));
	urlEditor->setBounds(70, 65, 100, 18);
	urlEditor->setFont(Font("Default", 15, Font::plain));
	urlEditor->setColour(Label::textColourId, Colours::white);
	urlEditor->setColour(Label::backgroundColourId, Colours::grey);
	urlEditor->setEditable(true);
	urlEditor->addListener(this);
	addAndMakeVisible(urlEditor);

    setEnabledState(false);

}

void ProtobufPluginEditor::refreshValues()
{
	ProtobufPlugin *p = (ProtobufPlugin *)getProcessor();
	urlEditor->setText(String(p->url), dontSendNotification);
	portEditor->setText(String(p->urlport), dontSendNotification);
}



void ProtobufPluginEditor::buttonEvent(Button* button)
{
	if (button == restartConnection)
	{
		ProtobufPlugin *p = (ProtobufPlugin *)getProcessor();
		p->setNewListeningPort(p->urlport);
	}

}

void ProtobufPluginEditor::setLabelColor(juce::Colour color)
{
	urlEditor->setColour(Label::backgroundColourId, color);
	portEditor->setColour(Label::backgroundColourId, color);
}


void ProtobufPluginEditor::labelTextChanged(juce::Label *label)
{
	if (label == portEditor)
	{
	   Value val = label->getTextValue();

		ProtobufPlugin *p= (ProtobufPlugin *)getProcessor();
		p->setNewListeningPort(val.getValue());
	}

	else if (label == urlEditor)
	{
		Value val = label->getTextValue();

		ProtobufPlugin *p = (ProtobufPlugin *)getProcessor();
		p->setNewListeningUrl(val.getValue());
	}
}


ProtobufPluginEditor::~ProtobufPluginEditor()
{

}


