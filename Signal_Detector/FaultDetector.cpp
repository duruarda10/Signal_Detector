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

bool FaultDetector::checkZScore(const std::vector<float>& signal, int index, int windowSize, float zThreshold) {
	if (index < windowSize) return false;

	float sum = 0.0f;
	for (int i = index - windowSize; i < index; i++) {
		sum += signal[i];
	}
	float mean = sum / windowSize;

	float sumSquaredDeviations = 0.0f;
	for (int i = index - windowSize; i < index; i++) {
		float deviation = signal[i] - mean;
		sumSquaredDeviations += deviation * deviation;
	}
	float stdDev = std::sqrt(sumSquaredDeviations / windowSize);

	if (stdDev < 0.0001f) return false;

	float zScore = (signal[index] - mean) / stdDev;
	return (std::abs(zScore) > zThreshold);
}
