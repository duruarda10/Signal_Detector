#pragma once
#include <vector>

struct FeatureVector {
	float rawValue = 0.0f;
	float rollingAverage = 0.0f;
	float rollingStdDev = 0.0f;
	float rateOfChange = 0.0f;
	float zScore = 0.0f;
	float residual = 0.0f;
	float firstDifference = 0.0f;
};

std::vector<FeatureVector> extractFeatures(const std::vector<float>& signal, const std::vector<float>& cleanSignal, int windowSize);