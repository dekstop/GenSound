#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/VoiceManager.h"
#include "compile/ScriptManager.h"
#include "compile/FileWatcher.h"

//==============================================================================
// PluginProcessor
//
// Top-level JUCE AudioProcessor. Owns VoiceManager, ScriptManager, and
// FileWatcher. The audio thread only touches VoiceManager; all other work
// happens on background threads.
//==============================================================================
class GenSynthProcessor : public juce::AudioProcessor
{
public:
    //--------------------------------------------------------------------------
    GenSynthProcessor();
    ~GenSynthProcessor() override;

    //--------------------------------------------------------------------------
    // AudioProcessor interface
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool                        hasEditor()    const override { return true; }

    const juce::String getName() const override { return "GenSynth"; }

    bool   acceptsMidi()           const override { return true; }
    bool   producesMidi()          const override { return false; }
    bool   isMidiEffect()          const override { return false; }
    double getTailLengthSeconds()  const override { return 0.0; }

    int  getNumPrograms()                              override { return 1; }
    int  getCurrentProgram()                           override { return 0; }
    void setCurrentProgram (int)                       override {}
    const juce::String getProgramName (int)            override { return {}; }
    void changeProgramName (int, const juce::String&)  override {}

    void getStateInformation (juce::MemoryBlock& dest)          override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //--------------------------------------------------------------------------
    // Called by the editor to trigger a manual compile
    void requestCompile();

    // Paths / status for the editor
    std::string  currentScriptPath()    const;
    void         setScriptPath         (const std::string& path);
    std::string  lastCompileStatus()    const;
    std::string  lastCompileErrors()    const;
    bool         lastCompileSuccess()   const;
    uint32_t     activeVersionNumber()  const;
    int          activeVoiceCount()     const;
    bool         isCompiling()          const;

    // 8 automatable parameters
    juce::AudioProcessorValueTreeState apvts;

private:
    void onCompileComplete (const CompileDiagnostics&      diag,
                            std::shared_ptr<ScriptVersion> newVersion);

    void startFileWatcher();

    //--------------------------------------------------------------------------
    VoiceManager  voiceManager_;
    ScriptManager scriptManager_;
    FileWatcher   fileWatcher_;

    double currentSampleRate_  = 44100.0;

    // Compile status — written on compiler thread, read on UI thread
    mutable juce::CriticalSection statusLock_;
    std::string   lastStatus_;
    std::string   lastErrors_;
    bool          lastSuccess_ = true;

    // Script path persisted in plugin state
    std::string   scriptPath_;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GenSynthProcessor)
};
