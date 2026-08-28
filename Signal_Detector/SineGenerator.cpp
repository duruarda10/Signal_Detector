#define _USE_MATH_DEFINES
#include "SineGenerator.h"
#include <cmath>

SineGenerator::SineGenerator(float frequency, float amplitude, float sampleRate, float phase)
    : freq(frequency), amp(amplitude), phi(phase), dt(1.0f / sampleRate) {
}

float SineGenerator::getNextSample() {
    float value = amp * std::sin(2.0f * M_PI * freq * t + phi);
    t += dt;
    return value;
}

void SineGenerator::reset() {
    t = 0;
}

void SineGenerator::setFrequency(float f) {
    freq = f;
}

void SineGenerator::setAmplitude(float a) {
    amp = a;
}

void SineGenerator::setPhase(float p) {
    phi = p;
}