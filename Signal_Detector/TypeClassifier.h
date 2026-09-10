#pragma once
#include "FeatureExtractor.h"
#include "Normalizer.h"
#include "OCSVM.h"

#include <vector>
#include <string>
#include <dlib/svm.h>

struct LabeledSample {
    FeatureVector feature;
    unsigned long label; // 1 = Spike, 2 = Stuck, 3 = Drift
};

struct TypeClassifierModel {
    dlib::decision_function<kernel_type> spikeModel;
    dlib::decision_function<kernel_type> stuckModel;
    dlib::decision_function<kernel_type> driftModel;
    double spikeOcsvmThreshold;
    double stuckOcsvmThreshold;
    double driftOcsvmThreshold;
};

TypeClassifierModel trainTypeClassifier(const std::vector<LabeledSample>& labeledData, const NormalizerStats& stats, const dlib::decision_function<kernel_type>& globalOcsvmModel, double nu, double gamma, float residualWeight);
unsigned long predictType(const FeatureVector& feature, const NormalizerStats& stats, const TypeClassifierModel& model, float residualWeight);
void saveTypeClassifier(const std::string& path, const TypeClassifierModel& model);
bool loadTypeClassifier(const std::string& path, TypeClassifierModel& model);
double thresholdForType(const TypeClassifierModel& model, unsigned long label, double fallbackThreshold);

const char* typeLabelToString(unsigned long label);