#include "OCSVM.h"
#include "Normalizer.h"

#include <dlib/svm.h>
#include <map>

typedef dlib::matrix<double, 6, 1> sample_type;
typedef dlib::radial_basis_kernel<sample_type> kernel_type;

std::vector<double> runOCSVMPerCluster(const std::vector<FeatureVector>& features, const std::vector<int>& clusterLabels, double nu, double gamma, float residualWeight) {
	Normalizer normalizer;
	std::vector<NormalizedFeatureVector> normalized = normalizer.normalize(features, residualWeight);

	int n = (int)normalized.size();
	std::vector<sample_type> samples(n);
	for (int i = 0; i < n; i++) {
		sample_type m;
		for (int j = 0; j < 6; j++) {
			m(j) = normalized[i].values[j];
		}
		samples[i] = m;
	}
	std::vector<double> scores(n, -999.0);

	std::map<int, std::vector<int>> clusterToIndices;
	for (int i = 0; i < n; i++) {
		if (clusterLabels[i] != -1) {
			clusterToIndices[clusterLabels[i]].push_back(i);
		}	
	}

	for (auto& entry : clusterToIndices) {
		std::vector<int>& indices = entry.second;

		if ((int)indices.size() < 6) {
			for (int idx : indices) scores[idx] = -999.0;
			continue;
		}

		std::vector<sample_type> clusterSamples;
		for (int idx : indices) clusterSamples.push_back(samples[idx]);
	
		dlib::svm_one_class_trainer<kernel_type> trainer;
		trainer.set_nu(nu);
		trainer.set_kernel(kernel_type(gamma));

		dlib::decision_function<kernel_type> df = trainer.train(clusterSamples);
		
		for (int idx : indices) scores[idx] = df(samples[idx]);
	}
	return scores;
}
