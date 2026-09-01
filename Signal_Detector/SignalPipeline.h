#pragma once
#include <vector>
#include <random>
#include "SineGenerator.h"
#include "NoiseInjector.h"

enum class SignalMode {
    Manual,
    Randomized
};

enum class AnomalyType {
    None,
    Spike,
    Stuck,
    Drift
};

void generateSignal(std::vector<float>& signal, std::vector<bool>& isAnomaly, SineGenerator& gen, NoiseInjector& noise,
    int bufferSize, AnomalyType selectedAnomaly,
    int spikeStart, float spikeMagnitude,
    int stuckStart, int stuckDuration,
    float driftRate, int driftStart);

void randomizeAnomaly(std::mt19937& randomGen, int bufferSize, AnomalyType& selectedAnomaly,
    int& spikeStart, float& spikeMagnitude,
    int& stuckStart, int& stuckDuration,
    float& driftRate, int& driftStart);
