#pragma once
#include <vector>
#include "FeatureExtractor.h"

std::vector<double> runOCSVMPerCluster(const std::vector<FeatureVector>& features, const std::vector<int>& clusterLabels, double nu, double gamma, float residualWeight = 1.0f);
