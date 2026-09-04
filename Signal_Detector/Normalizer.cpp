#include <vector>
#include <cmath>
#include "Normalizer.h"

std::vector<NormalizedFeatureVector> Normalizer::normalize(const std::vector<FeatureVector>& features, float residualWeight) {
	int n = (int)features.size();
	std::vector<NormalizedFeatureVector> result(n);

	float averages[7] = { 0.0f };
	for (int i = 0; i < n; i++) {
		averages[0] += features[i].rawValue;
		averages[1] += features[i].rollingAverage;
		averages[2] += features[i].rollingStdDev;
		averages[3] += features[i].rateOfChange;
		averages[4] += features[i].zScore;
		averages[5] += features[i].residual;
		averages[6] += features[i].firstDifference;
	}

	for (int j = 0; j < 7; j++) averages[j] /= n;

	float stdDevs[7] = { 0.0f };
	for (int i = 0; i < n; i++) {
		stdDevs[0] += (features[i].rawValue - averages[0]) * (features[i].rawValue - averages[0]);
		stdDevs[1] += (features[i].rollingAverage - averages[1]) * (features[i].rollingAverage - averages[1]);
		stdDevs[2] += (features[i].rollingStdDev - averages[2]) * (features[i].rollingStdDev - averages[2]);
		stdDevs[3] += (features[i].rateOfChange - averages[3]) * (features[i].rateOfChange - averages[3]);
		stdDevs[4] += (features[i].zScore - averages[4]) * (features[i].zScore - averages[4]);
		stdDevs[5] += (features[i].residual - averages[5]) * (features[i].residual - averages[5]);
		stdDevs[6] += (features[i].firstDifference - averages[6]) * (features[i].firstDifference - averages[6]);
	}

	for (int j = 0; j < 7; j++) {
		stdDevs[j] = std::sqrt(stdDevs[j] / n);
		if (stdDevs[j] < 0.0001f) stdDevs[j] = 1.0f;
	}

	for (int i = 0; i < n; i++) {
		result[i].values[0] = (features[i].rawValue - averages[0]) / stdDevs[0];
		result[i].values[1] = (features[i].rollingAverage - averages[1]) / stdDevs[1];
		result[i].values[2] = (features[i].rollingStdDev - averages[2]) / stdDevs[2];
		result[i].values[3] = (features[i].rateOfChange - averages[3]) / stdDevs[3];
		result[i].values[4] = (features[i].zScore - averages[4]) / stdDevs[4];
		result[i].values[5] = ((features[i].residual - averages[5]) / stdDevs[5]) * residualWeight;
		result[i].values[6] = (features[i].firstDifference - averages[6]) / stdDevs[6];
	}
	return result;
}