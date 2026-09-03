#pragma once
#include <vector>
#include "FeatureExtractor.h"
#include "Normalizer.h"

std::vector<double> OCSVM(const std::vector<FeatureVector>& features, double nu, double gamma);
