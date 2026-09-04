#include "NoiseInjector.h"
#include <algorithm> 

NoiseInjector::NoiseInjector(float stdDev)
    : gen(rd()), dist(0.0f, stdDev) {
}

float NoiseInjector::apply(float cleanValue) {
    return cleanValue + dist(gen);
}

void NoiseInjector::setStdDev(float stdDev) {
    dist = std::normal_distribution<float>(0.0f, stdDev);
}

void NoiseInjector::applySpike(std::vector<float>& signal, int start, float magnitude, int duration)
{
    if (start < 0) start = 0;
    if (duration <= 0) return;

    int end = std::min<int>(static_cast<int>(signal.size()), start + duration);
    for (int i = start; i < end; ++i) {
        signal[i] += magnitude; 
    }
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