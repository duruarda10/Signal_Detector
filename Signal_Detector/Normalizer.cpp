#include <vector>
#include <cmath>
#include "Normalizer.h"

std::vector<NormalizedFeatureVector> Normalizer::normalize(const std::vector<FeatureVector>& features) {
	int n = (int)features.size();
	std::vector<NormalizedFeatureVector> result(n);

	float averages[5] = { 0.0f };
	for (int i = 0; i < n; i++) {
		averages[0] += features[i].rawValue;
		averages[1] += features[i].rollingAverage;
		averages[2] += features[i].rollingStdDev;
		averages[3] += features[i].rateOfChange;
		averages[4] += features[i].zScore;
	}

	for (int j = 0; j < 5; j++) averages[j] /= n;

	float stdDevs[5] = { 0.0f };
	for (int i = 0; i < n; i++) {
		stdDevs[0] += std::pow(features[i].rawValue - averages[0], 2);
		stdDevs[1] += std::pow(features[i].rollingAverage - averages[1], 2);
		stdDevs[2] += std::pow(features[i].rollingStdDev - averages[2], 2);
		stdDevs[3] += std::pow(features[i].rateOfChange - averages[3], 2);
		stdDevs[4] += std::pow(features[i].zScore - averages[4], 2);
	}

	for (int j = 0; j < 5; j++) {
		stdDevs[j] = std::sqrt(stdDevs[j] / n);
		if (stdDevs[j] < 0.0001f) stdDevs[j] = 1.0f;
	}

	for (int i = 0; i < n; i++) {
		result[i].values[0] = (features[i].rawValue - averages[0]) / stdDevs[0];
		result[i].values[1] = (features[i].rollingAverage - averages[1]) / stdDevs[1];
		result[i].values[2] = (features[i].rollingStdDev - averages[2]) / stdDevs[2];
		result[i].values[3] = (features[i].rateOfChange - averages[3]) / stdDevs[3];
		result[i].values[4] = (features[i].zScore - averages[4]) / stdDevs[4];
	}
	return result;
}