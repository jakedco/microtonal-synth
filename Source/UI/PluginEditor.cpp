/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#pragma once
#include "../audioProcessor/PluginProcessor.h"
#include "PluginEditor.h"
#include "../components/instrumentPresets/PresetListBox.h"
#include "CustomLookAndFeel.h"
#include <string> 
#include <cctype> 
#include <math.h>
#include <iostream>
#include <algorithm>
using namespace std;

extern MicrotonalConfig microtonalMappings[7];
juce::String microtonalPresetNames[7] = { "Default", "1", "2", "3", "4", "5", "6" }, instrumentPresetNames[7] = { "Default", "<Instrument 1>", "<Instrument 2>", "<Instrument 3>", "<Instrument 4>", "<Instrument 5>", "<Instrument 6>" };


/*
*   Instruments work by saving ValueTree representation of your instrument XML file into this array.
*   Array holds 7 instrument presets at a time
*   Whenever you swap between instruments, you change the current instrument variable which functions as an index to access the correct instrument
*/
int mappingGroup = Default, currentInstrument = 0;
juce::ValueTree loadedInstruments[7];

MainContentComponent* createMainContentComponent(int index)
{
    return new MainContentComponent(index);
}

MicrotonalWindow::MicrotonalWindow(juce::String name, int index) : DocumentWindow(name,
    juce::Colours::dimgrey,
    DocumentWindow::closeButton | DocumentWindow::maximiseButton, true)
{
    double ratio = 2; // adjust as desired
    setContentOwned(new MainContentComponent(index), true);
    getConstrainer()->setFixedAspectRatio(ratio);
    centreWithSize(1300, 600);
    setResizable(true, true);
    setResizeLimits(800, 600/ratio, 1800, 1600/ratio);
    setVisible(true);
}

void MicrotonalWindow::closeButtonPressed()
{
    // juce::JUCEApplication::getInstance()->systemRequestedQuit();
    delete this;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    Synth::addADSRParameters(layout);
    Synth::addOvertoneParameters(layout);
    Synth::addGainParameters(layout);

    auto groupInstruments = std::make_unique<juce::AudioProcessorParameterGroup>("instruments", "Instruments", "|");
    groupInstruments->addChild(std::make_unique<juce::AudioParameterChoice>("instrumentPreset", "Instrument_Preset", juce::StringArray({ "preset342", "preset54" }), 0));
    layout.add(std::move(groupInstruments));

    return layout;
}

MicrotonalSynthAudioProcessorEditor::MicrotonalSynthAudioProcessorEditor()
    : foleys::MagicProcessor(juce::AudioProcessor::BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    treeState(*this, nullptr, ProjectInfo::projectName, createParameterLayout())
{
    FOLEYS_SET_SOURCE_PATH(__FILE__);

    float factor = 3.0f, phase = 0.0f;
    auto file = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
        .getChildFile("Contents")
        .getChildFile("Resources")
        .getChildFile("layout.xml");

    if (file.existsAsFile())
        magicState.setGuiValueTree(file);
    else
        magicState.setGuiValueTree(BinaryData::layout_xml, BinaryData::layout_xmlSize);

    // MAGIC GUI: add a meter at the output
    outputMeter = magicState.createAndAddObject<foleys::MagicLevelSource>("output");
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>("waveform");
    analyser = magicState.createAndAddObject<foleys::MagicAnalyser>("analyser");
    magicState.addBackgroundProcessing(analyser);
    
    /* START onClick methods */
    presetList = magicState.createAndAddObject<PresetListBox>("presets");
    presetList->onSelectionChanged = [&](int number){loadPresetInternal(number);};
    magicState.addTrigger("save-preset", [this]{savePresetInternal();});

    magicState.addTrigger("load-instrument-preset1", [this] {loadPresetInternal(1);});
    magicState.addTrigger("load-instrument-preset2", [this] {loadPresetInternal(2);});
    magicState.addTrigger("load-instrument-preset3", [this] {loadPresetInternal(3);});
    magicState.addTrigger("load-instrument-preset4", [this] {loadPresetInternal(4);});
    magicState.addTrigger("load-instrument-preset5", [this]{loadPresetInternal(5);});
    magicState.addTrigger("load-instrument-preset6", [this]{loadPresetInternal(6);});

    magicState.addTrigger("swap-instrument-1", [this] {loadHelper(1);});
    magicState.addTrigger("swap-instrument-2", [this] {loadHelper(2);});
    magicState.addTrigger("swap-instrument-3", [this] {loadHelper(3);});
    magicState.addTrigger("swap-instrument-4", [this] {loadHelper(4);});
    magicState.addTrigger("swap-instrument-5", [this] {loadHelper(5);});
    magicState.addTrigger("swap-instrument-6", [this] {loadHelper(6);});

    magicState.addTrigger("load-microtonal-preset1", [this] {loadMicrotonalPreset(1);});
    magicState.addTrigger("load-microtonal-preset2", [this] {loadMicrotonalPreset(2);});
    magicState.addTrigger("load-microtonal-preset3", [this] {loadMicrotonalPreset(3);});
    magicState.addTrigger("load-microtonal-preset4", [this] {loadMicrotonalPreset(4);});
    magicState.addTrigger("load-microtonal-preset5", [this] {loadMicrotonalPreset(5);});
    magicState.addTrigger("load-microtonal-preset6", [this] {loadMicrotonalPreset(6);});

    magicState.addTrigger("open-window1", [this] {if (activeWindow != 1) { delete window; } openWindow(1);});
    magicState.addTrigger("open-window2", [this] {if (activeWindow != 2) { delete window; } openWindow(2);});
    magicState.addTrigger("open-window3", [this] {if (activeWindow != 3) { delete window; } openWindow(3);});
    magicState.addTrigger("open-window4", [this] {if (activeWindow != 4) { delete window; } openWindow(4);});
    magicState.addTrigger("open-window5", [this] {if (activeWindow != 5) { delete window; } openWindow(5);});
    magicState.addTrigger("open-window6", [this] {if (activeWindow != 6) { delete window; } openWindow(6);});

    magicState.addTrigger("set-map1", [this] { mappingGroup = mappingGroup == Group1 ? Default : Group1;});
    magicState.addTrigger("set-map2", [this] {mappingGroup = mappingGroup == Group2 ? Default : Group2;});
    magicState.addTrigger("set-map3", [this] {mappingGroup = mappingGroup == Group3 ? Default : Group3;});
    magicState.addTrigger("set-map4", [this] {mappingGroup = mappingGroup == Group4 ? Default : Group4;});
    magicState.addTrigger("set-map5", [this] {mappingGroup = mappingGroup == Group5 ? Default : Group5;});
    magicState.addTrigger("set-map6", [this]{mappingGroup = mappingGroup == Group6 ? Default : Group6;});
    /* END onClick methods*/
    magicState.setApplicationSettingsFile(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName + juce::String(".settings")));
    magicState.setPlayheadUpdateFrequency(30);

    Synth::Sound::Ptr sound(new Synth::Sound(treeState));
    synthesiser.addSound(sound);
    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new Synth::Voice(treeState));
}


MicrotonalSynthAudioProcessorEditor::~MicrotonalSynthAudioProcessorEditor()
{
    if (window)
        delete window;
}
void MicrotonalSynthAudioProcessorEditor::prepareToPlay(double sampleRate, int blockSize)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // MAGIC GUI: setup the output meter
    outputMeter->setupSource(getTotalNumOutputChannels(), sampleRate, 500);
    oscilloscope->prepareToPlay(sampleRate, blockSize);
    analyser->prepareToPlay(sampleRate, blockSize);
}

void MicrotonalSynthAudioProcessorEditor::openWindow(int index)
{
    if (!window) {
        window = new MicrotonalWindow("Configure Microtonal Mapping Preset " + to_string(index), index);
        activeWindow = index;
    }
    else if (index != activeWindow) {
        delete window;
        new MicrotonalWindow("Configure Microtonal Mapping Preset " + to_string(index), index);
    }
}

void MicrotonalSynthAudioProcessorEditor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool MicrotonalSynthAudioProcessorEditor::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const
{
    // This synth only supports mono or stereo.
    return (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void MicrotonalSynthAudioProcessorEditor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // MAGIC GUI: send midi messages to the keyboard state and MidiLearn
    magicState.processMidiBuffer(midiMessages, buffer.getNumSamples(), true);

    // MAGIC GUI: send playhead information to the GUI
    magicState.updatePlayheadInformation(getPlayHead());

    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
	
    for (int i = 1; i < buffer.getNumChannels(); ++i)
        buffer.copyFrom(i, 0, buffer.getReadPointer(0), buffer.getNumSamples());

    // MAGIC GUI: send the finished buffer to the level meter
    outputMeter->pushSamples(buffer);
    oscilloscope->pushSamples(buffer);
    analyser->pushSamples(buffer);
}

//==============================================================================

//==============================================================================
void MicrotonalSynthAudioProcessorEditor::savePresetInternal()
{
    // Creates a reference to FoleyGuiMagic's state manager
    foleys::ParameterManager manager(*this);

    // Saves ALL the settings current set on the synthesizer (does not include microtonal settings) into a ValueTree
    juce::ValueTree instrument = magicState.getSettings().getOrCreateChildWithName("presets", nullptr);
    manager.saveParameterValues(instrument);

    // Creates a pointer to a file chooser and gives it the hostApplicationPath as a default path
    chooser = std::make_unique<juce::FileChooser>("Save an instrument preset", juce::File::getSpecialLocation(juce::File::hostApplicationPath).getParentDirectory(), "*xml", true, false);
    auto flags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::warnAboutOverwriting;
   
    // Opens up file window to save all user settings as an XML file
    chooser->launchAsync(flags, [this, instrument](const juce::FileChooser& fc) {
        if (fc.getResult() == juce::File{})
            return;
        juce::File myFile = fc.getResult().withFileExtension("xml");
        juce::String fileName = myFile.getFileName();
        /* Save file logic goes here*/
        if (!myFile.replaceWithText(instrument.toXmlString())) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                TRANS("Error whilst saving"),
                TRANS("Couldn't write to the specified file!")
            );
        }
     });
}

 void MicrotonalSynthAudioProcessorEditor::loadHelper(int swapTo)
{
     if (!loadedInstruments[swapTo].isValid())
     {
         DBG("INVALID INSTRUMENT");
         return;
     }

     // Create a reference to FGM's manager
    foleys::ParameterManager manager(*this);

    // Sets the index of the instrumentArray to be the selected instrument
    currentInstrument = swapTo;

    // Load the desired instrument into state
    manager.loadParameterValues(loadedInstruments[currentInstrument]);
}

void MicrotonalSynthAudioProcessorEditor::loadPresetInternal(int index)
{
    // choose a file
    chooser = std::make_unique<juce::FileChooser>("Load an instrument", juce::File::getSpecialLocation(juce::File::hostApplicationPath).getParentDirectory(), "*.xml", true, true);
    auto flags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this, index](const juce::FileChooser& fc) {
        if (fc.getResult() == juce::File{})
            return;
        juce::File myFile;

        myFile = fc.getResult();
        juce::String fileName = myFile.getFileName();
        instrumentPresetNames[index] = fileName;

        juce::XmlDocument doc(myFile.loadFileAsString());
        juce::XmlElement config = *doc.getDocumentElement();
        juce::ValueTree instrument;
        instrument = instrument.fromXml(config);

        loadedInstruments[index] = instrument;

        });
}


//==============================================================================

double MicrotonalSynthAudioProcessorEditor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicrotonalSynthAudioProcessorEditor();
}

class InstrumentPresetComponent : public juce::Component, private juce::Timer, public juce::Button::Listener
{
public:
    //==============================================================================
    InstrumentPresetComponent() {
        for (int i = 0; i < 6; i++) {
            addAndMakeVisible(btns[i]);
            /*btns[i].setTooltip(instrumentPresetNames[i + 1]);*/
            //btns[i].setEnabled(false);
            btns[i].addListener(this);
            btns[i].setMouseCursor(juce::MouseCursor::IBeamCursor);
            btns[i].setEnabled(false);
   
        }
        startTimerHz(30);
    };
    void setFactor(float f)
    {
        factor = f;
    }
    void resized() override {
        auto rect = getLocalBounds();
        rect.setHeight(rect.getHeight() / 6);
        for (int i = 0; i < 6; i++) {
            btns[i].setBounds(rect);
            rect.setY(rect.getY() + rect.getHeight());
        }
    }
    void paint(juce::Graphics& g) override {
        for (int i = 0; i < 6; i++) {
            if (i + 1 == currentInstrument && loadedInstruments[currentInstrument].isValid() ) {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
            }
            else if (i + 1 != currentInstrument && loadedInstruments[i + 1].isValid()) {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::blue);
            }
            else {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
            }
            btns[i].setButtonText(instrumentPresetNames[i + 1].contains(".xml") ? instrumentPresetNames[i + 1].substring(0, instrumentPresetNames[i + 1].indexOf(".")) : instrumentPresetNames[i + 1]);
            if (instrumentPresetNames[i + 1].contains(".xml")) {
                btns[i].setTooltip(instrumentPresetNames[i + 1].substring(0, instrumentPresetNames[i + 1].indexOf(".")));
            }

        }

    }
    void buttonClicked(juce::Button* btn) override {

    }
private:
    juce::TextButton btns[6];
    void timerCallback() override
    {
        phase += 0.1f;
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        repaint();
    }

    float factor = 3.0f;
    float phase = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentPresetComponent)
};

class InstrumentPresetComponentItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY(InstrumentPresetComponentItem)

        InstrumentPresetComponentItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node) : foleys::GuiItem(builder, node)
    {
        addAndMakeVisible(activeInstrumentComponent);
    }

    void update() override
    {
        // activepresetcomponent.repaint();
        auto factor = getProperty("factor");
        activeInstrumentComponent.setFactor(factor.isVoid() ? 3.0f : float(factor));
    }

    juce::Component* getWrappedComponent() override
    {
        return &activeInstrumentComponent;
    }

private:
    InstrumentPresetComponent activeInstrumentComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentPresetComponentItem)
};

/* A clickable component that visualizes mapped, active, and empty presets */
class ActivePresetComponent : public juce::Component, private juce::Timer, public juce::Button::Listener, public MicrotonalSynthAudioProcessorEditor
{
public:
    /* Constructor that renders and activates the buttons for active preset */
    ActivePresetComponent(){
        for (int i = 0; i < 6; i++) {
            addAndMakeVisible(btns[i]);
            btns[i].setButtonText(microtonalPresetNames[i+1]);
            //btns[i].setEnabled(false);
            btns[i].addListener(this);
            btns[i].setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
        startTimerHz(30);
    };
    void setFactor(float f)
    {
        factor = f;
    }

    /*
      * Description: Used to rerender the component when the window size changes
      * Is generated by JUCE: No
      * Parameters: None
      * Return: N/A
    */
    void resized() override{
        auto rect = getLocalBounds();
        rect.setHeight(rect.getHeight() / 6);
        for (int i = 0; i < 6; i++) {
            btns[i].setBounds(rect);
            rect.setY(rect.getY() + rect.getHeight());
        }
    }

    /*
      * Description: Used to rerender the component when repaint() is called
      * Is generated by JUCE: No
      * Parameters: The graphics container
      * Return: N/A
    */
    void paint(juce::Graphics& g) override{
        /* Changes the color of the preset based on if it is active-mapped, empty, inactive-mapped */
        for (int i = 0; i < 6; i++) {
            if (i+1 == mappingGroup) {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
            } 
            else if (microtonalMappings[i + 1].isMapped()) {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::blue);
                btns[i].setButtonText(microtonalPresetNames[i + 1].contains(".xml") ? microtonalPresetNames[i + 1].substring(0, microtonalPresetNames[i + 1].indexOf(".")) : microtonalPresetNames[i + 1]);
            }
            else {
                btns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
                btns[i].setButtonText(juce::String(i + 1));
            }
        }
    }

    /*
      * Description: Used to change the active preset 
      * Is generated by JUCE: No
      * Parameters: The preset that was clicked
      * Return: N/A
    */
    void buttonClicked(juce::Button* btn) override{
        for (int i = 0; i < 6; i++) {
            if (btn == &btns[i]) {
                mappingGroup = mappingGroup == i + 1 ? Default : i + 1;
            }

        }
    }
private:
    juce::TextButton btns[6];
    /* Function to update the state of the application on a timer */
    void timerCallback() override
    {
        phase += 0.1f;
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        repaint();
    }

    float factor = 3.0f;
    float phase = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivePresetComponent)
};

/*
  * Description: This class is used to connect a custom component to GUI Magic and can be used as a template for creating new custom components
  * Is generated by JUCE: Generated by Foley's GUI Magic
*/
class ActivePresetComponentItem : public foleys::GuiItem
{
public:
    FOLEYS_DECLARE_GUI_FACTORY(ActivePresetComponentItem)

    ActivePresetComponentItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node) : foleys::GuiItem(builder, node)
    {
        addAndMakeVisible(activepresetcomponent);
    }

    void update() override
    {
        auto factor = getProperty("factor");
        activepresetcomponent.setFactor(factor.isVoid() ? 3.0f : float(factor));
    }

    juce::Component* getWrappedComponent() override
    {
        return &activepresetcomponent;
    }

private:
    ActivePresetComponent activepresetcomponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivePresetComponentItem)
};

void MicrotonalSynthAudioProcessorEditor::initialiseBuilder(foleys::MagicGUIBuilder& builder)
{
    builder.registerJUCEFactories();
    builder.registerJUCELookAndFeels();
    builder.registerLookAndFeel("Settings", make_unique<customSettings>());
    builder.registerLookAndFeel("Save", make_unique<customSave>());
    builder.registerLookAndFeel("Load", make_unique<customLoad>());
    builder.registerLookAndFeel("Power", make_unique<customPower>());
    builder.registerFactory("ActivePresetComponent", &ActivePresetComponentItem::factory);
    builder.registerFactory("InstrumentPresetComponent", &InstrumentPresetComponentItem::factory);
    //DBG(builder.getGuiRootNode().toXmlString());
}

void MicrotonalSynthAudioProcessorEditor::loadMicrotonalPreset(int preset) {
    // choose a file
    chooser = std::make_unique<juce::FileChooser>("Load a microtonal mapping preset", juce::File::getSpecialLocation(juce::File::hostApplicationPath).getParentDirectory(), "*.xml", true, true);
    auto flags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags, [this, preset] (const juce::FileChooser& fc) {
        if (fc.getResult() == juce::File{})
            return;
        juce::File myFile;

        myFile = fc.getResult();	
        juce::String fileName = myFile.getFileName();
        microtonalPresetNames[preset] = fileName;

        juce::XmlDocument doc(myFile.loadFileAsString());
        juce::XmlElement config = *doc.getDocumentElement();
        juce::ValueTree t;
        t = t.fromXml(config);
        if (t.isValid()) {
            microtonalMappings[preset].base_frequency = stod(t.getProperty("base_frequency").toString().toStdString());
            microtonalMappings[preset].divisions = stod(t.getProperty("total_divisions").toString().toStdString());
            int i = 0;
            for (juce::ValueTree frequency : t) {
                microtonalMappings[preset].frequencies[i].index = stoi(frequency.getProperty("index").toString().toStdString());
                microtonalMappings[preset].frequencies[i].frequency = stod(frequency.getProperty("value").toString().toStdString());
                i++;
            }
        }
        
    });
}