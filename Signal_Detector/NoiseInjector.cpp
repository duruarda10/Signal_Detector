#include "NoiseInjector.h"

NoiseInjector::NoiseInjector(float stdDev)
    : gen(rd()), dist(0.0f, stdDev) {
}

float NoiseInjector::apply(float cleanValue) {
    return cleanValue + dist(gen);
}

void NoiseInjector::setStdDev(float stdDev) {
    dist = std::normal_distribution<float>(0.0f, stdDev);
}

float NoiseInjector::applySpike(float value, bool triggerSpike, float spikeMagnitude) {
    if (triggerSpike) {
        return value + spikeMagnitude;
    }
    return value;
}