/*
 * hierarchical_hmm.cpp
 *
 * Hierarchical Hidden Markov Model for continuous recognition and regression
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

#include "xmmHierarchicalHmm.hpp"
#include <algorithm>

xmm::HierarchicalHMM::HierarchicalHMM(bool bimodal)
    : Model<SingleClassHMM, HMM>(bimodal), forward_initialized_(false) {}

xmm::HierarchicalHMM::HierarchicalHMM(HierarchicalHMM const &src)
    : Model<SingleClassHMM, HMM>(src) {
    results = src.results;
    prior = src.prior;
    exit_transition = src.exit_transition;
    transition = src.transition;
    frontier_v1_ = src.frontier_v1_;
    frontier_v2_ = src.frontier_v2_;
    forward_initialized_ = false;
}

xmm::HierarchicalHMM::HierarchicalHMM(Json::Value const &root)
    : Model<SingleClassHMM, HMM>(root), forward_initialized_(false) {
    prior.resize(size());
    json2vector(root["prior"], prior, size());
    transition.resize(size());
    for (int i = 0; i < size(); i++) {
        transition[i].resize(size());
        json2vector(root["transition"][i], transition[i], size());
    }
    exit_transition.resize(size());
    json2vector(root["exit_transition"], exit_transition, size());
}

xmm::HierarchicalHMM &xmm::HierarchicalHMM::operator=(
    HierarchicalHMM const &src) {
    if (this != &src) {
        Model<SingleClassHMM, HMM>::operator=(src);
        results = src.results;
        prior = src.prior;
        exit_transition = src.exit_transition;
        transition = src.transition;
        frontier_v1_ = src.frontier_v1_;
        frontier_v2_ = src.frontier_v2_;
        forward_initialized_ = false;
    }
    return *this;
}

void xmm::HierarchicalHMM::clear() {
    Model<SingleClassHMM, HMM>::clear();
    prior.clear();
    transition.clear();
    exit_transition.clear();
}

void xmm::HierarchicalHMM::addExitPoint(int state, float proba) {
    // prevent_attribute_change();
    for (auto &model : models) {
        model.second.addExitPoint(state, proba);
    }
}

void xmm::HierarchicalHMM::normalizeTransitions() {
    double sumPrior(0.0);
    int num_models = static_cast<int>(size());
    for (int i = 0; i < num_models; i++) {
        sumPrior += prior[i];
        double sumTrans(0.0);
        for (int j = 0; j < num_models; j++) sumTrans += transition[i][j];
        if (sumTrans > 0.0)
            for (int j = 0; j < num_models; j++) transition[i][j] /= sumTrans;
    }
    for (int i = 0; i < size(); i++) prior[i] /= sumPrior;
}

void xmm::HierarchicalHMM::joinTraining() {
    Model<SingleClassHMM, HMM>::joinTraining();
    updateTransitionParameters();
}

void xmm::HierarchicalHMM::updateTransitionParameters() {
    if (this->size() == prior.size())  // number of primitives has not changed
        return;

    updatePrior();
    updateTransition();

    updateExitProbabilities();  // Update exit probabilities of Submodels
                                // (signal level)
}

void xmm::HierarchicalHMM::updatePrior() {
    prior.assign(size(), 1. / static_cast<double>(this->size()));
}

void xmm::HierarchicalHMM::updateTransition() {
    int num_classes = static_cast<int>(size());
    exit_transition.assign(num_classes, DEFAULT_EXITTRANSITION());
    transition.resize(num_classes);
    for (int i = 0; i < num_classes; i++) {
        transition[i].assign(num_classes,
                             1. / static_cast<double>(num_classes));
    }
}

void xmm::HierarchicalHMM::updateExitProbabilities() {
    for (auto &model : models) {
        model.second.updateExitProbabilities();
    }
}

void xmm::HierarchicalHMM::addModelForClass(std::string const &label) {
    Model<SingleClassHMM, HMM>::addModelForClass(label);
    updateTransitionParameters();
}

void xmm::HierarchicalHMM::forward_init(std::vector<float> const &observation) {
    checkTraining();
    double norm_const(0.0);

    int model_index(0);
    for (auto &model : models) {
        unsigned int N = model.second.parameters.states.get();

        for (int i = 0; i < 3; i++) {
            model.second.alpha_h[i].assign(N, 0.0);
        }

        // Compute Emission probability and initialize on the first state of
        // the
        // primitive
        if (model.second.parameters.transition_mode.get() ==
            HMM::TransitionMode::Ergodic) {
            for (int i = 0; i < model.second.parameters.states.get(); i++) {
                if (shared_parameters->bimodal.get()) {
                    model.second.alpha_h[0][i] =
                        model.second.prior[i] *
                        model.second.states[i].obsProb_input(&observation[0]);
                } else {
                    model.second.alpha_h[0][i] =
                        model.second.prior[i] *
                        model.second.states[i].obsProb(&observation[0]);
                }
                model.second.results.instant_likelihood +=
                    model.second.alpha_h[0][i];
            }
        } else {
            model.second.alpha_h[0][0] = this->prior[model_index];
            if (shared_parameters->bimodal.get()) {
                model.second.alpha_h[0][0] *=
                    model.second.states[0].obsProb_input(&observation[0]);
            } else {
                model.second.alpha_h[0][0] *=
                    model.second.states[0].obsProb(&observation[0]);
            }
            model.second.results.instant_likelihood =
                model.second.alpha_h[0][0];
        }
        norm_const += model.second.results.instant_likelihood;
        model_index++;
    }

    // Normalize Alpha variables
    for (auto &model : models) {
        unsigned int N = model.second.parameters.states.get();
        for (unsigned int e = 0; e < 3; e++)
            for (unsigned int k = 0; k < N; k++)
                model.second.alpha_h[e][k] /= norm_const;
    }

    forward_initialized_ = true;
}

void xmm::HierarchicalHMM::forward_update(
    std::vector<float> const &observation) {
    checkTraining();
    double norm_const(0.0);

    // Frontier Algorithm: variables
    double tmp(0);
    std::vector<double>
        front;  // frontier variable : intermediate computation variable

    // Intermediate variables: compute the sum of probabilities of making a
    // transition to a new primitive
    likelihoodAlpha(1, frontier_v1_);
    likelihoodAlpha(2, frontier_v2_);

    int num_classes = static_cast<int>(size());

    // FORWARD UPDATE
    // --------------------------------------
    int dst_model_index(0);
    for (auto &dstModel : models) {
        unsigned int N = dstModel.second.parameters.states.get();

        // 1) COMPUTE FRONTIER VARIABLE
        //    --------------------------------------
        front.assign(N, 0.0);

        if (dstModel.second.parameters.transition_mode.get() ==
            HMM::TransitionMode::Ergodic) {
            for (int k = 0; k < N; ++k) {
                for (unsigned int j = 0; j < N; ++j) {
                    front[k] += dstModel.second.transition[j * N + k] /
                                (1 - dstModel.second.exit_probabilities_[j]) *
                                dstModel.second.alpha_h[0][j];
                }

                for (int src_model_index = 0; src_model_index < num_classes;
                     src_model_index++) {
                    front[k] += dstModel.second.prior[k] *
                                (frontier_v1_[src_model_index] *
                                     this->transition[src_model_index]
                                                     [dst_model_index] +
                                 this->prior[dst_model_index] *
                                     frontier_v2_[src_model_index]);
                }
            }
        } else {
            // k=0: first state of the primitive
            front[0] =
                dstModel.second.transition[0] * dstModel.second.alpha_h[0][0];

            for (int src_model_index = 0; src_model_index < num_classes;
                 src_model_index++) {
                front[0] += frontier_v1_[src_model_index] *
                                transition[src_model_index][dst_model_index] +
                            this->prior[dst_model_index] *
                                frontier_v2_[src_model_index];
            }

            // k>0: rest of the primitive
            for (int k = 1; k < N; ++k) {
                front[k] += dstModel.second.transition[k * 2] /
                            (1 - dstModel.second.exit_probabilities_[k]) *
                            dstModel.second.alpha_h[0][k];
                front[k] += dstModel.second.transition[(k - 1) * 2 + 1] /
                            (1 - dstModel.second.exit_probabilities_[k - 1]) *
                            dstModel.second.alpha_h[0][k - 1];
            }

            for (int i = 0; i < 3; i++) {
                for (int k = 0; k < N; k++) {
                    dstModel.second.alpha_h[i][k] = 0.0;
                }
            }
        }

        // 2) UPDATE FORWARD VARIABLE
        //    --------------------------------------

        dstModel.second.results.exit_likelihood = 0.0;
        dstModel.second.results.instant_likelihood = 0.0;

        // end of the primitive: handle exit states
        for (int k = 0; k < N; ++k) {
            if (shared_parameters->bimodal.get())
                tmp = dstModel.second.states[k].obsProb_input(&observation[0]) *
                      front[k];
            else
                tmp = dstModel.second.states[k].obsProb(&observation[0]) *
                      front[k];

            dstModel.second.alpha_h[2][k] =
                this->exit_transition[dst_model_index] *
                dstModel.second.exit_probabilities_[k] * tmp;
            dstModel.second.alpha_h[1][k] =
                (1 - this->exit_transition[dst_model_index]) *
                dstModel.second.exit_probabilities_[k] * tmp;
            dstModel.second.alpha_h[0][k] =
                (1 - dstModel.second.exit_probabilities_[k]) * tmp;

            dstModel.second.results.exit_likelihood +=
                dstModel.second.alpha_h[1][k] + dstModel.second.alpha_h[2][k];
            dstModel.second.results.instant_likelihood +=
                dstModel.second.alpha_h[0][k] + dstModel.second.alpha_h[1][k] +
                dstModel.second.alpha_h[2][k];

            norm_const += tmp;
        }

        dstModel.second.results.exit_ratio =
            dstModel.second.results.exit_likelihood /
            dstModel.second.results.instant_likelihood;

        dst_model_index++;
    }

    // Normalize Alpha variables
    for (auto &model : models) {
        unsigned int N = model.second.parameters.states.get();
        for (unsigned int e = 0; e < 3; e++)
            for (unsigned int k = 0; k < N; k++)
                model.second.alpha_h[e][k] /= norm_const;
    }
}

void xmm::HierarchicalHMM::likelihoodAlpha(
    int exitNum, std::vector<double> &likelihoodVector) const {
    if (exitNum < 0) {  // Likelihood over all exit states
        unsigned int l(0);
        for (auto &model : models) {
            likelihoodVector[l] = 0.0;
            for (unsigned int exit = 0; exit < 3; ++exit) {
                for (unsigned int k = 0;
                     k < model.second.parameters.states.get(); k++) {
                    likelihoodVector[l] += model.second.alpha_h[exit][k];
                }
            }
            l++;
        }

    } else {  // Likelihood for exit state "exitNum"
        unsigned int l(0);
        for (auto &model : models) {
            likelihoodVector[l] = 0.0;
            for (unsigned int k = 0; k < model.second.parameters.states.get();
                 k++) {
                likelihoodVector[l] += model.second.alpha_h[exitNum][k];
            }
            l++;
        }
    }
}

void xmm::HierarchicalHMM::removeClass(std::string const &label) {
    Model<SingleClassHMM, HMM>::removeClass(label);
    updateTransitionParameters();
}

void xmm::HierarchicalHMM::reset() {
    Model<SingleClassHMM, HMM>::reset();
    results.instant_likelihoods.resize(size());
    results.instant_normalized_likelihoods.resize(size());
    results.smoothed_likelihoods.resize(size());
    results.smoothed_normalized_likelihoods.resize(size());
    results.smoothed_log_likelihoods.resize(size());
    if (shared_parameters->bimodal.get()) {
        unsigned int dimension_output =
            shared_parameters->dimension.get() -
            shared_parameters->dimension_input.get();
        results.output_values.resize(dimension_output);
        results.output_covariance.assign(
            (configuration.covariance_mode.get() ==
             GaussianDistribution::CovarianceMode::Full)
                ? dimension_output * dimension_output
                : dimension_output,
            0.0);
    }
    frontier_v1_.resize(this->size());
    frontier_v2_.resize(this->size());
    forward_initialized_ = false;
    for (auto &model : models) {
        model.second.is_hierarchical_ = configuration.hierarchical.get();
        model.second.reset();
    }
}

void xmm::HierarchicalHMM::filter(std::vector<float> const &observation) {
    checkTraining();
    if (configuration.hierarchical.get()) {
        if (forward_initialized_) {
            this->forward_update(observation);
        } else {
            this->forward_init(observation);
        }
    } else {
        int i(0);
        for (auto &model : models) {
            results.instant_likelihoods[i] = model.second.filter(observation);
            i++;
        }
    }

    // Compute time progression
    for (auto &model : models) {
        model.second.updateAlphaWindow();
        model.second.updateResults();
    }
    updateResults();

    if (shared_parameters->bimodal.get()) {
        unsigned int dimension = shared_parameters->dimension.get();
        unsigned int dimension_input = shared_parameters->dimension_input.get();
        unsigned int dimension_output = dimension - dimension_input;

        for (auto &model : models) {
            model.second.regression(observation);
        }

        if (configuration.multiClass_regression_estimator ==
            MultiClassRegressionEstimator::Likeliest) {
            results.output_values =
                this->models[results.likeliest].results.output_values;
            results.output_covariance =
                this->models[results.likeliest].results.output_covariance;
        } else {
            results.output_values.assign(dimension_output, 0.0);
            results.output_covariance.assign(
                (configuration.covariance_mode.get() ==
                 GaussianDistribution::CovarianceMode::Full)
                    ? dimension_output * dimension_output
                    : dimension_output,
                0.0);

            int i(0);
            for (auto &model : models) {
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

void xmm::HierarchicalHMM::updateResults() {
    double maxlog_likelihood = 0.0;
    double normconst_instant(0.0);
    double normconst_smoothed(0.0);
    int i(0);
    for (auto &model : models) {
        results.instant_likelihoods[i] =
            model.second.results.instant_likelihood;
        results.smoothed_log_likelihoods[i] =
            model.second.results.log_likelihood;
        results.smoothed_likelihoods[i] =
            exp(results.smoothed_log_likelihoods[i]);

        results.instant_normalized_likelihoods[i] =
            results.instant_likelihoods[i];
        results.smoothed_normalized_likelihoods[i] =
            results.smoothed_likelihoods[i];

        normconst_instant += results.instant_normalized_likelihoods[i];
        normconst_smoothed += results.smoothed_normalized_likelihoods[i];

        if (i == 0 || results.smoothed_log_likelihoods[i] > maxlog_likelihood) {
            maxlog_likelihood = results.smoothed_log_likelihoods[i];
            results.likeliest = model.first;
        }
        i++;
    }

    i = 0;
    for (auto it = this->models.begin(); it != this->models.end(); ++it, ++i) {
        results.instant_normalized_likelihoods[i] /= normconst_instant;
        results.smoothed_normalized_likelihoods[i] /= normconst_smoothed;
    }
}

Json::Value xmm::HierarchicalHMM::toJson() const {
    checkTraining();
    Json::Value root = Model<SingleClassHMM, HMM>::toJson();
    root["prior"] = vector2json(prior);
    //    int num_models = static_cast<int>(size());
    //    std::vector<double> transition_flat(num_models * num_models, 0.0);
    //    for (int i = 0; i < num_models; i++) {
    //        for (int j = 0; j < num_models; j++) {
    //            transition_flat[i * num_models + j] = transition[i][j];
    //        }
    //    }
    root["transition"].resize(static_cast<Json::ArrayIndex>(size()));
    for (int i = 0; i < size(); i++) {
        root["transition"][i] = vector2json(transition[i]);
    }
    root["exit_transition"] = vector2json(exit_transition);

    return root;
}

void xmm::HierarchicalHMM::fromJson(Json::Value const &root) {
    checkTraining();
    try {
        HierarchicalHMM tmp(root);
        *this = tmp;
    } catch (JsonException &e) {
        throw e;
    }
}

// void xmm::HierarchicalHMM::makeBimodal(unsigned int dimension_input)
//{
//    checkTraining();
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
//        model.second.makeBimodal(dimension_input);
//    }
//    set_trainingSet(NULL);
//    results.output_values.resize(dimension() - this->dimension_input());
//    results.output_variance.resize(dimension() - this->dimension_input());
//}
//
// void xmm::HierarchicalHMM::makeUnimodal()
//{
//    checkTraining();
//    if (!bimodal_)
//        throw std::runtime_error("The model is already unimodal");
//    this->referenceModel_.makeUnimodal();
//    for (model_iterator it=this->models.begin(); it != this->models.end();
//    ++it) {
//        model.second.makeUnimodal();
//    }
//    set_trainingSet(NULL);
//    results.output_values.clear();
//    results.output_variance.clear();
//    bimodal_ = false;
//}
//
// xmm::HierarchicalHMM
// xmm::HierarchicalHMM::extractSubmodel(std::vector<unsigned int>& columns)
// const
//{
//    checkTraining();
//    if (columns.size() > this->dimension())
//        throw std::out_of_range("requested number of columns exceeds the
//        dimension of the current model");
//    for (unsigned int column=0; column<columns.size(); ++column) {
//        if (columns[column] >= this->dimension())
//            throw std::out_of_range("Some column indices exceeds the dimension
//            of the current model");
//    }
//    HierarchicalHMM target_model(*this);
//    target_model.set_trainingSet(NULL);
//    target_model.bimodal_ = false;
//    target_model.referenceModel_ =
//    this->referenceModel_.extractSubmodel(columns);
//    for (model_iterator it=target_model.models.begin(); it !=
//    target_model.models.end(); ++it) {
//        model.second = this->models.at(model.first).extractSubmodel(columns);
//    }
//    return target_model;
//}
//
// xmm::HierarchicalHMM xmm::HierarchicalHMM::extractSubmodel_input() const
//{
//    checkTraining();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_input(dimension_input());
//    for (unsigned int i=0; i<dimension_input(); ++i) {
//        columns_input[i] = i;
//    }
//    return extractSubmodel(columns_input);
//}
//
// xmm::HierarchicalHMM xmm::HierarchicalHMM::extractSubmodel_output() const
//{
//    checkTraining();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns_output(dimension() - dimension_input());
//    for (unsigned int i=dimension_input(); i<dimension(); ++i) {
//        columns_output[i-dimension_input()] = i;
//    }
//    return extractSubmodel(columns_output);
//}
//
// xmm::HierarchicalHMM xmm::HierarchicalHMM::extract_inverse_model() const
//{
//    checkTraining();
//    if (!bimodal_)
//        throw std::runtime_error("The model needs to be bimodal");
//    std::vector<unsigned int> columns(dimension());
//    for (unsigned int i=0; i<dimension()-dimension_input(); ++i) {
//        columns[i] = i+dimension_input();
//    }
//    for (unsigned int i=dimension()-dimension_input(), j=0; i<dimension();
//    ++i,
//    ++j) {
//        columns[i] = j;
//    }
//    HierarchicalHMM target_model = extractSubmodel(columns);
//    target_model.makeBimodal(dimension()-dimension_input());
//    return target_model;
//}
