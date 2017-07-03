/*
 * xmmGmmSingleClass.cpp
 *
 * Gaussian Mixture Model Definition for a Single Class
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

#include "../kmeans/xmmKMeans.hpp"
#include "xmmGmmSingleClass.hpp"
#include <algorithm>

xmm::SingleClassGMM::SingleClassGMM(std::shared_ptr<SharedParameters> p)
    : SingleClassProbabilisticModel(p) {}

xmm::SingleClassGMM::SingleClassGMM(SingleClassGMM const& src)
    : SingleClassProbabilisticModel(src),
      parameters(src.parameters),
      components(src.components),
      mixture_coeffs(src.mixture_coeffs) {
    beta.resize(parameters.gaussians.get());
}

xmm::SingleClassGMM::SingleClassGMM(std::shared_ptr<SharedParameters> p,
                                    Json::Value const& root)
    : SingleClassProbabilisticModel(p, root) {
    parameters.fromJson(root["parameters"]);

    allocate();

    json2vector(root["mixture_coeffs"], mixture_coeffs,
                parameters.gaussians.get());

    int c(0);
    for (auto p : root["components"]) {
        components[c++].fromJson(p);
    }

    updateInverseCovariances();
}

xmm::SingleClassGMM& xmm::SingleClassGMM::operator=(SingleClassGMM const& src) {
    if (this != &src) {
        SingleClassProbabilisticModel::operator=(src);
        parameters = src.parameters;
        beta.resize(parameters.gaussians.get());
        mixture_coeffs = src.mixture_coeffs;
        components = src.components;
    }
    return *this;
};

void xmm::SingleClassGMM::reset() { SingleClassProbabilisticModel::reset(); }

double xmm::SingleClassGMM::filter(std::vector<float> const& observation) {
    check_training();
    double instantaneous_likelihood = likelihood(observation);
    if (shared_parameters->bimodal.get()) {
        regression(observation);
    }
    return instantaneous_likelihood;
}

void xmm::SingleClassGMM::emAlgorithmInit(TrainingSet* trainingSet) {
    initParametersToDefault(trainingSet->standardDeviation());
    initMeansWithKMeans(trainingSet);
    initCovariances_fullyObserved(trainingSet);
    addCovarianceOffset();
    updateInverseCovariances();
}

Json::Value xmm::SingleClassGMM::toJson() const {
    check_training();
    Json::Value root = SingleClassProbabilisticModel::toJson();
    root["mixture_coeffs"] = vector2json(mixture_coeffs);
    root["parameters"] = parameters.toJson();
    root["components"].resize(
        static_cast<Json::ArrayIndex>(parameters.gaussians.get()));
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        root["components"][c] = components[c].toJson();
    }
    return root;
}

void xmm::SingleClassGMM::fromJson(Json::Value const& root) {
    check_training();
    try {
        SingleClassGMM tmp(shared_parameters, root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

// void xmm::SingleClassGMM::makeBimodal(unsigned int dimension_input)
//{
//    check_training();
//    if (bimodal_)
//        throw std::runtime_error("The model is already bimodal");
//    if (dimension_input >= dimension_)
//        throw std::out_of_range("Request input dimension exceeds the current
//        dimension");
//    flags_ = BIMODAL;
//    bimodal_ = true;
//    dimension_input_ = dimension_input;
//    for (mixture_iterator component = components.begin() ; component !=
//    components.end(); ++component) {
//        component->makeBimodal(dimension_input);
//    }
//    baseResults_predicted_output.resize(dimension_ - dimension_input_);
//    results.output_variance.resize(dimension_ - dimension_input_);
//}
//
// void xmm::SingleClassGMM::makeUnimodal()
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model is already unimodal");
//    flags_ = NONE;
//    bimodal_ = false;
//    dimension_input_ = 0;
//    for (mixture_iterator component = components.begin() ; component !=
//    components.end(); ++component) {
//        component->makeUnimodal();
//    }
//    baseResults_predicted_output.clear();
//    results.output_variance.clear();
//}
//
// xmm::SingleClassGMM
// xmm::SingleClassGMM::extractSubmodel(std::vector<unsigned int>& columns) const
//{
//    check_training();
//    if (columns.size() > dimension_)
//        throw std::out_of_range("requested number of columns exceeds the
//        dimension of the current model");
//    for (unsigned int column=0; column<columns.size(); ++column) {
//        if (columns[column] >= dimension_)
//            throw std::out_of_range("Some column indices exceeds the dimension
//            of the current model");
//    }
//    unsigned int new_dim =columns.size();
//    GMM target_model(*this);
//    target_model.setTrainingCallback(NULL, NULL);
//    target_model.bimodal_ = false;
//    target_model.dimension_ = static_cast<unsigned int>(new_dim);
//    target_model.dimension_input_ = 0;
//    target_model.flags_ = NONE;
//    target_model.allocate();
//    for (unsigned int c=0; c<nbMixtureComponents_; ++c) {
//        target_model.components[c] = components[c].extractSubmodel(columns);
//    }
//    target_model.baseResults_predicted_output.clear();
//    target_model.results.output_variance.clear();
//    return target_model;
//}
//
// xmm::SingleClassGMM xmm::SingleClassGMM::extractSubmodel_input() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_input(dimension_input_);
//    for (unsigned int i=0; i<dimension_input_; ++i) {
//        columns_input[i] = i;
//    }
//    return extractSubmodel(columns_input);
//}
//
// xmm::SingleClassGMM xmm::SingleClassGMM::extractSubmodel_output() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_output(dimension_ - dimension_input_);
//    for (unsigned int i=dimension_input_; i<dimension_; ++i) {
//        columns_output[i-dimension_input_] = i;
//    }
//    return extractSubmodel(columns_output);
//}
//
// xmm::SingleClassGMM xmm::SingleClassGMM::extract_inverse_model() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns(dimension_);
//    for (unsigned int i=0; i<dimension_-dimension_input_; ++i) {
//        columns[i] = i+dimension_input_;
//    }
//    for (unsigned int i=dimension_-dimension_input_, j=0; i<dimension_; ++i,
//    ++j) {
//        columns[i] = j;
//    }
//    GMM target_model = extractSubmodel(columns);
//    target_model.makeBimodal(dimension_-dimension_input_);
//    return target_model;
//}

void xmm::SingleClassGMM::allocate() {
    mixture_coeffs.resize(parameters.gaussians.get());
    beta.resize(parameters.gaussians.get());
    components.assign(
        parameters.gaussians.get(),
        GaussianDistribution(shared_parameters->bimodal.get(),
                             shared_parameters->dimension.get(),
                             shared_parameters->dimension_input.get(),
                             parameters.covariance_mode.get()));
}

double xmm::SingleClassGMM::obsProb(const float* observation,
                                    int mixtureComponent) const {
    double p(0.);

    if (mixtureComponent < 0) {
        for (mixtureComponent = 0;
             mixtureComponent < parameters.gaussians.get();
             mixtureComponent++) {
            p += obsProb(observation, mixtureComponent);
        }
    } else {
        if (mixtureComponent >= parameters.gaussians.get())
            throw std::out_of_range(
                "The index of the Gaussian Mixture Component is out of bounds");
        p = mixture_coeffs[mixtureComponent] *
            components[mixtureComponent].likelihood(observation);
    }

    return p;
}

double xmm::SingleClassGMM::obsProb_input(const float* observation_input,
                                          int mixtureComponent) const {
    if (!shared_parameters->bimodal.get())
        throw std::runtime_error(
            "Model is not bimodal. Use the function 'obsProb'");
    double p(0.);

    if (mixtureComponent < 0) {
        for (mixtureComponent = 0;
             mixtureComponent < parameters.gaussians.get();
             mixtureComponent++) {
            p += obsProb_input(observation_input, mixtureComponent);
        }
    } else {
        p = mixture_coeffs[mixtureComponent] *
            components[mixtureComponent].likelihood_input(observation_input);
    }

    return p;
}

double xmm::SingleClassGMM::obsProb_bimodal(const float* observation_input,
                                            const float* observation_output,
                                            int mixtureComponent) const {
    if (!shared_parameters->bimodal.get())
        throw std::runtime_error(
            "Model is not bimodal. Use the function 'obsProb'");
    double p(0.);

    if (mixtureComponent < 0) {
        for (mixtureComponent = 0;
             mixtureComponent < parameters.gaussians.get();
             mixtureComponent++) {
            p += obsProb_bimodal(observation_input, observation_output,
                                 mixtureComponent);
        }
    } else {
        p = mixture_coeffs[mixtureComponent] *
            components[mixtureComponent].likelihood_bimodal(observation_input,
                                                            observation_output);
    }

    return p;
}

void xmm::SingleClassGMM::initMeansWithKMeans(TrainingSet* trainingSet) {
    if (!trainingSet || trainingSet->empty()) return;
    int dimension = static_cast<int>(shared_parameters->dimension.get());

    KMeans kmeans(parameters.gaussians.get());
    kmeans.initialization_mode = KMeans::InitializationMode::Biased;
    kmeans.train(trainingSet);
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        for (unsigned int d = 0; d < dimension; ++d) {
            components[c].mean[d] = kmeans.centers[c * dimension + d];
        }
    }
}

void xmm::SingleClassGMM::initCovariances_fullyObserved(
    TrainingSet* trainingSet) {
    // TODO: simplify with covariance symmetricity
    // TODO: If Kmeans, covariances from cluster members
    if (!trainingSet || trainingSet->empty()) return;
    int dimension = static_cast<int>(shared_parameters->dimension.get());

    if (parameters.covariance_mode.get() ==
        GaussianDistribution::CovarianceMode::Full) {
        for (int n = 0; n < parameters.gaussians.get(); n++)
            components[n].covariance.assign(dimension * dimension, 0.0);
    } else {
        for (int n = 0; n < parameters.gaussians.get(); n++)
            components[n].covariance.assign(dimension, 0.0);
    }

    std::vector<double> gmeans(parameters.gaussians.get() * dimension, 0.0);
    std::vector<int> factor(parameters.gaussians.get(), 0);
    for (auto phrase_it = trainingSet->begin(); phrase_it != trainingSet->end();
         phrase_it++) {
        unsigned int step =
            phrase_it->second->size() / parameters.gaussians.get();
        unsigned int offset(0);
        for (int n = 0; n < parameters.gaussians.get(); n++) {
            for (int t = 0; t < step; t++) {
                for (int d1 = 0; d1 < dimension; d1++) {
                    gmeans[n * dimension + d1] +=
                        phrase_it->second->getValue(offset + t, d1);
                    if (parameters.covariance_mode.get() ==
                        GaussianDistribution::CovarianceMode::Full) {
                        for (int d2 = 0; d2 < dimension; d2++) {
                            components[n].covariance[d1 * dimension + d2] +=
                                phrase_it->second->getValue(offset + t, d1) *
                                phrase_it->second->getValue(offset + t, d2);
                        }
                    } else {
                        float value =
                            phrase_it->second->getValue(offset + t, d1);
                        components[n].covariance[d1] += value * value;
                    }
                }
            }
            offset += step;
            factor[n] += step;
        }
    }

    for (int n = 0; n < parameters.gaussians.get(); n++) {
        for (int d1 = 0; d1 < dimension; d1++) {
            gmeans[n * dimension + d1] /= factor[n];
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                for (int d2 = 0; d2 < dimension; d2++)
                    components[n].covariance[d1 * dimension + d2] /= factor[n];
            } else {
                components[n].covariance[d1] /= factor[n];
            }
        }
    }

    for (int n = 0; n < parameters.gaussians.get(); n++) {
        for (int d1 = 0; d1 < dimension; d1++) {
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                for (int d2 = 0; d2 < dimension; d2++)
                    components[n].covariance[d1 * dimension + d2] -=
                        gmeans[n * dimension + d1] * gmeans[n * dimension + d2];
            } else {
                components[n].covariance[d1] -=
                    gmeans[n * dimension + d1] * gmeans[n * dimension + d1];
            }
        }
    }
}

double xmm::SingleClassGMM::emAlgorithmUpdate(TrainingSet* trainingSet) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    double log_prob(0.);

    int totalLength(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it)
        totalLength += it->second->size();

    std::vector<std::vector<double> > p(parameters.gaussians.get());
    std::vector<double> E(parameters.gaussians.get(), 0.0);
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        p[c].resize(totalLength);
        E[c] = 0.;
    }

    int tbase(0);

    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it) {
        unsigned int T = it->second->size();
        for (int t = 0; t < T; t++) {
            double norm_const(0.);
            for (int c = 0; c < parameters.gaussians.get(); c++) {
                if (shared_parameters->bimodal.get()) {
                    p[c][tbase + t] =
                        obsProb_bimodal(it->second->getPointer_input(t),
                                        it->second->getPointer_output(t), c);
                } else {
                    p[c][tbase + t] = obsProb(it->second->getPointer(t), c);
                }

                if (p[c][tbase + t] == 0. || std::isnan(p[c][tbase + t]) ||
                    std::isinf(p[c][tbase + t])) {
                    p[c][tbase + t] = 1e-100;
                }
                norm_const += p[c][tbase + t];
            }
            for (int c = 0; c < parameters.gaussians.get(); c++) {
                p[c][tbase + t] /= norm_const;
                E[c] += p[c][tbase + t];
            }
            log_prob += log(norm_const);
        }
        tbase += T;
    }

    // Estimate Mixture coefficients
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        mixture_coeffs[c] = E[c] / double(totalLength);
    }

    // Estimate means
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        for (int d = 0; d < dimension; d++) {
            components[c].mean[d] = 0.;
            tbase = 0;
            for (auto it = trainingSet->cbegin(); it != trainingSet->cend();
                 ++it) {
                unsigned int T = it->second->size();
                for (int t = 0; t < T; t++) {
                    components[c].mean[d] +=
                        p[c][tbase + t] * it->second->getValue(t, d);
                }
                tbase += T;
            }
            components[c].mean[d] /= E[c];
        }
    }

    // estimate covariances
    if (parameters.covariance_mode.get() ==
        GaussianDistribution::CovarianceMode::Full) {
        for (int c = 0; c < parameters.gaussians.get(); c++) {
            for (int d1 = 0; d1 < dimension; d1++) {
                for (int d2 = d1; d2 < dimension; d2++) {
                    components[c].covariance[d1 * dimension + d2] = 0.;
                    tbase = 0;
                    for (auto it = trainingSet->cbegin();
                         it != trainingSet->cend(); ++it) {
                        unsigned int T = it->second->size();
                        for (int t = 0; t < T; t++) {
                            components[c].covariance[d1 * dimension + d2] +=
                                p[c][tbase + t] * (it->second->getValue(t, d1) -
                                                   components[c].mean[d1]) *
                                (it->second->getValue(t, d2) -
                                 components[c].mean[d2]);
                        }
                        tbase += T;
                    }
                    components[c].covariance[d1 * dimension + d2] /= E[c];
                    if (d1 != d2)
                        components[c].covariance[d2 * dimension + d1] =
                            components[c].covariance[d1 * dimension + d2];
                }
            }
        }
    } else {
        for (int c = 0; c < parameters.gaussians.get(); c++) {
            for (int d1 = 0; d1 < dimension; d1++) {
                components[c].covariance[d1] = 0.;
                tbase = 0;
                for (auto it = trainingSet->cbegin(); it != trainingSet->cend();
                     ++it) {
                    unsigned int T = it->second->size();
                    for (int t = 0; t < T; t++) {
                        float value = (it->second->getValue(t, d1) -
                                       components[c].mean[d1]);
                        components[c].covariance[d1] +=
                            p[c][tbase + t] * value * value;
                    }
                    tbase += T;
                }
                components[c].covariance[d1] /= E[c];
            }
        }
    }

    addCovarianceOffset();
    updateInverseCovariances();

    return log_prob;
}

void xmm::SingleClassGMM::initParametersToDefault(
    std::vector<float> const& dataStddev) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());

    double norm_coeffs(0.);
    current_regularization.resize(dataStddev.size());
    for (int i = 0; i < dataStddev.size(); i++) {
        current_regularization[i] =
            std::max(parameters.absolute_regularization.get(),
                     parameters.relative_regularization.get() * dataStddev[i]);
    }
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        if (parameters.covariance_mode.get() ==
            GaussianDistribution::CovarianceMode::Full) {
            components[c].covariance.assign(
                dimension * dimension,
                parameters.absolute_regularization.get() / 2.);
        } else {
            components[c].covariance.assign(dimension, 0.);
        }
        components[c].regularize(current_regularization);
        mixture_coeffs[c] = 1. / float(parameters.gaussians.get());
        norm_coeffs += mixture_coeffs[c];
    }
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        mixture_coeffs[c] /= norm_coeffs;
    }
}

void xmm::SingleClassGMM::normalizeMixtureCoeffs() {
    double norm_const(0.);
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        norm_const += mixture_coeffs[c];
    }
    if (norm_const > 0) {
        for (int c = 0; c < parameters.gaussians.get(); c++) {
            mixture_coeffs[c] /= norm_const;
        }
    } else {
        for (int c = 0; c < parameters.gaussians.get(); c++) {
            mixture_coeffs[c] = 1 / float(parameters.gaussians.get());
        }
    }
}

void xmm::SingleClassGMM::addCovarianceOffset() {
    for (auto& component : components) {
        component.regularize(current_regularization);
    }
}

void xmm::SingleClassGMM::updateInverseCovariances() {
    try {
        for (auto& component : components) {
            component.updateInverseCovariance();
        }
    } catch (std::exception& e) {
        throw std::runtime_error(
            "Matrix inversion error: varianceoffset must be too small");
    }
}

void xmm::SingleClassGMM::regression(
    std::vector<float> const& observation_input) {
    check_training();
    unsigned int dimension_output = shared_parameters->dimension.get() -
                                   shared_parameters->dimension_input.get();
    results.output_values.assign(dimension_output, 0.0);
    results.output_covariance.assign(
        (parameters.covariance_mode.get() ==
         GaussianDistribution::CovarianceMode::Full)
            ? dimension_output * dimension_output
            : dimension_output,
        0.0);
    std::vector<float> tmp_output_values(dimension_output, 0.0);

    for (int c = 0; c < parameters.gaussians.get(); c++) {
        components[c].regression(observation_input, tmp_output_values);
        for (int d = 0; d < dimension_output; ++d) {
            results.output_values[d] += beta[c] * tmp_output_values[d];
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                for (int d2 = 0; d2 < dimension_output; ++d2)
                    results.output_covariance[d * dimension_output + d2] +=
                        beta[c] * beta[c] *
                        components[c]
                            .output_covariance[d * dimension_output + d2];
            } else {
                results.output_covariance[d] +=
                    beta[c] * beta[c] * components[c].output_covariance[d];
            }
        }
    }
}

double xmm::SingleClassGMM::likelihood(
    std::vector<float> const& observation,
    std::vector<float> const& observation_output) {
    check_training();
    double likelihood(0.);
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        if (shared_parameters->bimodal.get()) {
            if (observation_output.empty())
                beta[c] = obsProb_input(&observation[0], c);
            else
                beta[c] =
                    obsProb_bimodal(&observation[0], &observation_output[0], c);
        } else {
            beta[c] = obsProb(&observation[0], c);
        }
        likelihood += beta[c];
    }
    for (int c = 0; c < parameters.gaussians.get(); c++) {
        beta[c] /= likelihood;
    }

    results.instant_likelihood = likelihood;
    updateResults();
    return likelihood;
}

void xmm::SingleClassGMM::updateResults() {
    likelihood_buffer_.push(log(results.instant_likelihood));
    results.log_likelihood = 0.0;
    unsigned int bufSize = likelihood_buffer_.size_t();
    for (unsigned int i = 0; i < bufSize; i++) {
        results.log_likelihood += likelihood_buffer_(0, i);
    }
    results.log_likelihood /= double(bufSize);
}
