#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class GenSynthProcessor;

//==============================================================================
// GenSynthEditor
//
// Minimal UI: script path, compile status, error text, active voice count,
// version number, compile button, and open-folder button.
// Updates on a 100ms timer tick.
//==============================================================================
class GenSynthEditor : public juce::AudioProcessorEditor,
                       private juce::Timer,
                       private juce::FileDragAndDropTarget
{
public:
    explicit GenSynthEditor (GenSynthProcessor& p);
    ~GenSynthEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    // juce::Timer
    void timerCallback() override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void refreshStatus();
    void chooseScript();
    void openScriptFolder();

    GenSynthProcessor& processor_;

    juce::Label       titleLabel_;
    juce::Label       scriptPathLabel_;
    juce::TextButton  chooseScriptBtn_;
    juce::TextButton  compileBtn_;
    juce::TextButton  openFolderBtn_;
    juce::Label       statusLabel_;
    juce::Label       versionLabel_;
    juce::Label       voiceCountLabel_;
    juce::TextEditor  errorBox_;
    juce::Label       dropHintLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GenSynthEditor)
};
