#include "PluginProcessor.h"
#include "ui/PluginEditor.h"

#include <filesystem>

namespace fs = std::filesystem;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
GenSynthProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "p" + std::to_string (i), 1 },
            "P" + std::to_string (i),
            juce::NormalisableRange<float> (0.0f, 1.0f),
            0.0f));
    }

    return layout;
}

//==============================================================================
GenSynthProcessor::GenSynthProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Default build dir beside the plugin's user data folder
    auto buildDir = (juce::File::getSpecialLocation (
                         juce::File::userApplicationDataDirectory)
                     .getChildFile ("GenSynth/build")).getFullPathName().toStdString();

    auto sdkDir   = (juce::File::getSpecialLocation (
                         juce::File::currentApplicationFile)
                     .getParentDirectory()
                     .getChildFile ("sdk")).getFullPathName().toStdString();

    scriptManager_.setBuildDirectory  (buildDir);
    scriptManager_.setSdkIncludePath  (sdkDir);
}

GenSynthProcessor::~GenSynthProcessor()
{
    fileWatcher_.stop();
}

//==============================================================================
bool GenSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::disabled())
        return false;
    return true;
}

//==============================================================================
void GenSynthProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate_ = sampleRate;
}

void GenSynthProcessor::releaseResources() {}

//==============================================================================
void GenSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer&          midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // Push any new script version to the voice manager
    auto version = scriptManager_.activeVersion();
    if (version && version->valid)
        voiceManager_.setCurrentScriptVersion (version);

    // Collect parameter values
    float p[8] {};
    for (int i = 0; i < 8; ++i)
    {
        auto* param = apvts.getRawParameterValue ("p" + std::to_string (i));
        p[i] = param ? param->load() : 0.0f;
    }

    // Process MIDI and handle note events
    int numSamples = buffer.getNumSamples();

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
            voiceManager_.handleNoteOn (static_cast<uint8_t> (msg.getNoteNumber()),
                                        static_cast<uint8_t> (msg.getVelocity()));
        else if (msg.isNoteOff())
            voiceManager_.handleNoteOff (static_cast<uint8_t> (msg.getNoteNumber()));
    }

    // Transport info
    float bpm             = 120.0f;
    float beatPosition    = 0.0f;
    float beatsPerSample  = 0.0f;
    float songTime        = 0.0f;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = static_cast<float> (*pos->getBpm());

            if (pos->getPpqPosition().hasValue())
                beatPosition = static_cast<float> (*pos->getPpqPosition());

            if (pos->getTimeInSeconds().hasValue())
                songTime = static_cast<float> (*pos->getTimeInSeconds());
        }
    }

    beatsPerSample = (bpm / 60.0f) / static_cast<float> (currentSampleRate_);

    // Render voices
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);

    voiceManager_.renderBlock (outL, outR, numSamples,
                               static_cast<float> (currentSampleRate_),
                               bpm, beatPosition, beatsPerSample, songTime, p);

    // Housekeeping — collect old dylibs safely here (non-audio-critical section)
    scriptManager_.collectGarbage();
}

//==============================================================================
void GenSynthProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("scriptPath", juce::String (scriptPath_), nullptr);

    juce::MemoryOutputStream mos (dest, true);
    state.writeToStream (mos);
}

void GenSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
    if (tree.isValid())
    {
        apvts.replaceState (tree);
        auto path = tree.getProperty ("scriptPath").toString().toStdString();
        if (!path.empty())
            setScriptPath (path);
    }
}

//==============================================================================
juce::AudioProcessorEditor* GenSynthProcessor::createEditor()
{
    return new GenSynthEditor (*this);
}

//==============================================================================
void GenSynthProcessor::requestCompile()
{
    scriptManager_.compile ([this] (const CompileDiagnostics&      diag,
                                   std::shared_ptr<ScriptVersion>  newVersion)
    {
        onCompileComplete (diag, std::move (newVersion));
    });
}

void GenSynthProcessor::onCompileComplete (const CompileDiagnostics&      diag,
                                           std::shared_ptr<ScriptVersion> /*newVersion*/)
{
    juce::ScopedLock sl (statusLock_);
    lastStatus_ = diag.summary();
    lastErrors_ = diag.rawStderr;
}

//==============================================================================
void GenSynthProcessor::setScriptPath (const std::string& path)
{
    scriptPath_ = path;
    scriptManager_.setScriptPath (path);
    startFileWatcher();
    requestCompile();
}

std::string GenSynthProcessor::currentScriptPath()  const { return scriptPath_; }
uint32_t    GenSynthProcessor::activeVersionNumber() const { return scriptManager_.activeVersionNumber(); }
int         GenSynthProcessor::activeVoiceCount()    const { return voiceManager_.activeVoiceCount(); }
bool        GenSynthProcessor::isCompiling()         const { return scriptManager_.isCompiling(); }

std::string GenSynthProcessor::lastCompileStatus() const
{
    juce::ScopedLock sl (statusLock_);
    return lastStatus_;
}

std::string GenSynthProcessor::lastCompileErrors() const
{
    juce::ScopedLock sl (statusLock_);
    return lastErrors_;
}

//==============================================================================
void GenSynthProcessor::startFileWatcher()
{
    if (scriptPath_.empty()) return;

    fileWatcher_.start (
        scriptPath_,
        [this] (const std::string&)
        {
            // Called on watcher thread — requestCompile() is thread-safe
            requestCompile();
        });
}

//==============================================================================
// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GenSynthProcessor();
}
