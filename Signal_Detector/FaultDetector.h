#pragma once
#include <vector>

enum class DetectorMethod {
	Threshold,
	MovingAverage,
	ZScore
};

class FaultDetector {
public:
	bool checkThreshold(float value, float minVal, float maxVal);
	bool checkMovingAverage(const std::vector<float>& signal, int index, int windowSize, float threshold);
	bool checkZScore(const std::vector<float>& signal, int index, int windowSize, float zThreshold);
};
