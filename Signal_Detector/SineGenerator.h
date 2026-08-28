
#pragma once

class SineGenerator {
public:
    SineGenerator(float frequency, float amplitude, float sampleRate, float phase = 0.0f);
    float getNextSample();
    void reset();
    void setFrequency(float f);
    void setAmplitude(float a);
    void setPhase(float p);

private:
    float freq;
    float amp;
    float phi;
    float t = 0;
    float dt;
};