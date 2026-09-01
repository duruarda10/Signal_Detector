#include "FaultDetector.h"
#include <cmath>

bool FaultDetector::checkThreshold(float value, float minVal, float maxVal) {
	return (value < minVal || value > maxVal);
}	

bool FaultDetector::checkMovingAverage(const std::vector<float>& signal, int index, int windowSize, float threshold) {
	if (index < windowSize) return false;

	float sum = 0.0f;
	for (int i = index - windowSize; i < index; i++) {
		sum += signal[i];
	}
	float rollingAverage = sum / windowSize;

	float deviation = std::abs(signal[index] - rollingAverage);
	return (deviation > threshold);
}