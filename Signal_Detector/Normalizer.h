#pragma once
#include <vector>
#include "FeatureExtractor.h"

struct NormalizedFeatureVector {
    float values[7];
};

struct NormalizerStats {
    float means[7];
    float stdDevs[7];
};

class Normalizer {
public:
    static NormalizerStats fit(const std::vector<FeatureVector>& features);
    static std::vector<NormalizedFeatureVector> apply(const std::vector<FeatureVector>& features, const NormalizerStats& stats, float residualWeight);
};