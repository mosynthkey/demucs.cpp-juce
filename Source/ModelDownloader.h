#pragma once

#include <JuceHeader.h>
#include <functional>

class ModelDownloader : public juce::Thread
{
public:
    using CompletionCallback = std::function<void(bool wasSuccessful, const juce::String& error)>;
    using ProgressCallback = std::function<void(float progress, const juce::String& message)>;

    ModelDownloader(CompletionCallback onComplete, ProgressCallback onProgress) 
        : Thread("ModelDownloader"),
          mOnComplete(std::move(onComplete)),
          mOnProgress(std::move(onProgress))
    {
        modelUrl = "https://huggingface.co/datasets/Retrobear/demucs.cpp/resolve/main/ggml-model-htdemucs-6s-f16.bin";
    }

    static juce::File getModelDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("DemucsJUCE/demucs_models");
    }

    static juce::File getDefaultModelFile()
    {
        return getModelDirectory().getChildFile("ggml-model-htdemucs-6s-f16.bin");
    }

    void run() override
    {
        DBG("ModelDownloader: Starting download process");
        
        auto modelDir = getModelDirectory();
        DBG("ModelDownloader: Model directory path: " + modelDir.getFullPathName());
        
        if (!modelDir.exists())
        {
            DBG("ModelDownloader: Creating model directory");
            reportProgress(0.0f, "Creating model directory...");
            if (!modelDir.createDirectory())
            {
                DBG("ModelDownloader: Failed to create directory");
                success = false;
                errorMessage = "Failed to create model directory: " + modelDir.getFullPathName();
                return;
            }
        }

        auto destFile = getDefaultModelFile();
        DBG("ModelDownloader: Destination file path: " + destFile.getFullPathName());
        
        reportProgress(0.0f, "Connecting to server...");
        DBG("ModelDownloader: Connecting to URL: " + modelUrl);
        
        juce::URL url(modelUrl);
        auto stream = url.createInputStream(false);
        if (stream == nullptr)
        {
            DBG("ModelDownloader: Failed to create input stream");
            success = false;
            errorMessage = "Failed to connect to download server";
            return;
        }

        const int64 totalSize = stream->getTotalLength();
        const float totalSizeMB = totalSize / (1024.0f * 1024.0f);
        
        DBG("ModelDownloader: Total file size: " + juce::String(totalSizeMB, 1) + " MB");
        reportProgress(0.0f, "Starting download (" + juce::String(totalSizeMB, 1) + " MB)...");
        
        juce::FileOutputStream outputStream(destFile);
        if (outputStream.failedToOpen())
        {
            DBG("ModelDownloader: Failed to create output file: " + destFile.getFullPathName());
            success = false;
            errorMessage = "Failed to create output file: " + destFile.getFullPathName();
            return;
        }

        juce::MemoryBlock buffer(8192);
        int64 downloaded = 0;
        int lastPercentage = 0;
        int readAttempts = 0;
        const int maxReadAttempts = 3;

        while (!threadShouldExit())
        {
            const int numRead = stream->read(buffer.getData(), buffer.getSize());
            if (numRead < 0)
            {
                DBG("ModelDownloader: Read error occurred");
                break;
            }
            else if (numRead == 0)
            {
                readAttempts++;
                DBG("ModelDownloader: Zero bytes read (attempt " + juce::String(readAttempts) + " of " + juce::String(maxReadAttempts) + ")");
                if (readAttempts >= maxReadAttempts)
                {
                    DBG("ModelDownloader: Max read attempts reached");
                    break;
                }
                juce::Thread::sleep(1000); // Wait a second before retry
                continue;
            }

            readAttempts = 0; // Reset counter on successful read
            
            if (!outputStream.write(buffer.getData(), numRead))
            {
                DBG("ModelDownloader: Failed to write to output file");
                success = false;
                errorMessage = "Failed to write to output file";
                return;
            }

            downloaded += numRead;
            DBG("ModelDownloader: Downloaded " + juce::String(downloaded / (1024.0f * 1024.0f), 1) + " MB");

            if (totalSize > 0)
            {
                const float progress = downloaded / (double)totalSize;
                int currentPercentage = static_cast<int>(progress * 100);
                if (currentPercentage / 10 > lastPercentage / 10)
                {
                    const float downloadedMB = downloaded / (1024.0f * 1024.0f);
                    auto progressMsg = juce::String(currentPercentage) + "% completed (" + 
                                     juce::String(downloadedMB, 1) + " MB / " + 
                                     juce::String(totalSizeMB, 1) + " MB)";
                    DBG("ModelDownloader: " + progressMsg);
                    reportProgress(progress, progressMsg);
                    lastPercentage = currentPercentage;
                }
            }
        }

        if (threadShouldExit())
        {
            DBG("ModelDownloader: Download cancelled by user");
            reportProgress(1.0f, "Download cancelled");
            success = false;
            errorMessage = "Download cancelled by user";
        }
        else if (downloaded == totalSize)
        {
            DBG("ModelDownloader: Download completed successfully");
            reportProgress(1.0f, "Download completed successfully");
            success = true;
        }
        else
        {
            DBG("ModelDownloader: Download incomplete. Expected: " + juce::String(totalSize) + 
                " bytes, Got: " + juce::String(downloaded) + " bytes");
            success = false;
            errorMessage = "Download incomplete or corrupted";
        }

        outputStream.flush();
        
        // 完了通知
        if (mOnComplete)
        {
            DBG("ModelDownloader: Sending completion callback");
            juce::MessageManager::callAsync([this]()
            {
                mOnComplete(success, errorMessage);
            });
        }
    }

    bool wasSuccessful() const { return success; }
    const juce::String& getError() const { return errorMessage; }

private:
    void reportProgress(float progress, const juce::String& message)
    {
        if (mOnProgress)
        {
            juce::MessageManager::callAsync([this, progress, message]()
            {
                mOnProgress(progress, message);
            });
        }
    }

    juce::String modelUrl;
    bool success = false;
    juce::String errorMessage;
    CompletionCallback mOnComplete;
    ProgressCallback mOnProgress;
}; 