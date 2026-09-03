#define _USE_MATH_DEFINES

#include "SignalPipeline.h"
#include <cmath>

void generateSignal(std::vector<float>& signal, std::vector<float>& cleanSignal, std::vector<bool>& isAnomaly, SineGenerator& gen, NoiseInjector& noise,
    int bufferSize, AnomalyType selectedAnomaly,
    int spikeStart, float spikeMagnitude,
    int stuckStart, int stuckDuration,
    float driftRate, int driftStart, int driftDuration) {

    gen.reset();
    signal.clear();
    cleanSignal.clear();
    isAnomaly.clear();
    float stuckValue = 0.0f;

    for (int i = 0; i < bufferSize; i++) {
        float clean = gen.getNextSample();
        cleanSignal.push_back(clean);

        float noisy = noise.apply(clean);

        bool spikeThisSample = (selectedAnomaly == AnomalyType::Spike) && (i == spikeStart);
        noisy = noise.applySpike(noisy, spikeThisSample, spikeMagnitude);

        bool isStuck = (selectedAnomaly == AnomalyType::Stuck) && (i >= stuckStart) && (i < stuckStart + stuckDuration);
        if (isStuck && i == stuckStart) {
            stuckValue = noisy;
        }
        noisy = noise.applyStuck(noisy, isStuck, stuckValue);

        bool isDrifting = (selectedAnomaly == AnomalyType::Drift) && (i >= driftStart) && (i < driftStart + driftDuration);
        int samplesSinceDriftStart = isDrifting ? (i - driftStart) : 0;
        noisy = noise.applyDrift(noisy, isDrifting, driftRate, samplesSinceDriftStart);

        bool anomalousSample = spikeThisSample || isStuck || isDrifting;

        signal.push_back(noisy);
        isAnomaly.push_back(anomalousSample);
    }
}

void randomizeAnomaly(std::mt19937& randomGen, int bufferSize, AnomalyType& selectedAnomaly,
    int& spikeStart, float& spikeMagnitude,
    int& stuckStart, int& stuckDuration,
    float& driftRate, int& driftStart, int& driftDuration) {

    std::uniform_int_distribution<int> anomalyType(0, 3);
    selectedAnomaly = (AnomalyType)anomalyType(randomGen);

    std::uniform_int_distribution<int> anomalyStart(0, bufferSize - 1);
    int randomStart = anomalyStart(randomGen);

    spikeStart = randomStart;
    stuckStart = randomStart;
    driftStart = randomStart;

    std::uniform_real_distribution<float> anomalyMagnitude(0.5f, 10.0f);
    spikeMagnitude = anomalyMagnitude(randomGen);

    std::uniform_int_distribution<int> anomalyDuration(50, 2000);
    stuckDuration = anomalyDuration(randomGen);

    std::uniform_real_distribution<float> anomalyRate(0.0001f, 0.002f);
    driftRate = anomalyRate(randomGen);

	std::uniform_int_distribution<int> driftDurationDist(100, 1000);
	driftDuration = driftDurationDist(randomGen);
}

void randomizeWave(std::mt19937& randomGen, float& frequency, float& amplitude, float& phase, float& noiseStdDev) {
	std::uniform_real_distribution<float> freqDist(0.5f, 5.0f);
	frequency = freqDist(randomGen);

	std::uniform_real_distribution<float> ampDist(0.5f, 3.0f);
	amplitude = ampDist(randomGen);

	std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
	phase = phaseDist(randomGen);

	std::uniform_real_distribution<float> noiseDist(0.02f, 1.0f);
	noiseStdDev = noiseDist(randomGen);
}