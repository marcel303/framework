/*
 * xmmGmm.hpp
 *
 * Gaussian Mixture Model for Continuous Recognition and Regression
 * (Multi-class)
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

#include "xmmGmm.hpp"
#include "../kmeans/xmmKMeans.hpp"

xmm::GMM::GMM(bool bimodal) : Model<SingleClassGMM, GMM>(bimodal) {}

xmm::GMM::GMM(GMM const& src)
    : Model<SingleClassGMM, GMM>(src), results(src.results) {}

xmm::GMM::GMM(Json::Value const& root) : Model<SingleClassGMM, GMM>(root) {}

xmm::GMM& xmm::GMM::operator=(GMM const& src) {
    if (this != &src) {
        Model<SingleClassGMM, GMM>::operator=(src);
        results = src.results;
    }
    return *this;
}

void xmm::GMM::updateResults() {
    double maxLogLikelihood = 0.0;
    double normconst_instant(0.0);
    double normconst_smoothed(0.0);
    int i(0);
    for (auto it = this->models.begin(); it != this->models.end(); ++it, ++i) {
        results.instant_likelihoods[i] = it->second.results.instant_likelihood;
        results.smoothed_log_likelihoods[i] = it->second.results.log_likelihood;
        results.smoothed_likelihoods[i] =
            exp(results.smoothed_log_likelihoods[i]);

        results.instant_normalized_likelihoods[i] =
            results.instant_likelihoods[i];
        results.smoothed_normalized_likelihoods[i] =
            results.smoothed_likelihoods[i];

        normconst_instant += results.instant_normalized_likelihoods[i];
        normconst_smoothed += results.smoothed_normalized_likelihoods[i];

        if (i == 0 || results.smoothed_log_likelihoods[i] > maxLogLikelihood) {
            maxLogLikelihood = results.smoothed_log_likelihoods[i];
            results.likeliest = it->first;
        }
    }

    i = 0;
    for (auto it = this->models.begin(); it != this->models.end(); ++it, ++i) {
        results.instant_normalized_likelihoods[i] /= normconst_instant;
        results.smoothed_normalized_likelihoods[i] /= normconst_smoothed;
    }
}

#pragma mark -
#pragma mark Performance
void xmm::GMM::reset() {
    results.instant_likelihoods.resize(size());
    results.instant_normalized_likelihoods.resize(size());
    results.smoothed_likelihoods.resize(size());
    results.smoothed_normalized_likelihoods.resize(size());
    results.smoothed_log_likelihoods.resize(size());
    if (shared_parameters->bimodal.get()) {
        unsigned int dimension_output = shared_parameters->dimension.get() -
                                       shared_parameters->dimension_input.get();
        results.output_values.resize(dimension_output);
        results.output_covariance.assign(
            (configuration.covariance_mode.get() ==
             GaussianDistribution::CovarianceMode::Full)
                ? dimension_output * dimension_output
                : dimension_output,
            0.0);
    }
    for (auto& model : models) {
        model.second.reset();
    }
}

void xmm::GMM::filter(std::vector<float> const& observation) {
    checkTraining();
    int i(0);
    for (auto& model : models) {
        results.instant_likelihoods[i] = model.second.filter(observation);
        i++;
    }

    updateResults();

    if (shared_parameters->bimodal.get()) {
        unsigned int dimension = shared_parameters->dimension.get();
        unsigned int dimension_input = shared_parameters->dimension_input.get();
        unsigned int dimension_output = dimension - dimension_input;

        if (configuration.multiClass_regression_estimator ==
            MultiClassRegressionEstimator::Likeliest) {
            results.output_values =
                models[results.likeliest].results.output_values;
            results.output_covariance =
                models[results.likeliest].results.output_covariance;

        } else {
            results.output_values.assign(dimension_output, 0.0);
            results.output_covariance.assign(
                (configuration.covariance_mode.get() ==
                 GaussianDistribution::CovarianceMode::Full)
                    ? dimension_output * dimension_output
                    : dimension_output,
                0.0);

            int i(0);
            for (auto& model : models) {
                for (int d = 0; d < dimension_output; d++) {
                    results.output_values[d] +=
                        results.smoothed_normalized_likelihoods[i] *
                        model.second.results.output_values[d];
                    if ((configuration.covariance_mode.get() ==
                         GaussianDistribution::CovarianceMode::Full)) {
                        for (int d2 = 0; d2 < dimension_output; d2++)
                            results
                                .output_covariance[d * dimension_output + d2] +=
                                results.smoothed_normalized_likelihoods[i] *
                                model.second.results
                                    .output_covariance[d * dimension_output +
                                                       d2];
                    } else {
                        results.output_covariance[d] +=
                            results.smoothed_normalized_likelihoods[i] *
                            model.second.results.output_covariance[d];
                    }
                }
                i++;
            }
        }
    }
}

//#pragma mark > Conversion & Extraction
// void xmm::GMM::makeBimodal(unsigned int dimension_input)
//{
//    check_training();
//    if (bimodal_)
//        throw std::runtime_error("The model is already bimodal");
//    if (dimension_input >= dimension())
//        throw std::out_of_range("Request input dimension exceeds the current
//        dimension");
//
//    try {
//        this->referenceModel_.makeBimodal(dimension_input);
//    } catch (std::exception const& e) {
//    }
//    bimodal_ = true;
//    for (model_iterator it=this->models.begin(); it != this->models.end();
//    ++it) {
//        it->second.makeBimodal(dimension_input);
//    }
//    set_trainingSet(NULL);
//    results_predicted_output.resize(dimension() - this->dimension_input());
//    results_output_variance.resize(dimension() - this->dimension_input());
//}
//
// void xmm::GMM::makeUnimodal()
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model is already unimodal");
//    this->referenceModel_.makeUnimodal();
//    for (model_iterator it=this->models.begin(); it != this->models.end();
//    ++it) {
//        it->second.makeUnimodal();
//    }
//    set_trainingSet(NULL);
//    results_predicted_output.clear();
//    results_output_variance.clear();
//    bimodal_ = false;
//}
//
// xmm::GMM xmm::GMM::extractSubmodel(std::vector<unsigned int>& columns) const
//{
//    check_training();
//    if (columns.size() > this->dimension())
//        throw std::out_of_range("requested number of columns exceeds the
//        dimension of the current model");
//    for (unsigned int column=0; column<columns.size(); ++column) {
//        if (columns[column] >= this->dimension())
//            throw std::out_of_range("Some column indices exceeds the dimension
//            of the current model");
//    }
//    GMM target_model(*this);
//    target_model.set_trainingSet(NULL);
//    target_model.bimodal_ = false;
//    target_model.referenceModel_ =
//    this->referenceModel_.extractSubmodel(columns);
//    for (model_iterator it=target_model.models.begin(); it !=
//    target_model.models.end(); ++it) {
//        it->second = this->models.at(it->first).extractSubmodel(columns);
//    }
//    target_model.results_predicted_output.clear();
//    target_model.results_output_variance.clear();
//    return target_model;
//}
//
// xmm::GMM xmm::GMM::extractSubmodel_input() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_input(dimension_input());
//    for (unsigned int i=0; i<dimension_input(); ++i) {
//        columns_input[i] = i;
//    }
//    return extractSubmodel(columns_input);
//}
//
// xmm::GMM xmm::GMM::extractSubmodel_output() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_output(dimension() - dimension_input());
//    for (unsigned int i=dimension_input(); i<dimension(); ++i) {
//        columns_output[i-dimension_input()] = i;
//    }
//    return extractSubmodel(columns_output);
//}
//
// xmm::GMM xmm::GMM::extract_inverse_model() const
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns(dimension());
//    for (unsigned int i=0; i<dimension()-dimension_input(); ++i) {
//        columns[i] = i+dimension_input();
//    }
//    for (unsigned int i=dimension()-dimension_input(), j=0; i<dimension(); ++i,
//    ++j) {
//        columns[i] = j;
//    }
//    GMM target_model = extractSubmodel(columns);
//    target_model.makeBimodal(dimension()-dimension_input());
//    return target_model;
//}
