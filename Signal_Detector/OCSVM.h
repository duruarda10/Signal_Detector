#pragma once
#include <vector>
#include <string>
#include <dlib/svm.h>
#include "FeatureExtractor.h"
#include "Normalizer.h"

typedef dlib::matrix<double, 7, 1> sample_type;
typedef dlib::radial_basis_kernel<sample_type> kernel_type;

dlib::decision_function<kernel_type> trainGlobalOCSVM(const std::vector<FeatureVector>& trainingFeatures, const NormalizerStats& stats, double nu, double gamma, float residualWeight);
std::vector<double> scoreWithModel(const std::vector<FeatureVector>& features, const NormalizerStats& stats, const dlib::decision_function<kernel_type>& model, float residualWeight);
void saveOCSVMModel(const std::string& path, const dlib::decision_function<kernel_type>& model, const NormalizerStats& stats);
bool loadOCSVMModel(const std::string& path, dlib::decision_function<kernel_type>& model, NormalizerStats& stats);