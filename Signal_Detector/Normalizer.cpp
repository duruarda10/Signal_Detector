#include <vector>
#include <cmath>
#include "Normalizer.h"

NormalizerStats Normalizer::fit(const std::vector<FeatureVector>& features) {
    int n = (int)features.size();
    NormalizerStats stats{};

    for (int i = 0; i < n; i++) {
        stats.means[0] += features[i].rawValue;
        stats.means[1] += features[i].rollingAverage;
        stats.means[2] += features[i].rollingStdDev;
        stats.means[3] += features[i].rateOfChange;
        stats.means[4] += features[i].zScore;
        stats.means[5] += features[i].residual;
        stats.means[6] += features[i].firstDifference;
    }
    for (int j = 0; j < 7; j++) stats.means[j] /= n;

    for (int i = 0; i < n; i++) {
        stats.stdDevs[0] += (features[i].rawValue - stats.means[0]) * (features[i].rawValue - stats.means[0]);
        stats.stdDevs[1] += (features[i].rollingAverage - stats.means[1]) * (features[i].rollingAverage - stats.means[1]);
        stats.stdDevs[2] += (features[i].rollingStdDev - stats.means[2]) * (features[i].rollingStdDev - stats.means[2]);
        stats.stdDevs[3] += (features[i].rateOfChange - stats.means[3]) * (features[i].rateOfChange - stats.means[3]);
        stats.stdDevs[4] += (features[i].zScore - stats.means[4]) * (features[i].zScore - stats.means[4]);
        stats.stdDevs[5] += (features[i].residual - stats.means[5]) * (features[i].residual - stats.means[5]);
        stats.stdDevs[6] += (features[i].firstDifference - stats.means[6]) * (features[i].firstDifference - stats.means[6]);
    }
    for (int j = 0; j < 7; j++) {
        stats.stdDevs[j] = std::sqrt(stats.stdDevs[j] / n);
        if (stats.stdDevs[j] < 0.0001f) stats.stdDevs[j] = 1.0f;
    }
    return stats;
}

std::vector<NormalizedFeatureVector> Normalizer::apply(const std::vector<FeatureVector>& features, const NormalizerStats& stats, float residualWeight) {
    int n = (int)features.size();
    std::vector<NormalizedFeatureVector> result(n);

    for (int i = 0; i < n; i++) {
        result[i].values[0] = (features[i].rawValue - stats.means[0]) / stats.stdDevs[0];
        result[i].values[1] = (features[i].rollingAverage - stats.means[1]) / stats.stdDevs[1];
        result[i].values[2] = (features[i].rollingStdDev - stats.means[2]) / stats.stdDevs[2];
        result[i].values[3] = (features[i].rateOfChange - stats.means[3]) / stats.stdDevs[3];
        result[i].values[4] = (features[i].zScore - stats.means[4]) / stats.stdDevs[4];
        result[i].values[5] = ((features[i].residual - stats.means[5]) / stats.stdDevs[5]) * residualWeight;
        result[i].values[6] = (features[i].firstDifference - stats.means[6]) / stats.stdDevs[6];
    }
    return result;
}