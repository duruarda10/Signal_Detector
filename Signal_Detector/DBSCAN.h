#pragma once
#include <vector>
#include "FeatureExtractor.h"
#include "Normalizer.h"

std::vector<int> runDBSCAN(const std::vector<FeatureVector>& features, float epsilon, int minPts, float residualWeight = 1.0f);