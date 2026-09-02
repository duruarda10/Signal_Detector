#include "DBSCAN.h"
#include <cmath>

float distanceSquared(const NormalizedFeatureVector& a, const NormalizedFeatureVector& b) {
	float sum = 0.0f;
	for (int i = 0; i < 5; i++) {
		float diff = a.values[i] - b.values[i];
		sum += diff * diff;
	}
	return sum;
}

std::vector<int> findNeighbors(const std::vector<NormalizedFeatureVector>& points, int pointIndex, float epsilon) {
	std::vector<int> neighbors;
	float epsSquared = epsilon * epsilon;

	for (int i = 0 ; i < (int)points.size(); i++) {
		if (i != pointIndex && distanceSquared(points[pointIndex], points[i]) <= epsSquared) {
			neighbors.push_back(i);
		}
	}
	return neighbors;
}

std::vector<int> runDBSCAN(const std::vector<FeatureVector>& features, float epsilon, int minPts) {
    Normalizer normalizer;
    std::vector<NormalizedFeatureVector> points = normalizer.normalize(features);

    int n = (int)points.size();
    std::vector<int> labels(n, -1);
    std::vector<bool> visited(n, false);
    int currentCluster = 0;

    for (int i = 0; i < n; i++) {
        if (visited[i]) continue;
        visited[i] = true;

        std::vector<int> neighbors = findNeighbors(points, i, epsilon);

        if ((int)neighbors.size() < minPts) {
            labels[i] = -1;
            continue;
        }

        labels[i] = currentCluster;
        std::vector<bool> inSeedSet(n, false);
        inSeedSet[i] = true;
        for (int idx : neighbors) inSeedSet[idx] = true;

        for (int j = 0; j < (int)neighbors.size(); j++) {
            int neighborIndex = neighbors[j];

            if (!visited[neighborIndex]) {
                visited[neighborIndex] = true;
                std::vector<int> neighborNeighbors = findNeighbors(points, neighborIndex, epsilon);

                if ((int)neighborNeighbors.size() >= minPts) {
                    for (int k = 0; k < (int)neighborNeighbors.size(); k++) {
                        int candidate = neighborNeighbors[k];
                        if (!inSeedSet[candidate]) {
                            inSeedSet[candidate] = true;
                            neighbors.push_back(candidate);
                        }
                    }
                }
            }

            if (labels[neighborIndex] == -1) {
                labels[neighborIndex] = currentCluster;
            }
        }

        currentCluster++;
    }

    return labels;
}
