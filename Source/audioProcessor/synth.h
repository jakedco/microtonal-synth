/*
  ==============================================================================

    synth.h
    Created: 22 Jan 2022 9:37:13pm
    Author:  Michael

  ==============================================================================
*/

#pragma once


#include "JuceHeader.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>

class Synth : public juce::Synthesiser
{
public:
    static int  numOscillators;

    static void addADSRParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    static void addOvertoneParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    static void addGainParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    Synth() = default;

    class Sound : public juce::SynthesiserSound
    {
    public:
        Sound(juce::AudioProcessorValueTreeState& state);
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }

        juce::ADSR::Parameters getADSR();

    private:
        juce::AudioProcessorValueTreeState& state;
        juce::AudioParameterFloat* attack = nullptr;
        juce::AudioParameterFloat* decay = nullptr;
        juce::AudioParameterFloat* sustain = nullptr;
        juce::AudioParameterFloat* release = nullptr;
        juce::AudioParameterFloat* gain = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sound)
    };

    class Voice : public juce::SynthesiserVoice
    {
    public:
        Voice(juce::AudioProcessorValueTreeState& state);

        bool canPlaySound(juce::SynthesiserSound*) override;

        void startNote(int midiNoteNumber,
            float velocity,
            juce::SynthesiserSound* sound,
            int currentPitchWheelPosition) override;

        void stopNote(float velocity, bool allowTailOff) override;

        void pitchWheelMoved(int newPitchWheelValue) override;

        void controllerMoved(int controllerNumber, int newControllerValue) override;

        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
            int startSample,
            int numSamples) override;

        void setCurrentPlaybackSampleRate(double newRate) override;

    private:

        class BaseOscillator
        {
        public:
            BaseOscillator() = default;

            juce::dsp::ProcessorChain<juce::dsp::Oscillator<float>, juce::dsp::Gain<float>> osc;
            juce::AudioParameterFloat* gain = nullptr;
            juce::AudioParameterFloat* detune = nullptr;
            juce::AudioParameterChoice* wave_form = nullptr;
            juce::AudioParameterFloat* gainA = nullptr;
            juce::AudioParameterFloat* detuneA = nullptr;
            juce::AudioParameterChoice* wave_formA = nullptr;
            juce::AudioParameterFloat* attackA = nullptr;
            juce::AudioParameterFloat* decayA = nullptr;
            juce::AudioParameterFloat* sustainA = nullptr;
            juce::AudioParameterFloat* releaseA = nullptr;
            float                       angleDelta = 0.0;
            float                       angleDeltaA = 0.0; //LFO
            float                       currentAngle = 0.0;
            float                       currentAngleA = 0.0;
            float                       lastGainASDR = 0.0;
            float                       releaseGain = 0.0;
            double multiplier = 1.0;

        private:
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseOscillator)
        };

        double getDetuneFromPitchWheel(int wheelValue) const;
        double getFrequencyForNote(int noteNumber, double detune, double concertPitch = 440.0) const;

        void updateFrequency(BaseOscillator& oscillator, bool noteStart = false);

        std::vector<std::unique_ptr<BaseOscillator>> oscillators;

        double                      pitchWheelValue = 0.0;
        int                         maxPitchWheelSemitones = 12;
        const int                   internalBufferSize = 64;
        juce::AudioBuffer<float>    oscillatorBuffer;
        juce::AudioBuffer<float>    voiceBuffer;
        juce::ADSR                  adsr;
        juce::AudioParameterFloat* gainParameter = nullptr;
        float                       lastGain = 0.0;
        int64_t                     timeG = 0;
        int64_t                     starttime = 0;
        int64_t                     starttimeR = 0;
        bool                        released = false;
        std::vector<float> cu_w[7];
        float cu_t[7] = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)

    public:
        void getSamples(BaseOscillator& osc, juce::dsp::ProcessContextReplacing<float>& pc);
        void loadcustomwave(const char* file, int i);
        float getOscASDR(BaseOscillator& osc);
        float getOsc(float currentAngleR, int wave_form);
    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Synth)
};


void incCurrentAngle(float& currentAngleR, float angleDeltaR);