#pragma once

#include <JuceHeader.h>
#include "model.hpp"
#include "ModelDownloader.h"

class MainComponent : public juce::Component,
                     public juce::Thread
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void run() override; // Thread
    void processAudioFile();
    void updateProgressMessage(const juce::String& message);
    void loadModel();
    void resetProcessingState();

    juce::TextButton mOpenButton { "Open Audio File" };
    juce::TextButton mProcessButton { "Process" };
    juce::TextEditor mLogArea;
    juce::Label mStatusLabel { {}, "Status: Ready" };

    juce::File mSelectedFile;
    juce::File mModelFile;
    std::unique_ptr<demucscpp::demucs_model> mModel;

    std::unique_ptr<juce::FileChooser> mFileChooser;
    std::unique_ptr<ModelDownloader> mDownloader;

    std::atomic<bool> mIsProcessing { false };

    static constexpr const char* STEM_NAMES[] = { 
        "drums", "bass", "other", "vocals", "guitar", "piano" 
    };
    static constexpr int kNumStems = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
}; 