#pragma once
#include <vector>
#include "FeatureExtractor.h"

struct NormalizedFeatureVector {
    float values[6];
};

class Normalizer {
public:
    std::vector<NormalizedFeatureVector> normalize(const std::vector<FeatureVector>& features, float residualWeight = 1.0f);
};