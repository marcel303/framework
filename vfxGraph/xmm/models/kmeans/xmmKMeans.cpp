/*
 * xmmKMeans.hpp
 *
 * K-Means clustering
 *
 * Contact:
 * - Jules Francoise <jules.francoise@ircam.fr>
 *
 * This code has been initially authored by Jules Francoise
 * <http://julesfrancoise.com> during his PhD thesis, supervised by Frederic
 * Bevilacqua <href="http://frederic-bevilacqua.net>, in the Sound Music
 * Movement Interaction team <http://ismm.ircam.fr> of the
 * STMS Lab - IRCAM, CNRS, UPMC (2011-2015).
 *
 * Copyright (C) 2015 UPMC, Ircam-Centre Pompidou.
 *
 * This File is part of XMM.
 *
 * XMM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XMM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XMM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xmmKMeans.hpp"
#include <limits>

#ifdef WIN32
static long random() { return rand(); }
#endif
#define kmax(a, b) (((a) > (b)) ? (a) : (b))

#pragma mark -
#pragma mark === Public Interface ===
#pragma mark > Constructors
xmm::KMeans::KMeans(unsigned int clusters_)
    : shared_parameters(std::make_shared<SharedParameters>()),
      initialization_mode(InitializationMode::Random) {
    configuration.clusters.set(clusters_);
}

xmm::KMeans::KMeans(KMeans const& src)
    : configuration(src.configuration),
      centers(src.centers),
      initialization_mode(src.initialization_mode) {}

xmm::KMeans& xmm::KMeans::operator=(KMeans const& src) {
    if (this != &src) {
        initialization_mode = src.initialization_mode;
        configuration = src.configuration;
        centers = src.centers;
    }
    return *this;
}

#pragma mark > Training
void xmm::KMeans::train(TrainingSet* trainingSet) {
    if (!trainingSet || trainingSet->empty()) return;

    shared_parameters->dimension.set(trainingSet->dimension.get());
    centers.resize(
        configuration.clusters.get() * shared_parameters->dimension.get(), 0.0);
    if (initialization_mode == InitializationMode::Random)
        randomizeClusters(trainingSet->standardDeviation());
    else
        initClustersWithFirstPhrase(trainingSet->begin()->second);

    int dimension = static_cast<int>(shared_parameters->dimension.get());
    int clusters = static_cast<int>(configuration.clusters.get());

    for (int trainingNbIterations = 0;
         trainingNbIterations < configuration.max_iterations.get();
         ++trainingNbIterations) {
        std::vector<float> previous_centers = centers;

        updateCenters(previous_centers, trainingSet);

        float meanClusterDistance(0.0);
        float maxRelativeCenterVariation(0.0);
        for (unsigned int k = 0; k < clusters; ++k) {
            for (unsigned int l = 0; l < clusters; ++l) {
                if (k != l) {
                    meanClusterDistance +=
                        euclidean_distance(&centers[k * dimension],
                                           &centers[l * dimension], dimension);
                }
            }
            maxRelativeCenterVariation =
                kmax(euclidean_distance(&previous_centers[k * dimension],
                                        &centers[k * dimension], dimension),
                     maxRelativeCenterVariation);
        }
        meanClusterDistance /= float(clusters * (clusters - 1));
        maxRelativeCenterVariation /= float(clusters);
        maxRelativeCenterVariation /= meanClusterDistance;
        if (maxRelativeCenterVariation <
            configuration.relative_distance_threshold.get())
            break;
    }
}

void xmm::KMeans::initClustersWithFirstPhrase(std::shared_ptr<Phrase> phrase) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int step = phrase->size() / configuration.clusters.get();

    unsigned int offset(0);
    for (unsigned int c = 0; c < configuration.clusters.get(); c++) {
        for (unsigned int d = 0; d < dimension; d++) {
            centers[c * dimension + d] = 0.0;
        }
        for (unsigned int t = 0; t < step; t++) {
            for (unsigned int d = 0; d < dimension; d++) {
                centers[c * dimension + d] +=
                    phrase->getValue(offset + t, d) / float(step);
            }
        }
        offset += step;
    }
}

void xmm::KMeans::randomizeClusters(
    std::vector<float> const& trainingSetVariance) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    for (unsigned int k = 0; k < configuration.clusters.get(); ++k) {
        for (unsigned int d = 0; d < dimension; ++d) {
            centers[k * dimension + d] =
                trainingSetVariance[d] * (2. * random() / float(RAND_MAX) - 1.);
        }
    }
}

void xmm::KMeans::updateCenters(std::vector<float>& previous_centers,
                                TrainingSet* trainingSet) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    int clusters = static_cast<int>(configuration.clusters.get());

    unsigned int phraseIndex(0);
    centers.assign(clusters * dimension, 0.0);
    std::vector<unsigned int> numFramesPerCluster(clusters, 0);
    for (auto it = trainingSet->begin(); it != trainingSet->end();
         ++it, ++phraseIndex) {
        for (unsigned int t = 0; t < it->second->size(); ++t) {
            float min_distance;
            if (trainingSet->bimodal()) {
                std::vector<float> frame(dimension);
                for (unsigned int d = 0; d < dimension; ++d) {
                    frame[d] = it->second->getValue(t, d);
                }
                min_distance = euclidean_distance(
                    &frame[0], &previous_centers[0], dimension);
            } else {
                min_distance = euclidean_distance(
                    it->second->getPointer(t), &previous_centers[0], dimension);
            }
            unsigned int cluster_membership(0);
            for (unsigned int k = 1; k < clusters; ++k) {
                float distance;
                if (trainingSet->bimodal()) {
                    std::vector<float> frame(dimension);
                    for (unsigned int d = 0; d < dimension; ++d) {
                        frame[d] = it->second->getValue(t, d);
                    }
                    distance = euclidean_distance(
                        &frame[0], &previous_centers[k * dimension], dimension);
                } else {
                    distance = euclidean_distance(
                        it->second->getPointer(t),
                        &previous_centers[k * dimension], dimension);
                }
                if (distance < min_distance) {
                    cluster_membership = k;
                    min_distance = distance;
                }
            }
            numFramesPerCluster[cluster_membership]++;
            for (unsigned int d = 0; d < dimension; ++d) {
                centers[cluster_membership * dimension + d] +=
                    it->second->getValue(t, d);
            }
        }
    }
    for (unsigned int k = 0; k < clusters; ++k) {
        if (numFramesPerCluster[k] > 0)
            for (unsigned int d = 0; d < dimension; ++d) {
                centers[k * dimension + d] /= float(numFramesPerCluster[k]);
            }
    }
}

#pragma mark > Performance
void xmm::KMeans::reset() {
    results.distances.resize(configuration.clusters.get(), 0.0);
}

void xmm::KMeans::filter(std::vector<float> const& observation) {
    if (observation.size() != shared_parameters->dimension.get())
        throw std::runtime_error("Observation has wrong dimension");
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    results.likeliest = 0;
    float minDistance(std::numeric_limits<float>::max());
    for (unsigned int k = 0; k < configuration.clusters.get(); ++k) {
        results.distances[k] = euclidean_distance(
            &observation[0], &centers[k * dimension], dimension);
        if (results.distances[k] < minDistance) {
            minDistance = results.distances[k];
            results.likeliest = k;
        }
    }
}

#pragma mark -
#pragma mark File IO
Json::Value xmm::KMeans::toJson() const {
    //    JSONNode json_model(JSON_NODE);
    //    json_model.set_name("KMeans");
    //
    //    json_model.push_back(JSONNode("dimension", dimension_));
    //    json_model.push_back(JSONNode("nbclusters", nbClusters_));
    //    json_model.push_back(vector2json(centers, "centers"));

    return Json::Value();
}

void xmm::KMeans::fromJson(Json::Value const& root) {
    //    try {
    //        if (root.type() != JSON_NODE)
    //            throw JSONException("Wrong type: was expecting 'JSON_NODE'",
    //            root.name());
    //        JSONNode::const_iterator root_it = root.begin();
    //
    //        // Get Dimension
    //        root_it = root.find("dimension");
    //        if (root_it == root.end())
    //            throw JSONException("JSON Node is incomplete",
    //            root_it->name());
    //        if (root_it->type() != JSON_NUMBER)
    //            throw JSONException("Wrong type for node 'dimension': was
    //            expecting 'JSON_NUMBER'", root_it->name());
    //        dimension_ = static_cast<unsigned int>(root_it->as_int());
    //
    //        // Get Number of Clusters
    //        root_it = root.find("nbclusters");
    //        if (root_it == root.end())
    //            throw JSONException("JSON Node is incomplete",
    //            root_it->name());
    //        if (root_it->type() != JSON_NUMBER)
    //            throw JSONException("Wrong type for node 'nbclusters': was
    //            expecting 'JSON_NUMBER'", root_it->name());
    //        nbClusters_ = static_cast<unsigned int>(root_it->as_int());
    //
    //        // Get Cluster Centers
    //        root_it = root.find("centers");
    //        if (root_it == root.end())
    //            throw JSONException("JSON Node is incomplete",
    //            root_it->name());
    //        if (root_it->type() != JSON_ARRAY)
    //            throw JSONException("Wrong type for node 'centers': was
    //            expecting 'JSON_ARRAY'", root_it->name());
    //        json2vector(*root_it, centers, nbClusters_);
    //    } catch (JSONException &e) {
    //        throw JSONException(e, root.name());
    //    } catch (std::exception &e) {
    //        throw JSONException(e, root.name());
    //    }
}

#pragma mark > Utility
template <typename T>
T xmm::euclidean_distance(const T* vector1, const T* vector2,
                          unsigned int dimension) {
    T distance(0.0);
    for (unsigned int d = 0; d < dimension; d++) {
        distance += (vector1[d] - vector2[d]) * (vector1[d] - vector2[d]);
    }
    return sqrt(distance);
}
