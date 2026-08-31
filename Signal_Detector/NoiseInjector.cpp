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

float NoiseInjector::applyStuck(float value, bool triggerStuck, float stuckValue) {
	if (triggerStuck) {
		return stuckValue;
	}
	return value;
}

float NoiseInjector::applyDrift(float value, bool triggerDrift, float driftRate, int samplesSinceStart) {
    if (triggerDrift) {
        return value + (driftRate * samplesSinceStart);
    }
    return value;
}