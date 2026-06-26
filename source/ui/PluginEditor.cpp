#include "PluginEditor.h"
#include "../PluginProcessor.h"

static constexpr const char* kLastFolderKey = "lastScriptFolder";

//==============================================================================
GenSoundEditor::GenSoundEditor (GenSoundProcessor& p)
    : AudioProcessorEditor (p), processor_ (p)
{
    // Persistent preferences stored in ~/Library/Preferences/GenSound/
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "GenSound";
    opts.filenameSuffix      = "settings";
    opts.osxLibrarySubFolder = "Preferences";
    appProperties_.setStorageParameters (opts);

    setSize (520, 360);
    setResizable (true, true);
    setResizeLimits (400, 280, 900, 600);

    // Title
    titleLabel_.setText ("GenSound", juce::dontSendNotification);
    titleLabel_.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    titleLabel_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel_);

    // Script path display
    scriptPathLabel_.setText ("No script loaded — drag a .cpp file or click Choose",
                              juce::dontSendNotification);
    scriptPathLabel_.setFont (juce::FontOptions (11.0f));
    scriptPathLabel_.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (scriptPathLabel_);

    // Buttons
    chooseScriptBtn_.setButtonText ("Choose Script...");
    chooseScriptBtn_.onClick = [this] { chooseScript(); };
    addAndMakeVisible (chooseScriptBtn_);

    compileBtn_.setButtonText ("Compile");
    compileBtn_.onClick = [this] { processor_.requestCompile(); };
    addAndMakeVisible (compileBtn_);

    openFolderBtn_.setButtonText ("Open Folder");
    openFolderBtn_.onClick = [this] { openScriptFolder(); };
    addAndMakeVisible (openFolderBtn_);

    // Status labels
    statusLabel_.setText ("Ready", juce::dontSendNotification);
    statusLabel_.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (statusLabel_);

    versionLabel_.setText ("Version: —", juce::dontSendNotification);
    versionLabel_.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (versionLabel_);

    voiceCountLabel_.setText ("Voices: 0", juce::dontSendNotification);
    voiceCountLabel_.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (voiceCountLabel_);

    // Error/diagnostic output
    errorBox_.setMultiLine (true);
    errorBox_.setReadOnly (true);
    errorBox_.setScrollbarsShown (true);
    errorBox_.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    errorBox_.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1a1a1a));
    errorBox_.setColour (juce::TextEditor::textColourId,       juce::Colours::lightgrey);
    addAndMakeVisible (errorBox_);

    // Drop hint
    dropHintLabel_.setText ("Drop a .cpp script here", juce::dontSendNotification);
    dropHintLabel_.setFont (juce::FontOptions (11.0f));
    dropHintLabel_.setColour (juce::Label::textColourId, juce::Colours::grey);
    dropHintLabel_.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (dropHintLabel_);

    startTimerHz (10);  // 100ms refresh
}

GenSoundEditor::~GenSoundEditor()
{
    stopTimer();
}

//==============================================================================
juce::File GenSoundEditor::lastBrowseFolder()
{
    if (auto* props = appProperties_.getUserSettings())
    {
        auto saved = props->getValue (kLastFolderKey);
        if (saved.isNotEmpty())
        {
            juce::File folder (saved);
            if (folder.isDirectory())
                return folder;
        }
    }
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory);
}

void GenSoundEditor::saveLastBrowseFolder (const juce::File& folder)
{
    if (auto* props = appProperties_.getUserSettings())
    {
        props->setValue (kLastFolderKey, folder.getFullPathName());
        props->saveIfNeeded();
    }
}

//==============================================================================
void GenSoundEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff242424));

    // Separator line above error box
    auto bounds = getLocalBounds().reduced (8);
    int  sepY   = bounds.getY() + 140;
    g.setColour (juce::Colours::darkgrey);
    g.drawHorizontalLine (sepY, static_cast<float> (bounds.getX()),
                                static_cast<float> (bounds.getRight()));
}

void GenSoundEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    titleLabel_.setBounds      (area.removeFromTop (30));
    area.removeFromTop (4);

    scriptPathLabel_.setBounds (area.removeFromTop (18));
    area.removeFromTop (6);

    // Button row
    auto btnRow = area.removeFromTop (28);
    chooseScriptBtn_.setBounds (btnRow.removeFromLeft (130));
    btnRow.removeFromLeft (6);
    compileBtn_.setBounds      (btnRow.removeFromLeft (90));
    btnRow.removeFromLeft (6);
    openFolderBtn_.setBounds   (btnRow.removeFromLeft (100));

    area.removeFromTop (6);

    // Status row
    auto statusRow = area.removeFromTop (20);
    statusLabel_.setBounds     (statusRow.removeFromLeft (220));
    versionLabel_.setBounds    (statusRow.removeFromLeft (120));
    voiceCountLabel_.setBounds (statusRow);

    area.removeFromTop (10);

    // Error box fills the rest; drop hint sits above it
    dropHintLabel_.setBounds (area.removeFromTop (16));
    area.removeFromTop (2);
    errorBox_.setBounds (area);
}

//==============================================================================
void GenSoundEditor::timerCallback()
{
    refreshStatus();
}

void GenSoundEditor::refreshStatus()
{
    bool compiling = processor_.isCompiling();

    compileBtn_.setEnabled (!compiling);
    compileBtn_.setButtonText (compiling ? "Compiling..." : "Compile");

    auto path = processor_.currentScriptPath();
    if (path.empty())
        scriptPathLabel_.setText ("No script loaded — drag a .cpp file or click Choose",
                                  juce::dontSendNotification);
    else
        scriptPathLabel_.setText (path, juce::dontSendNotification);

    auto status = processor_.lastCompileStatus();
    statusLabel_.setText (status.empty() ? "Ready" : status,
                          juce::dontSendNotification);

    // Colour is driven by the process exit code, not string parsing
    bool ok = status.empty() || processor_.lastCompileSuccess();
    statusLabel_.setColour (juce::Label::textColourId,
                            ok ? juce::Colours::lightgreen : juce::Colours::orangered);

    uint32_t ver = processor_.activeVersionNumber();
    versionLabel_.setText (ver > 0 ? ("v" + std::to_string (ver)) : "Version: —",
                           juce::dontSendNotification);

    voiceCountLabel_.setText ("Voices: " + std::to_string (processor_.activeVoiceCount()),
                              juce::dontSendNotification);

    auto errors = processor_.lastCompileErrors();
    if (errorBox_.getText().toStdString() != errors)
        errorBox_.setText (errors, false);
}

//==============================================================================
bool GenSoundEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWith (".cpp"))
            return true;
    return false;
}

void GenSoundEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
    {
        if (f.endsWith (".cpp"))
        {
            saveLastBrowseFolder (juce::File (f).getParentDirectory());
            processor_.setScriptPath (f.toStdString());
            break;
        }
    }
}

//==============================================================================
void GenSoundEditor::chooseScript()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Select a GenSound script (.cpp)",
        lastBrowseFolder(),
        "*.cpp");

    chooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                saveLastBrowseFolder (result.getParentDirectory());
                processor_.setScriptPath (result.getFullPathName().toStdString());
            }
        });
}

void GenSoundEditor::openScriptFolder()
{
    auto path = processor_.currentScriptPath();
    if (path.empty()) return;

    juce::File (path).getParentDirectory().revealToUser();
}
