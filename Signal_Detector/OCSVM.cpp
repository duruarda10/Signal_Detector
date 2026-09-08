#include "OCSVM.h"
#include <fstream>
#include <algorithm>

static sample_type toSample(const NormalizedFeatureVector& nfv) {
    sample_type m;
    for (int j = 0; j < 6; j++) m(j) = nfv.values[j];
    m(6) = std::clamp((double)nfv.values[6], -5.0, 5.0);
    return m;
}

dlib::decision_function<kernel_type> trainGlobalOCSVM(const std::vector<FeatureVector>& trainingFeatures, const NormalizerStats& stats, double nu, double gamma, float residualWeight) {
    std::vector<NormalizedFeatureVector> normalized = Normalizer::apply(trainingFeatures, stats, residualWeight);

    std::vector<sample_type> samples;
    samples.reserve(normalized.size());
    for (auto& nfv : normalized) samples.push_back(toSample(nfv));

    dlib::svm_one_class_trainer<kernel_type> trainer;
    trainer.set_nu(std::clamp(nu, 0.001, 0.5));
    trainer.set_kernel(kernel_type(gamma));

    return trainer.train(samples);
}

std::vector<double> scoreWithModel(const std::vector<FeatureVector>& features, const NormalizerStats& stats, const dlib::decision_function<kernel_type>& model, float residualWeight) {
    std::vector<NormalizedFeatureVector> normalized = Normalizer::apply(features, stats, residualWeight);

    std::vector<double> scores(normalized.size());
    for (size_t i = 0; i < normalized.size(); i++) {
        scores[i] = model(toSample(normalized[i]));
    }
    return scores;
}

void saveOCSVMModel(const std::string& path, const dlib::decision_function<kernel_type>& model, const NormalizerStats& stats) {
    std::ofstream out(path, std::ios::binary);
    dlib::serialize(model, out);
    out.write(reinterpret_cast<const char*>(&stats), sizeof(NormalizerStats));
}

bool loadOCSVMModel(const std::string& path, dlib::decision_function<kernel_type>& model, NormalizerStats& stats) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    dlib::deserialize(model, in);
    in.read(reinterpret_cast<char*>(&stats), sizeof(NormalizerStats));
    return true;
}