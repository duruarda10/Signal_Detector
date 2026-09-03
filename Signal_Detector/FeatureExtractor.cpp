#include "FeatureExtractor.h"
#include <cmath>

std::vector<FeatureVector> extractFeatures(const std::vector<float>& signal, const std::vector<float>& cleanSignal, int windowSize) {
	std::vector<FeatureVector> features;

	for (int i = 0; i < (int)signal.size(); i++) {
		FeatureVector f;
		f.rawValue = signal[i];
		f.residual = signal[i] - cleanSignal[i];

		if (i < windowSize) {
			f.rollingAverage = signal[i];
			f.rollingStdDev = 0.0f;
			f.rateOfChange = 0.0f;
			f.zScore = 0.0f;
		}
		else {
			float sum = 0.0f;
			for (int j = i - windowSize; j < i; j++) {
				sum += signal[i];
			}
			float mean = sum / windowSize;

			float sumSquaredDiff = 0.0f;
			for (int j = i - windowSize; j < i; j++) {
				float diff = signal[j] - mean;
				sumSquaredDiff += diff * diff;
			}
			float stdDev = std::sqrt(sumSquaredDiff / windowSize);

			float rate = signal[i] - signal[i - 1];

			float zScore = 0.0f;
			if (stdDev > 0.0001f) {
				zScore = (signal[i] - mean) / stdDev;
			}
			f.rollingAverage = mean;
			f.rollingStdDev = stdDev;
			f.rateOfChange = rate;
			f.zScore = zScore;
		}
		features.push_back(f);
	}
	return features;
}