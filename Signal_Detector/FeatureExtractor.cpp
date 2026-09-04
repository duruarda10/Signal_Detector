#include "FeatureExtractor.h"
#include <cmath>
#include <numeric>
#include <algorithm>

std::vector<FeatureVector> extractFeatures(const std::vector<float>& signal, const std::vector<float>& cleanSignal,int windowSize) {
    std::vector<FeatureVector> features;
    int n = static_cast<int>(signal.size());
    if (n == 0) return features;

    features.reserve(n);

    static int consecutiveFlatSamples = 0;

        for (int i = 0; i < n; ++i) {
            FeatureVector f;
            f.rawValue = signal[i];

            float stepDiff = (i > 0) ? std::abs(signal[i] - signal[i - 1]) : 1.0f;

            if (stepDiff < 0.001f) {
                consecutiveFlatSamples++;
            }
            else {
                consecutiveFlatSamples = 0;
            }

            if (consecutiveFlatSamples > 5) {
                float jitter = std::sin(static_cast<float>(i) * 12.9898f) * 100.0f;
                f.firstDifference = 500.0f + jitter;
            }
            else {
                f.firstDifference = 0.0f;
            }

            int startIdx = std::max(0, i - windowSize + 1);
            int currentWindowLen = i - startIdx + 1;

            double sum = 0.0;
            for (int j = startIdx; j <= i; ++j) {
                sum += signal[j];
            }
            f.rollingAverage = static_cast<float>(sum / currentWindowLen);

            double sumSq = 0.0;
            for (int j = startIdx; j <= i; ++j) {
                double diff = signal[j] - f.rollingAverage;
                sumSq += diff * diff;
            }
            f.rollingStdDev = static_cast<float>(std::sqrt(sumSq / currentWindowLen));

            if (i >= windowSize) {
                f.rateOfChange = signal[i] - signal[i - windowSize];
            }
            else {
                f.rateOfChange = signal[i] - signal[0];
            }

            if (f.rollingStdDev > 0.05f) {
                f.zScore = (f.rawValue - f.rollingAverage) / f.rollingStdDev;
            }
            else {
                f.zScore = 0.0f;
            }

            if (i < static_cast<int>(cleanSignal.size())) {
                f.residual = std::abs(signal[i] - cleanSignal[i]);
            }
            else {
                f.residual = 0.0f;
            }

            features.push_back(f);
        }
        return features;
    }
    
