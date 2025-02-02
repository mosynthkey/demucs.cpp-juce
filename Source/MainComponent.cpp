#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::white);
    g.drawText("Demucs JUCE", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
} 