#include "OCSVM.h"
#include <dlib/svm.h>

typedef dlib::matrix<double, 5, 1> sample_type;
typedef dlib::radial_basis_kernel<sample_type> kernel_type;

std::vector<double> OCSVM(const std::vector<FeatureVector>& features, double nu, double gamma) {
	Normalizer normalizer;
	std::vector<NormalizedFeatureVector> normalized = normalizer.normalize(features);

	std::vector<sample_type> samples;
	for (int i = 0; i < (int)normalized.size(); i++) {
		sample_type m;
		for (int j = 0; j < 5; j++) {
			m(j) = normalized[i].values[j];
		}
		samples.push_back(m);	
	}

	dlib::svm_one_class_trainer<kernel_type> trainer;
	trainer.set_nu(nu);
	trainer.set_kernel(kernel_type(gamma));

	dlib::decision_function<kernel_type> df = trainer.train(samples);

	std::vector<double> scores;
	for (int i = 0; i < (int)samples.size(); i++) {
		scores.push_back(df(samples[i]));
	}

	return scores;
}
