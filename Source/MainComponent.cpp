#include "MainComponent.h"
#include <memory>

MainComponent::MainComponent()
    : Thread("DemucsProcessingThread")
{
    setSize(800, 600);

    addAndMakeVisible(mOpenButton);
    addAndMakeVisible(mProcessButton);
    addAndMakeVisible(mLogArea);
    addAndMakeVisible(mStatusLabel);
    addAndMakeVisible(mProgressBar);

    mOpenButton.onClick = [this]()
    {
        mFileChooser = std::make_unique<juce::FileChooser>(
            "Select a 44.1kHz stereo WAV file...",
            juce::File{},
            "*.wav");

        mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | 
                           juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                if (fc.getResults().size() > 0)
                {
                    auto file = fc.getResult();
                    
                    // Check audio file properties
                    juce::AudioFormatManager formatManager;
                    formatManager.registerBasicFormats();
                    
                    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(
                        formatManager.createReaderFor(file)))
                    {
                        if (reader->sampleRate != 44100.0)
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::WarningIcon,
                                "Unsupported Sample Rate",
                                "Only 44.1kHz audio files are supported.");
                            return;
                        }
                        
                        if (reader->numChannels != 2)
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::WarningIcon,
                                "Unsupported Channel Count",
                                "Only stereo audio files are supported.");
                            return;
                        }

                        mSelectedFile = file;
                        mProcessButton.setEnabled(mModel != nullptr);
                        updateProgressMessage("Audio file selected: " + mSelectedFile.getFileName());
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Invalid File",
                            "Could not open audio file.");
                    }
                }
            });
    };

    mProcessButton.onClick = [this]()
    {
        if (mIsProcessing)
        {
            signalThreadShouldExit();
            mProcessButton.setEnabled(false);
            updateProgressMessage("Stopping...");
        }
        else if (mModel != nullptr)
        {
            mIsProcessing = true;
            mProcessButton.setButtonText("Stop");
            mOpenButton.setEnabled(false);
            startThread();
        }
    };

    mLogArea.setMultiLine(true);
    mLogArea.setReadOnly(true);
    mLogArea.setCaretVisible(false);

    mProcessButton.setEnabled(false);

    if (ModelDownloader::getDefaultModelFile().exists())
    {
        mModelFile = ModelDownloader::getDefaultModelFile();
        loadModel();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::QuestionIcon,
            "Model Download Required",
            "The required model file is not found. Click 'OK' to download it now.",
            "OK",
            this,
            juce::ModalCallbackFunction::create(
                [this](int result)
                {
                    mDownloader = std::make_unique<ModelDownloader>(
                        [this](bool wasSuccessful, const juce::String& error)
                        {
                            if (wasSuccessful)
                            {
                                mModelFile = ModelDownloader::getDefaultModelFile();
                                loadModel();

                                juce::AlertWindow::showMessageBoxAsync(
                                    juce::AlertWindow::InfoIcon,
                                    "Success",
                                    "Model downloaded successfully!");
                            }
                            else
                            {
                                juce::AlertWindow::showMessageBoxAsync(
                                    juce::AlertWindow::WarningIcon,
                                    "Download Failed",
                                    "Failed to download model: " + error);
                            }
                            
                            mDownloader.reset();
                        },
                        [this](float progress, const juce::String& message)
                        {
                            updateProgressMessage(message);
                        });

                    mDownloader->startThread();
                }));
    }
}

MainComponent::~MainComponent()
{
    signalThreadShouldExit();
    stopThread(3000);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto topArea = area.removeFromTop(40);

    mOpenButton.setBounds(topArea.removeFromLeft(150));
    topArea.removeFromLeft(10);
    mProcessButton.setBounds(topArea.removeFromLeft(150));

    area.removeFromTop(10);
    mStatusLabel.setBounds(area.removeFromTop(30));

    area.removeFromTop(10);
    auto progressArea = area.removeFromTop(20);
    mProgressBar.setBounds(progressArea);

    area.removeFromTop(10);
    mLogArea.setBounds(area);
}

void MainComponent::loadModel()
{
    try
    {
        updateProgressMessage("Loading model...");
        mModel = std::make_unique<demucscpp::demucs_model>();
        if (!demucscpp::load_demucs_model(mModelFile.getFullPathName().toStdString(), mModel.get()))
        {
            throw std::runtime_error("Failed to load model");
        }
        updateProgressMessage("Model loaded successfully");
        mProcessButton.setEnabled(mSelectedFile.exists());
    }
    catch (const std::exception& e)
    {
        mModel.reset();
        updateProgressMessage("Error loading model: " + juce::String(e.what()));
    }
}

void MainComponent::run()
{
    try
    {
        processAudioFile();
    }
    catch (const std::exception& e)
    {
        juce::MessageManager::callAsync([this, error = std::string(e.what())]()
        {
            updateProgressMessage("Error: " + juce::String(error));
            resetProcessingState();
        });
    }
}

void MainComponent::processAudioFile()
{
    updateProgressMessage("Processing audio file...", 0.0f);

    if (!mModel)
        throw std::runtime_error("Model not loaded");

    // Load audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(mSelectedFile));

    if (!reader)
        throw std::runtime_error("Could not load audio file");

    const int numSamples = static_cast<int>(reader->lengthInSamples);
    Eigen::MatrixXf audioData(2, numSamples);
    
    juce::AudioBuffer<float> buffer(2, numSamples);
    reader->read(&buffer, 0, numSamples, 0, true, true);

    // Copy to Eigen matrix
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            audioData(ch, i) = channelData[i];
    }

    updateProgressMessage("Running Demucs inference...");
    
    // Process with demucs
    auto out_targets = demucscpp::demucs_inference(*mModel, audioData, 
        [this](float progress, const std::string& message) {
            if (threadShouldExit())
            {
                throw std::runtime_error("Processing cancelled by user");
            }
            juce::MessageManager::callAsync([this, message, progress]() {
                updateProgressMessage(message, progress);
            });
        });
    
    if (threadShouldExit())
    {
        throw std::runtime_error("Processing cancelled by user");
    }

    updateProgressMessage("Saving separated tracks...");

    // Create output directory
    auto outputDir = mSelectedFile.getParentDirectory().getChildFile(mSelectedFile.getFileNameWithoutExtension() + "_stems");
    outputDir.createDirectory();

    // Save each target
    juce::WavAudioFormat wavFormat;
    for (int target = 0; target < kNumStems; ++target)
    {
        if (threadShouldExit())
        {
            throw std::runtime_error("Processing cancelled by user");
        }

        auto outputFile = outputDir.getChildFile(
            mSelectedFile.getFileNameWithoutExtension() + juce::String("_") + STEM_NAMES[target] + ".wav");

        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(new juce::FileOutputStream(outputFile),
                                    44100, 2, 16, {}, 0));
        
        if (writer)
        {
            juce::AudioBuffer<float> outBuffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
            {
                float* channelData = outBuffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    channelData[i] = out_targets(target, ch, i);
                }
            }
            
            writer->writeFromAudioSampleBuffer(outBuffer, 0, numSamples);
        }
        
        updateProgressMessage("Saved " + juce::String(STEM_NAMES[target]));
    }

    updateProgressMessage("Processing complete!", 1.0f);
    
    juce::MessageManager::callAsync([this]()
    {
        resetProcessingState();
    });
}

void MainComponent::updateProgressMessage(const juce::String& message, float progress)
{
    juce::MessageManager::callAsync([this, message, progress]()
    {
        mStatusLabel.setText(message, juce::dontSendNotification);
        mLogArea.moveCaretToEnd();
        mLogArea.insertTextAtCaret(message + "\n");
        
        if (progress >= 0.0f)
        {
            mProgress = progress;
        }
    });
}

void MainComponent::resetProcessingState()
{
    mIsProcessing = false;
    mProgress = 0.0;
    mProcessButton.setButtonText("Process");
    mProcessButton.setEnabled(true);
    mOpenButton.setEnabled(true);
} 
