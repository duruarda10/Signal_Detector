#pragma once
#include <vector>

struct FeatureVector {
	float rawValue;
	float rollingAverage;
	float rollingStdDev;
	float rateOfChange;
	float zScore;
	float residual;
};

std::vector<FeatureVector> extractFeatures(const std::vector<float>& signal, const std::vector<float>& cleanSignal, int windowSize);