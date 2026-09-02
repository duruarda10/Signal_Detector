#pragma once
#include <vector>

struct FeatureVector {
	float rawValue;
	float rollingAverage;
	float rollingStdDev;
	float rateOfChange;
	float zScore;
};

std::vector<FeatureVector> extractFeatures(const std::vector<float>& signal, int windowSize);