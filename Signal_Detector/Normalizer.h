#pragma once
#include <vector>
#include "FeatureExtractor.h"

struct NormalizedFeatureVector {
    float values[5];
};

class Normalizer {
public:
    std::vector<NormalizedFeatureVector> normalize(const std::vector<FeatureVector>& features);
};