#pragma once
#include <vector>

class FaultDetector {
public:
	bool checkThreshold(float value, float minVal, float maxVal);
	bool checkMovingAverage(const std::vector<float>& signal, int index, int windowSize, float threshold);
};
