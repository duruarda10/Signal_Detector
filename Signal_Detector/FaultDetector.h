#pragma once
#include <vector>

enum class DetectorMethod {
	Threshold,
	MovingAverage,
	ZScore,
	Rate
};

class FaultDetector {
public:
	bool checkThreshold(float value, float minVal, float maxVal);
	bool checkMovingAverage(const std::vector<float>& signal, int index, int windowSize, float threshold);
	bool checkZScore(const std::vector<float>& signal, int index, int windowSize, float zThreshold);
	bool checkRateSpike(const std::vector<float>& signal, int index, float rateThreshold);
	bool checkRateStuck(const std::vector<float>& signal, int index, int windowSize, float rateThreshold);
};
