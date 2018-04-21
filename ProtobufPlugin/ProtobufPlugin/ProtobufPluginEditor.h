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


#ifndef __PROTOBUFPLUGINEDITOR_H_D6EC8B49__
#define __PROTOBUFPLUGINEDITOR_H_D6EC8B49__

#include <EditorHeaders.h>

class ProtobufPlugin;


/**

  User interface for the "ProtobufPlugin" source node.

  @see SourceNode

*/

class ProtobufPluginEditor : public GenericEditor,public Label::Listener
{
public:
	ProtobufPluginEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors);
	virtual ~ProtobufPluginEditor();

    void buttonEvent(Button* button);
	void labelTextChanged(juce::Label *);
	void setLabelColor(juce::Colour color);
private:

	ScopedPointer<UtilityButton> restartConnection;
    ScopedPointer<Label> urlLabel;
	ScopedPointer<Label> labelPort;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProtobufPluginEditor);

};



#endif  // __PROTOBUFPLUGINEDITOR_H_D6EC8B49__
