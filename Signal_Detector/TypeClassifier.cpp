#include "TypeClassifier.h"
#include <fstream>
#include <algorithm>

static sample_type toSampleNormalized(const FeatureVector& feature, const NormalizerStats& stats, float residualWeight) {
    std::vector<FeatureVector> single = { feature };
    std::vector<NormalizedFeatureVector> normalized = Normalizer::apply(single, stats, residualWeight);
    sample_type m;
    for (int j = 0; j < 6; j++) m(j) = normalized[0].values[j];
    m(6) = std::clamp((double)normalized[0].values[6], -5.0, 5.0);
    return m;
}

static dlib::decision_function<kernel_type> trainOneClass(const std::vector<sample_type>& samples, double nu, double gamma) {
    dlib::svm_one_class_trainer<kernel_type> trainer;
    trainer.set_nu(nu);
    trainer.set_kernel(kernel_type(gamma));
    return trainer.train(samples);
}

static double computeOcsvmThresholdForType(const std::vector<sample_type>& samples, const dlib::decision_function<kernel_type>& globalOcsvmModel) {
    std::vector<double> scores;
    scores.reserve(samples.size());
    for (auto& s : samples) scores.push_back(globalOcsvmModel(s));
    std::sort(scores.begin(), scores.end());

    size_t idx = (size_t)(scores.size() * 0.7);
    if (idx >= scores.size()) idx = scores.size() - 1;
    return scores[idx];
}

TypeClassifierModel trainTypeClassifier(const std::vector<LabeledSample>& labeledData, const NormalizerStats& stats, const dlib::decision_function<kernel_type>& globalOcsvmModel, double nu, double gamma, float residualWeight) {
    std::vector<sample_type> spikeSamples, stuckSamples, driftSamples;

    for (auto& entry : labeledData) {
        sample_type s = toSampleNormalized(entry.feature, stats, residualWeight);
        if (entry.label == 1) spikeSamples.push_back(s);
        else if (entry.label == 2) stuckSamples.push_back(s);
        else if (entry.label == 3) driftSamples.push_back(s);
    }

    TypeClassifierModel model;
    model.spikeModel = trainOneClass(spikeSamples, nu, gamma);
    model.stuckModel = trainOneClass(stuckSamples, nu, gamma);
    model.driftModel = trainOneClass(driftSamples, nu, gamma);

    model.spikeOcsvmThreshold = computeOcsvmThresholdForType(spikeSamples, globalOcsvmModel);
    model.stuckOcsvmThreshold = computeOcsvmThresholdForType(stuckSamples, globalOcsvmModel);
    model.driftOcsvmThreshold = computeOcsvmThresholdForType(driftSamples, globalOcsvmModel);

    return model;
}

unsigned long predictType(const FeatureVector& feature, const NormalizerStats& stats, const TypeClassifierModel& model, float residualWeight) {
    sample_type sample = toSampleNormalized(feature, stats, residualWeight);

    double spikeScore = model.spikeModel(sample);
    double stuckScore = model.stuckModel(sample);
    double driftScore = model.driftModel(sample);

    if (spikeScore >= stuckScore && spikeScore >= driftScore) return 1;
    if (stuckScore >= spikeScore && stuckScore >= driftScore) return 2;
    return 3;
}

double thresholdForType(const TypeClassifierModel& model, unsigned long label, double fallbackThreshold) {
    switch (label) {
    case 1: return model.spikeOcsvmThreshold;
    case 2: return model.stuckOcsvmThreshold;
    case 3: return model.driftOcsvmThreshold;
    default: return fallbackThreshold;
    }
}

void saveTypeClassifier(const std::string& path, const TypeClassifierModel& model) {
    std::ofstream out(path, std::ios::binary);
    dlib::serialize(model.spikeModel, out);
    dlib::serialize(model.stuckModel, out);
    dlib::serialize(model.driftModel, out);
    out.write(reinterpret_cast<const char*>(&model.spikeOcsvmThreshold), sizeof(double));
    out.write(reinterpret_cast<const char*>(&model.stuckOcsvmThreshold), sizeof(double));
    out.write(reinterpret_cast<const char*>(&model.driftOcsvmThreshold), sizeof(double));
}

bool loadTypeClassifier(const std::string& path, TypeClassifierModel& model) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    dlib::deserialize(model.spikeModel, in);
    dlib::deserialize(model.stuckModel, in);
    dlib::deserialize(model.driftModel, in);
    in.read(reinterpret_cast<char*>(&model.spikeOcsvmThreshold), sizeof(double));
    in.read(reinterpret_cast<char*>(&model.stuckOcsvmThreshold), sizeof(double));
    in.read(reinterpret_cast<char*>(&model.driftOcsvmThreshold), sizeof(double));
    return true;
}

const char* typeLabelToString(unsigned long label) {
    switch (label) {
    case 1: return "Spike";
    case 2: return "Stuck";
    case 3: return "Drift";
    default: return "Unknown";
    }
}