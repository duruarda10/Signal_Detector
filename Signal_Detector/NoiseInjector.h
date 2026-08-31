#pragma once
#include <random>

class NoiseInjector {
public:
	NoiseInjector(float stdDev);
	float apply(float cleanValue);
	void setStdDev(float stdDev);
	float applySpike(float value, bool triggerSpike, float spikeMagnitude);
private:
	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dist;
};
