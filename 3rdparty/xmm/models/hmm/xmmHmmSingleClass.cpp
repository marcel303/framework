/*
 * xmmHmmSingleClass.cpp
 *
 * Hidden Markov Model Definition for a Single Class
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

#include "xmmHmmSingleClass.hpp"

xmm::SingleClassHMM::SingleClassHMM(std::shared_ptr<SharedParameters> p)
    : SingleClassProbabilisticModel(p), is_hierarchical_(true) {}

xmm::SingleClassHMM::SingleClassHMM(SingleClassHMM const& src)
    : SingleClassProbabilisticModel(src),
      parameters(src.parameters),
      states(src.states),
      prior(src.prior),
      transition(src.transition),
      is_hierarchical_(src.is_hierarchical_),
      exit_probabilities_(src.exit_probabilities_) {
    alpha.resize(parameters.states.get());
    previous_alpha_.resize(parameters.states.get());
    beta_.resize(parameters.states.get());
    previous_beta_.resize(parameters.states.get());
}

xmm::SingleClassHMM::SingleClassHMM(std::shared_ptr<SharedParameters> p,
                                    Json::Value const& root)
    : SingleClassProbabilisticModel(p, root), is_hierarchical_(true) {
    parameters.fromJson(root["parameters"]);

    allocate();

    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        json2vector(root["prior"], prior, parameters.states.get());
        json2vector(root["transition"], transition,
                    parameters.states.get() * parameters.states.get());
    } else {
        json2vector(root["transition"], transition,
                    2 * parameters.states.get());
    }
    if (root.isMember("exitProbabilities")) {
        json2vector(root["exitProbabilities"], exit_probabilities_,
                    parameters.states.get());
    }

    int s(0);
    for (auto p : root["states"]) {
        states[s++].fromJson(p);
    }
}

xmm::SingleClassHMM& xmm::SingleClassHMM::operator=(SingleClassHMM const& src) {
    if (this != &src) {
        SingleClassProbabilisticModel::operator=(src);

        is_hierarchical_ = src.is_hierarchical_;
        parameters = src.parameters;
        transition = src.transition;
        prior = src.prior;
        exit_probabilities_ = src.exit_probabilities_;
        states = src.states;

        alpha.resize(parameters.states.get());
        previous_alpha_.resize(parameters.states.get());
        beta_.resize(parameters.states.get());
        previous_beta_.resize(parameters.states.get());
    }
    return *this;
}

#pragma mark -
#pragma mark Parameters initialization
void xmm::SingleClassHMM::allocate() {
    unsigned int numStates = parameters.states.get();

    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        prior.resize(numStates);
        transition.resize(numStates * numStates);
    } else {
        prior.clear();
        transition.resize(numStates * 2);
    }
    alpha.resize(numStates);
    previous_alpha_.resize(numStates);
    beta_.resize(numStates);
    previous_beta_.resize(numStates);
    SingleClassGMM tmpGMM(shared_parameters);
    tmpGMM.parameters.gaussians.set(parameters.gaussians.get());
    tmpGMM.parameters.relative_regularization.set(
        parameters.relative_regularization.get());
    tmpGMM.parameters.absolute_regularization.set(
        parameters.absolute_regularization.get());
    tmpGMM.parameters.covariance_mode.set(parameters.covariance_mode.get());
    states.assign(numStates, tmpGMM);
    for (auto& state : states) {
        state.allocate();
    }
    if (is_hierarchical_) updateExitProbabilities(NULL);
}

void xmm::SingleClassHMM::initParametersToDefault(
    std::vector<float> const& dataStddev) {
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        setErgodic();
    } else {
        setLeftRight();
    }
    for (int i = 0; i < parameters.states.get(); i++) {
        states[i].initParametersToDefault(dataStddev);
    }
}

void xmm::SingleClassHMM::initMeansWithAllPhrases(TrainingSet* trainingSet) {
    if (!trainingSet || trainingSet->empty()) return;
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int numStates = parameters.states.get();

    for (int n = 0; n < numStates; n++)
        for (int d = 0; d < dimension; d++)
            states[n].components[0].mean[d] = 0.0;

    std::vector<int> factor(numStates, 0);
    for (auto phrase_it = trainingSet->begin(); phrase_it != trainingSet->end();
         phrase_it++) {
        unsigned int step = phrase_it->second->size() / numStates;
        unsigned int offset(0);
        for (unsigned int n = 0; n < numStates; n++) {
            for (unsigned int t = 0; t < step; t++) {
                for (unsigned int d = 0; d < dimension; d++) {
                    states[n].components[0].mean[d] +=
                        phrase_it->second->getValue(offset + t, d);
                }
            }
            offset += step;
            factor[n] += step;
        }
    }

    for (int n = 0; n < numStates; n++)
        for (int d = 0; d < dimension; d++)
            states[n].components[0].mean[d] /= factor[n];
}

void xmm::SingleClassHMM::initCovariances_fullyObserved(
    TrainingSet* trainingSet) {
    // TODO: simplify with covariance symmetricity.
    if (!trainingSet || trainingSet->empty()) return;
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int numStates = parameters.states.get();

    if (parameters.covariance_mode.get() ==
        GaussianDistribution::CovarianceMode::Full) {
        for (int n = 0; n < numStates; n++)
            states[n].components[0].covariance.assign(dimension * dimension,
                                                      0.0);
    } else {
        for (int n = 0; n < numStates; n++)
            states[n].components[0].covariance.assign(dimension, 0.0);
    }

    std::vector<int> factor(numStates, 0);
    std::vector<double> othermeans(numStates * dimension, 0.0);
    for (auto phrase_it = trainingSet->begin(); phrase_it != trainingSet->end();
         phrase_it++) {
        unsigned int step = phrase_it->second->size() / numStates;
        unsigned int offset(0);
        for (unsigned int n = 0; n < numStates; n++) {
            for (unsigned int t = 0; t < step; t++) {
                for (unsigned int d1 = 0; d1 < dimension; d1++) {
                    othermeans[n * dimension + d1] +=
                        phrase_it->second->getValue(offset + t, d1);
                    if (parameters.covariance_mode.get() ==
                        GaussianDistribution::CovarianceMode::Full) {
                        for (int d2 = 0; d2 < dimension; d2++) {
                            states[n].components[0].covariance[d1 * dimension +
                                                               d2] +=
                                phrase_it->second->getValue(offset + t, d1) *
                                phrase_it->second->getValue(offset + t, d2);
                        }
                    } else {
                        float value =
                            phrase_it->second->getValue(offset + t, d1);
                        states[n].components[0].covariance[d1] += value * value;
                    }
                }
            }
            offset += step;
            factor[n] += step;
        }
    }

    for (unsigned int n = 0; n < numStates; n++)
        for (unsigned int d1 = 0; d1 < dimension; d1++) {
            othermeans[n * dimension + d1] /= factor[n];
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                for (int d2 = 0; d2 < dimension; d2++)
                    states[n].components[0].covariance[d1 * dimension + d2] /=
                        factor[n];
            } else {
                states[n].components[0].covariance[d1] /= factor[n];
            }
        }

    for (int n = 0; n < numStates; n++) {
        for (int d1 = 0; d1 < dimension; d1++) {
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                for (int d2 = 0; d2 < dimension; d2++)
                    states[n].components[0].covariance[d1 * dimension + d2] -=
                        othermeans[n * dimension + d1] *
                        othermeans[n * dimension + d2];
            } else {
                states[n].components[0].covariance[d1] -=
                    othermeans[n * dimension + d1] *
                    othermeans[n * dimension + d1];
            }
        }
        states[n].addCovarianceOffset();
        states[n].updateInverseCovariances();
    }
}

void xmm::SingleClassHMM::initMeansCovariancesWithGMMEM(
    TrainingSet* trainingSet) {
    unsigned int numStates = parameters.states.get();

    for (unsigned int n = 0; n < numStates; n++) {
        TrainingSet temp_ts(MemoryMode::SharedMemory,
                            shared_parameters->bimodal.get()
                                ? Multimodality::Bimodal
                                : Multimodality::Unimodal);
        temp_ts.dimension.set(shared_parameters->dimension.get());
        temp_ts.dimension_input.set(shared_parameters->dimension_input.get());
        for (auto phrase_it = trainingSet->begin();
             phrase_it != trainingSet->end(); phrase_it++) {
            unsigned int step = phrase_it->second->size() / numStates;
            if (step == 0) continue;
            temp_ts.addPhrase(phrase_it->first, label);
            if (shared_parameters->bimodal.get())
                temp_ts.getPhrase(phrase_it->first)
                    ->connect(phrase_it->second->getPointer_input(n * step),
                              phrase_it->second->getPointer_output(n * step),
                              step);
            else
                temp_ts.getPhrase(phrase_it->first)
                    ->connect(phrase_it->second->getPointer(n * step), step);
        }
        if (temp_ts.empty()) continue;
        SingleClassGMM tmpGMM(shared_parameters);
        tmpGMM.parameters.gaussians.set(parameters.gaussians.get());
        tmpGMM.parameters.relative_regularization.set(
            parameters.relative_regularization.get());
        tmpGMM.parameters.absolute_regularization.set(
            parameters.absolute_regularization.get());
        tmpGMM.parameters.covariance_mode.set(parameters.covariance_mode.get());
        tmpGMM.train(&temp_ts);
        for (unsigned int c = 0; c < parameters.gaussians.get(); c++) {
            states[n].components[c].mean = tmpGMM.components[c].mean;
            states[n].components[c].covariance =
                tmpGMM.components[c].covariance;
            states[n].updateInverseCovariances();
        }
    }
}

void xmm::SingleClassHMM::setErgodic() {
    unsigned int numStates = parameters.states.get();

    for (int i = 0; i < numStates; i++) {
        prior[i] = 1 / (float)numStates;
        for (int j = 0; j < numStates; j++) {
            transition[i * numStates + j] = 1 / (float)numStates;
        }
    }
}

void xmm::SingleClassHMM::setLeftRight() {
    unsigned int numStates = parameters.states.get();

    transition.assign(numStates * 2, 0.5);
    transition[(numStates - 1) * 2] = 1.;
    transition[(numStates - 1) * 2 + 1] = 0.;
}

void xmm::SingleClassHMM::normalizeTransitions() {
    unsigned int numStates = parameters.states.get();

    double norm_transition;
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        double norm_prior(0.);
        for (int i = 0; i < numStates; i++) {
            norm_prior += prior[i];
            norm_transition = 0.;
            for (int j = 0; j < numStates; j++)
                norm_transition += transition[i * numStates + j];
            for (int j = 0; j < numStates; j++)
                transition[i * numStates + j] /= norm_transition;
        }
        for (int i = 0; i < numStates; i++) prior[i] /= norm_prior;
    } else {
        for (int i = 0; i < numStates; i++) {
            norm_transition = transition[i * 2] + transition[i * 2 + 1];
            transition[i * 2] /= norm_transition;
            transition[i * 2 + 1] /= norm_transition;
        }
    }
}

#pragma mark -
#pragma mark Forward-Backward algorithm
double xmm::SingleClassHMM::forward_init(const float* observation,
                                         const float* observation_output) {
    unsigned int numStates = parameters.states.get();

    double norm_const(0.);
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        for (int i = 0; i < numStates; i++) {
            if (shared_parameters->bimodal.get()) {
                if (observation_output)
                    alpha[i] = prior[i] *
                               states[i].obsProb_bimodal(observation,
                                                         observation_output);
                else
                    alpha[i] = prior[i] * states[i].obsProb_input(observation);
            } else {
                alpha[i] = prior[i] * states[i].obsProb(observation);
            }
            norm_const += alpha[i];
        }
    } else {
        alpha.assign(numStates, 0.0);
        if (shared_parameters->bimodal.get()) {
            if (observation_output)
                alpha[0] =
                    states[0].obsProb_bimodal(observation, observation_output);
            else
                alpha[0] = states[0].obsProb_input(observation);
        } else {
            alpha[0] = states[0].obsProb(observation);
        }
        norm_const += alpha[0];
    }
    if (norm_const > 0) {
        for (int i = 0; i < numStates; i++) {
            alpha[i] /= norm_const;
        }
        return 1 / norm_const;
    } else {
        for (int j = 0; j < numStates; j++) {
            alpha[j] = 1. / double(numStates);
        }
        return 1.;
    }
}

double xmm::SingleClassHMM::forward_update(const float* observation,
                                           const float* observation_output) {
    unsigned int numStates = parameters.states.get();

    double norm_const(0.);
    previous_alpha_ = alpha;
    for (int j = 0; j < numStates; j++) {
        alpha[j] = 0.;
        if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
            for (int i = 0; i < numStates; i++) {
                alpha[j] += previous_alpha_[i] * transition[i * numStates + j];
            }
        } else {
            alpha[j] += previous_alpha_[j] * transition[j * 2];
            if (j > 0) {
                alpha[j] +=
                    previous_alpha_[j - 1] * transition[(j - 1) * 2 + 1];
            } else {
                alpha[0] += previous_alpha_[numStates - 1] *
                            transition[numStates * 2 - 1];
            }
        }
        if (shared_parameters->bimodal.get()) {
            if (observation_output)
                alpha[j] *=
                    states[j].obsProb_bimodal(observation, observation_output);
            else
                alpha[j] *= states[j].obsProb_input(observation);
        } else {
            alpha[j] *= states[j].obsProb(observation);
        }
        norm_const += alpha[j];
    }
    if (norm_const > 1e-300) {
        for (int j = 0; j < numStates; j++) {
            alpha[j] /= norm_const;
        }
        return 1. / norm_const;
    } else {
        return 0.;
    }
}

void xmm::SingleClassHMM::backward_init(double ct) {
    for (int i = 0; i < parameters.states.get(); i++) beta_[i] = ct;
}

void xmm::SingleClassHMM::backward_update(double ct, const float* observation,
                                          const float* observation_output) {
    unsigned int numStates = parameters.states.get();

    previous_beta_ = beta_;
    for (int i = 0; i < numStates; i++) {
        beta_[i] = 0.;
        if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
            for (int j = 0; j < numStates; j++) {
                if (shared_parameters->bimodal.get()) {
                    if (observation_output)
                        beta_[i] += transition[i * numStates + j] *
                                    previous_beta_[j] *
                                    states[j].obsProb_bimodal(
                                        observation, observation_output);
                    else
                        beta_[i] += transition[i * numStates + j] *
                                    previous_beta_[j] *
                                    states[j].obsProb_input(observation);
                } else {
                    beta_[i] += transition[i * numStates + j] *
                                previous_beta_[j] *
                                states[j].obsProb(observation);
                }
            }
        } else {
            if (shared_parameters->bimodal.get()) {
                if (observation_output)
                    beta_[i] += transition[i * 2] * previous_beta_[i] *
                                states[i].obsProb_bimodal(observation,
                                                          observation_output);
                else
                    beta_[i] += transition[i * 2] * previous_beta_[i] *
                                states[i].obsProb_input(observation);
            } else {
                beta_[i] += transition[i * 2] * previous_beta_[i] *
                            states[i].obsProb(observation);
            }
            if (i < numStates - 1) {
                if (shared_parameters->bimodal.get()) {
                    if (observation_output)
                        beta_[i] += transition[i * 2 + 1] *
                                    previous_beta_[i + 1] *
                                    states[i + 1].obsProb_bimodal(
                                        observation, observation_output);
                    else
                        beta_[i] += transition[i * 2 + 1] *
                                    previous_beta_[i + 1] *
                                    states[i + 1].obsProb_input(observation);
                } else {
                    beta_[i] += transition[i * 2 + 1] * previous_beta_[i + 1] *
                                states[i + 1].obsProb(observation);
                }
            }
        }
        beta_[i] *= ct;
        if (std::isnan(beta_[i]) || std::isinf(fabs(beta_[i]))) {
            beta_[i] = 1e100;
        }
    }
}

#pragma mark -
#pragma mark Training algorithm
void xmm::SingleClassHMM::emAlgorithmInit(TrainingSet* trainingSet) {
    if (!trainingSet || trainingSet->empty()) return;
    unsigned int numStates = parameters.states.get();
    unsigned int numGaussians = parameters.gaussians.get();

    initParametersToDefault(trainingSet->standardDeviation());

    if (numGaussians > 0) {  // TODO: weird > 0
        initMeansCovariancesWithGMMEM(trainingSet);
    } else {
        initMeansWithAllPhrases(trainingSet);
        initCovariances_fullyObserved(trainingSet);
    }

    unsigned int nbPhrases = trainingSet->size();

    // Initialize Algorithm variables
    // ---------------------------------------
    gamma_sequence_.resize(nbPhrases);
    epsilon_sequence_.resize(nbPhrases);
    gamma_sequence_per_mixture_.resize(nbPhrases);
    unsigned int maxT(0);
    unsigned int i(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it) {
        unsigned int T = it->second->size();
        gamma_sequence_[i].resize(T * numStates);
        if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
            epsilon_sequence_[i].resize(T * numStates * numStates);
        } else {
            epsilon_sequence_[i].resize(T * 2 * numStates);
        }
        gamma_sequence_per_mixture_[i].resize(numGaussians);
        for (int c = 0; c < numGaussians; c++) {
            gamma_sequence_per_mixture_[i][c].resize(T * numStates);
        }
        if (T > maxT) {
            maxT = T;
        }
        i++;
    }
    alpha_seq_.resize(maxT * numStates);
    beta_seq_.resize(maxT * numStates);

    gamma_sum_.resize(numStates);
    gamma_sum_per_mixture_.resize(numStates * numGaussians);
}

void xmm::SingleClassHMM::emAlgorithmTerminate() {
    normalizeTransitions();
    gamma_sequence_.clear();
    epsilon_sequence_.clear();
    gamma_sequence_per_mixture_.clear();
    alpha_seq_.clear();
    beta_seq_.clear();
    gamma_sum_.clear();
    gamma_sum_per_mixture_.clear();
    SingleClassProbabilisticModel::emAlgorithmTerminate();
}

double xmm::SingleClassHMM::emAlgorithmUpdate(TrainingSet* trainingSet) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int numStates = parameters.states.get();

    double log_prob(0.);

    // Forward-backward for each phrase
    // =================================================
    int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it) {
        if (it->second->size() > 0)
            log_prob += baumWelch_forwardBackward(it->second, phraseIndex);
        phraseIndex++;
    }

    baumWelch_gammaSum(trainingSet);

    // Re-estimate model parameters
    // =================================================

    // set covariance and mixture coefficients to zero for each state
    for (int i = 0; i < numStates; i++) {
        for (int c = 0; c < parameters.gaussians.get(); c++) {
            states[i].mixture_coeffs[c] = 0.;
            if (parameters.covariance_mode.get() ==
                GaussianDistribution::CovarianceMode::Full) {
                states[i].components[c].covariance.assign(dimension * dimension,
                                                          0.0);
            } else {
                states[i].components[c].covariance.assign(dimension, 0.0);
            }
        }
    }

    baumWelch_estimateMixtureCoefficients(trainingSet);
    baumWelch_estimateMeans(trainingSet);
    baumWelch_estimateCovariances(trainingSet);
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic)
        baumWelch_estimatePrior(trainingSet);
    baumWelch_estimateTransitions(trainingSet);

    return log_prob;
}

double xmm::SingleClassHMM::baumWelch_forward_update(
    std::vector<double>::iterator observation_likelihoods) {
    unsigned int numStates = parameters.states.get();

    double norm_const(0.);
    previous_alpha_ = alpha;
    for (int j = 0; j < numStates; j++) {
        alpha[j] = 0.;
        if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
            for (int i = 0; i < numStates; i++) {
                alpha[j] += previous_alpha_[i] * transition[i * numStates + j];
            }
        } else {
            alpha[j] += previous_alpha_[j] * transition[j * 2];
            if (j > 0) {
                alpha[j] +=
                    previous_alpha_[j - 1] * transition[(j - 1) * 2 + 1];
            } else {
                alpha[0] += previous_alpha_[numStates - 1] *
                            transition[numStates * 2 - 1];
            }
        }
        alpha[j] *= observation_likelihoods[j];
        norm_const += alpha[j];
    }
    if (norm_const > 1e-300) {
        for (int j = 0; j < numStates; j++) {
            alpha[j] /= norm_const;
        }
        return 1. / norm_const;
    } else {
        return 0.;
    }
}

void xmm::SingleClassHMM::baumWelch_backward_update(
    double ct, std::vector<double>::iterator observation_likelihoods) {
    unsigned int numStates = parameters.states.get();

    previous_beta_ = beta_;
    for (int i = 0; i < numStates; i++) {
        beta_[i] = 0.;
        if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
            for (int j = 0; j < numStates; j++) {
                beta_[i] += transition[i * numStates + j] * previous_beta_[j] *
                            observation_likelihoods[j];
            }
        } else {
            beta_[i] += transition[i * 2] * previous_beta_[i] *
                        observation_likelihoods[i];
            if (i < numStates - 1) {
                beta_[i] += transition[i * 2 + 1] * previous_beta_[i + 1] *
                            observation_likelihoods[i + 1];
            }
        }
        beta_[i] *= ct;
        if (std::isnan(beta_[i]) || std::isinf(fabs(beta_[i]))) {
            beta_[i] = 1e100;
        }
    }
}

double xmm::SingleClassHMM::baumWelch_forwardBackward(
    std::shared_ptr<Phrase> currentPhrase, int phraseIndex) {
    unsigned int T = currentPhrase->size();
    unsigned int numStates = parameters.states.get();

    std::vector<double> ct(T);
    std::vector<double>::iterator alpha_seq_it = alpha_seq_.begin();

    double log_prob;

    std::vector<double> observation_probabilities(numStates * T);
    for (unsigned int t = 0; t < T; ++t) {
        for (unsigned int i = 0; i < numStates; i++) {
            if (shared_parameters->bimodal.get()) {
                observation_probabilities[t * numStates + i] =
                    states[i].obsProb_bimodal(
                        currentPhrase->getPointer_input(t),
                        currentPhrase->getPointer_output(t));
            } else {
                observation_probabilities[t * numStates + i] =
                    states[i].obsProb(currentPhrase->getPointer(t));
            }
        }
    }

    // Forward algorithm
    if (shared_parameters->bimodal.get()) {
        ct[0] = forward_init(currentPhrase->getPointer_input(0),
                             currentPhrase->getPointer_output(0));
    } else {
        ct[0] = forward_init(currentPhrase->getPointer(0));
    }
    log_prob = -log(ct[0]);
    copy(alpha.begin(), alpha.end(), alpha_seq_it);
    alpha_seq_it += numStates;

    for (int t = 1; t < T; t++) {
        ct[t] = baumWelch_forward_update(observation_probabilities.begin() +
                                         t * numStates);
        log_prob -= log(ct[t]);
        copy(alpha.begin(), alpha.end(), alpha_seq_it);
        alpha_seq_it += numStates;
    }

    // Backward algorithm
    backward_init(ct[T - 1]);
    copy(beta_.begin(), beta_.end(), beta_seq_.begin() + (T - 1) * numStates);

    for (int t = int(T - 2); t >= 0; t--) {
        baumWelch_backward_update(
            ct[t], observation_probabilities.begin() + (t + 1) * numStates);
        copy(beta_.begin(), beta_.end(), beta_seq_.begin() + t * numStates);
    }

    // Compute Gamma Variable
    for (int t = 0; t < T; t++) {
        for (int i = 0; i < numStates; i++) {
            gamma_sequence_[phraseIndex][t * numStates + i] =
                alpha_seq_[t * numStates + i] * beta_seq_[t * numStates + i] /
                ct[t];
        }
    }

    // Compute Gamma variable for each mixture component
    double oo;
    double norm_const;

    for (int t = 0; t < T; t++) {
        for (int i = 0; i < numStates; i++) {
            norm_const = 0.;
            if (parameters.gaussians.get() == 1) {
                oo = observation_probabilities[t * numStates + i];
                gamma_sequence_per_mixture_[phraseIndex][0][t * numStates + i] =
                    gamma_sequence_[phraseIndex][t * numStates + i] * oo;
                norm_const += oo;
            } else {
                for (int c = 0; c < parameters.gaussians.get(); c++) {
                    if (shared_parameters->bimodal.get()) {
                        oo = states[i].obsProb_bimodal(
                            currentPhrase->getPointer_input(t),
                            currentPhrase->getPointer_output(t), c);
                    } else {
                        oo = states[i].obsProb(currentPhrase->getPointer(t), c);
                    }
                    gamma_sequence_per_mixture_[phraseIndex][c][t * numStates +
                                                                i] =
                        gamma_sequence_[phraseIndex][t * numStates + i] * oo;
                    norm_const += oo;
                }
            }
            if (norm_const > 0)
                for (int c = 0; c < parameters.gaussians.get(); c++)
                    gamma_sequence_per_mixture_[phraseIndex][c][t * numStates +
                                                                i] /=
                        norm_const;
        }
    }

    // Compute Epsilon Variable
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        for (int t = 0; t < T - 1; t++) {
            for (int i = 0; i < numStates; i++) {
                for (int j = 0; j < numStates; j++) {
                    epsilon_sequence_[phraseIndex][t * numStates * numStates +
                                                   i * numStates + j] =
                        alpha_seq_[t * numStates + i] *
                        transition[i * numStates + j] *
                        beta_seq_[(t + 1) * numStates + j];
                    epsilon_sequence_[phraseIndex][t * numStates * numStates +
                                                   i * numStates + j] *=
                        observation_probabilities[(t + 1) * numStates + j];
                }
            }
        }
    } else {
        for (int t = 0; t < T - 1; t++) {
            for (int i = 0; i < numStates; i++) {
                epsilon_sequence_[phraseIndex][t * 2 * numStates + i * 2] =
                    alpha_seq_[t * numStates + i] * transition[i * 2] *
                    beta_seq_[(t + 1) * numStates + i];
                epsilon_sequence_[phraseIndex][t * 2 * numStates + i * 2] *=
                    observation_probabilities[(t + 1) * numStates + i];
                if (i < numStates - 1) {
                    epsilon_sequence_[phraseIndex][t * 2 * numStates + i * 2 +
                                                   1] =
                        alpha_seq_[t * numStates + i] * transition[i * 2 + 1] *
                        beta_seq_[(t + 1) * numStates + i + 1];
                    epsilon_sequence_[phraseIndex][t * 2 * numStates + i * 2 +
                                                   1] *=
                        observation_probabilities[(t + 1) * numStates + i + 1];
                }
            }
        }
    }

    return log_prob;
}

void xmm::SingleClassHMM::baumWelch_gammaSum(TrainingSet* trainingSet) {
    unsigned int numStates = parameters.states.get();
    unsigned int numGaussians = parameters.gaussians.get();

    for (int i = 0; i < numStates; i++) {
        gamma_sum_[i] = 0.;
        for (int c = 0; c < numGaussians; c++) {
            gamma_sum_per_mixture_[i * numGaussians + c] = 0.;
        }
    }

    unsigned int phraseLength;
    unsigned int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it) {
        phraseLength = it->second->size();
        for (int i = 0; i < numStates; i++) {
            for (int t = 0; t < phraseLength; t++) {
                gamma_sum_[i] +=
                    gamma_sequence_[phraseIndex][t * numStates + i];
                for (int c = 0; c < numGaussians; c++) {
                    gamma_sum_per_mixture_[i * numGaussians + c] +=
                        gamma_sequence_per_mixture_[phraseIndex][c]
                                                   [t * numStates + i];
                }
            }
        }
        phraseIndex++;
    }
}

void xmm::SingleClassHMM::baumWelch_estimateMixtureCoefficients(
    TrainingSet* trainingSet) {
    unsigned int numStates = parameters.states.get();

    unsigned int phraseLength;
    unsigned int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); ++it) {
        phraseLength = it->second->size();
        for (int i = 0; i < numStates; i++) {
            for (int t = 0; t < phraseLength; t++) {
                for (int c = 0; c < parameters.gaussians.get(); c++) {
                    states[i].mixture_coeffs[c] += gamma_sequence_per_mixture_
                        [phraseIndex][c][t * numStates + i];
                }
            }
        }
        phraseIndex++;
    }

    // Scale mixture coefficients
    for (int i = 0; i < numStates; i++) {
        states[i].normalizeMixtureCoeffs();
    }
}

void xmm::SingleClassHMM::baumWelch_estimateMeans(TrainingSet* trainingSet) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int numStates = parameters.states.get();
    unsigned int numGaussians = parameters.gaussians.get();

    unsigned int phraseLength;

    for (int i = 0; i < numStates; i++) {
        for (int c = 0; c < numGaussians; c++) {
            states[i].components[c].mean.assign(dimension, 0.0);
        }
    }

    // Re-estimate Mean
    int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); it++) {
        phraseLength = it->second->size();
        for (int i = 0; i < numStates; i++) {
            for (int t = 0; t < phraseLength; t++) {
                for (int c = 0; c < numGaussians; c++) {
                    for (int d = 0; d < dimension; d++) {
                        states[i].components[c].mean[d] +=
                            gamma_sequence_per_mixture_[phraseIndex][c]
                                                       [t * numStates + i] *
                            it->second->getValue(t, d);
                    }
                }
            }
        }
        phraseIndex++;
    }

    // Normalize mean
    for (int i = 0; i < numStates; i++) {
        for (int c = 0; c < numGaussians; c++) {
            for (int d = 0; d < dimension; d++) {
                if (gamma_sum_per_mixture_[i * numGaussians + c] > 0) {
                    states[i].components[c].mean[d] /=
                        gamma_sum_per_mixture_[i * numGaussians + c];
                }
                if (std::isnan(states[i].components[c].mean[d]))
                    throw std::runtime_error("Convergence Error");
            }
        }
    }
}

void xmm::SingleClassHMM::baumWelch_estimateCovariances(
    TrainingSet* trainingSet) {
    int dimension = static_cast<int>(shared_parameters->dimension.get());
    unsigned int numStates = parameters.states.get();
    unsigned int numGaussians = parameters.gaussians.get();

    unsigned int phraseLength;

    int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); it++) {
        phraseLength = it->second->size();
        for (int i = 0; i < numStates; i++) {
            for (int t = 0; t < phraseLength; t++) {
                for (int c = 0; c < numGaussians; c++) {
                    for (int d1 = 0; d1 < dimension; d1++) {
                        if (parameters.covariance_mode.get() ==
                            GaussianDistribution::CovarianceMode::Full) {
                            for (int d2 = d1; d2 < dimension; d2++) {
                                states[i]
                                    .components[c]
                                    .covariance[d1 * dimension + d2] +=
                                    gamma_sequence_per_mixture_
                                        [phraseIndex][c][t * numStates + i] *
                                    (it->second->getValue(t, d1) -
                                     states[i].components[c].mean[d1]) *
                                    (it->second->getValue(t, d2) -
                                     states[i].components[c].mean[d2]);
                            }
                        } else {
                            float value = it->second->getValue(t, d1) -
                                          states[i].components[c].mean[d1];
                            states[i].components[c].covariance[d1] +=
                                gamma_sequence_per_mixture_[phraseIndex][c]
                                                           [t * numStates + i] *
                                value * value;
                        }
                    }
                }
            }
        }
        phraseIndex++;
    }

    // Scale covariance
    for (int i = 0; i < numStates; i++) {
        for (int c = 0; c < numGaussians; c++) {
            if (gamma_sum_per_mixture_[i * numGaussians + c] > 0) {
                for (int d1 = 0; d1 < dimension; d1++) {
                    if (parameters.covariance_mode.get() ==
                        GaussianDistribution::CovarianceMode::Full) {
                        for (int d2 = d1; d2 < dimension; d2++) {
                            states[i].components[c].covariance[d1 * dimension +
                                                               d2] /=
                                gamma_sum_per_mixture_[i * numGaussians + c];
                            if (d1 != d2)
                                states[i]
                                    .components[c]
                                    .covariance[d2 * dimension + d1] =
                                    states[i]
                                        .components[c]
                                        .covariance[d1 * dimension + d2];
                        }
                    } else {
                        states[i].components[c].covariance[d1] /=
                            gamma_sum_per_mixture_[i * numGaussians + c];
                    }
                }
            }
        }
        states[i].addCovarianceOffset();
        states[i].updateInverseCovariances();
    }
}

void xmm::SingleClassHMM::baumWelch_estimatePrior(TrainingSet* trainingSet) {
    unsigned int numStates = parameters.states.get();

    // Set prior vector to 0
    for (int i = 0; i < numStates; i++) prior[i] = 0.;

    // Re-estimate Prior probabilities
    double sumprior = 0.;
    int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); it++) {
        for (int i = 0; i < numStates; i++) {
            prior[i] += gamma_sequence_[phraseIndex][i];
            sumprior += gamma_sequence_[phraseIndex][i];
        }
        phraseIndex++;
    }

    // Scale Prior vector
    if (sumprior > 0.) {
        for (int i = 0; i < numStates; i++) {
            prior[i] /= sumprior;
        }
    }
}

void xmm::SingleClassHMM::baumWelch_estimateTransitions(
    TrainingSet* trainingSet) {
    unsigned int numStates = parameters.states.get();

    // Set prior vector and transition matrix to 0
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        transition.assign(numStates * numStates, 0.0);
    } else {
        transition.assign(numStates * 2, 0.0);
    }

    unsigned int phraseLength;
    // Re-estimate Prior and Transition probabilities
    unsigned int phraseIndex(0);
    for (auto it = trainingSet->cbegin(); it != trainingSet->cend(); it++) {
        phraseLength = it->second->size();
        if (phraseLength > 0) {
            for (int i = 0; i < numStates; i++) {
                // Experimental: A bit of regularization (sometimes avoids
                // numerical
                // errors)
                if (parameters.transition_mode.get() ==
                    HMM::TransitionMode::LeftRight) {
                    transition[i * 2] += TRANSITION_REGULARIZATION();
                    if (i < numStates - 1)
                        transition[i * 2 + 1] += TRANSITION_REGULARIZATION();
                    else
                        transition[i * 2] += TRANSITION_REGULARIZATION();
                }
                // End Regularization
                if (parameters.transition_mode.get() ==
                    HMM::TransitionMode::Ergodic) {
                    for (int j = 0; j < numStates; j++) {
                        for (int t = 0; t < phraseLength - 1; t++) {
                            transition[i * numStates + j] +=
                                epsilon_sequence_[phraseIndex][t * numStates *
                                                                   numStates +
                                                               i * numStates +
                                                               j];
                        }
                    }
                } else {
                    for (int t = 0; t < phraseLength - 1; t++) {
                        transition[i * 2] +=
                            epsilon_sequence_[phraseIndex][t * 2 * numStates +
                                                           i * 2];
                    }
                    if (i < numStates - 1) {
                        for (int t = 0; t < phraseLength - 1; t++) {
                            transition[i * 2 + 1] +=
                                epsilon_sequence_[phraseIndex][t * 2 *
                                                                   numStates +
                                                               i * 2 + 1];
                        }
                    }
                }
            }
        }
        phraseIndex++;
    }

    // Scale transition matrix
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        for (int i = 0; i < numStates; i++) {
            for (int j = 0; j < numStates; j++) {
                transition[i * numStates + j] /=
                    (gamma_sum_[i] + 2. * TRANSITION_REGULARIZATION());
                if (std::isnan(transition[i * numStates + j]))
                    throw std::runtime_error(
                        "Convergence Error. Check your training data or "
                        "increase the variance offset");
            }
        }
    } else {
        for (int i = 0; i < numStates; i++) {
            transition[i * 2] /=
                (gamma_sum_[i] + 2. * TRANSITION_REGULARIZATION());
            if (std::isnan(transition[i * 2]))
                throw std::runtime_error(
                    "Convergence Error. Check your training data or increase "
                    "the variance offset");
            if (i < numStates - 1) {
                transition[i * 2 + 1] /=
                    (gamma_sum_[i] + 2. * TRANSITION_REGULARIZATION());
                if (std::isnan(transition[i * 2 + 1]))
                    throw std::runtime_error(
                        "Convergence Error. Check your training data or "
                        "increase the variance offset");
            }
        }
    }
}

#pragma mark -
#pragma mark Performance

void xmm::SingleClassHMM::reset() {
    check_training();
    SingleClassProbabilisticModel::reset();
    forward_initialized_ = false;
    if (is_hierarchical_) {
        for (int i = 0; i < 3; i++)
            alpha_h[i].resize(parameters.states.get(), 0.0);
        alpha.clear();
        previous_alpha_.clear();
        beta_.clear();
        previous_beta_.clear();
    } else {
        addCyclicTransition(0.05);
    }
}

void xmm::SingleClassHMM::addCyclicTransition(double proba) {
    unsigned int numStates = parameters.states.get();

    check_training();
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic) {
        if (numStates > 1) transition[(numStates - 1) * numStates] = proba;
    } else {
        if (numStates > 1) transition[(numStates - 1) * 2 + 1] = proba;
    }
}

double xmm::SingleClassHMM::filter(std::vector<float> const& observation) {
    check_training();
    double ct;

    if (forward_initialized_) {
        ct = forward_update(&observation[0]);
    } else {
        this->likelihood_buffer_.clear();
        ct = forward_init(&observation[0]);
    }

    forward_initialized_ = true;

    results.instant_likelihood = 1. / ct;
    updateAlphaWindow();
    updateResults();

    if (shared_parameters->bimodal.get()) {
        regression(observation);
    }

    return results.instant_likelihood;
}

unsigned int argmax(std::vector<double> const& v) {
    unsigned int amax(-1);
    if (v.size() == 0) return amax;
    double current_max(v[0]);
    for (unsigned int i = 1; i < v.size(); ++i) {
        if (v[i] > current_max) {
            current_max = v[i];
            amax = i;
        }
    }
    return amax;
}

void xmm::SingleClassHMM::updateAlphaWindow() {
    unsigned int numStates = parameters.states.get();

    check_training();
    results.likeliest_state = 0;
    // Get likeliest State
    double best_alpha(is_hierarchical_ ? (alpha_h[0][0] + alpha_h[1][0])
                                       : alpha[0]);
    for (unsigned int i = 1; i < numStates; ++i) {
        if (is_hierarchical_) {
            if ((alpha_h[0][i] + alpha_h[1][i]) > best_alpha) {
                best_alpha = alpha_h[0][i] + alpha_h[1][i];
                results.likeliest_state = i;
            }
        } else {
            if (alpha[i] > best_alpha) {
                best_alpha = alpha[i];
                results.likeliest_state = i;
            }
        }
    }

    // Compute Window
    window_minindex_ = (static_cast<int>(results.likeliest_state) -
                        static_cast<int>(numStates) / 2);
    window_maxindex_ = (static_cast<int>(results.likeliest_state) +
                        static_cast<int>(numStates) / 2);
    window_minindex_ = (window_minindex_ >= 0) ? window_minindex_ : 0;
    window_maxindex_ = (window_maxindex_ <= static_cast<int>(numStates))
                           ? window_maxindex_
                           : static_cast<int>(numStates);
    window_normalization_constant_ = 0.0;
    for (int i = window_minindex_; i < window_maxindex_; ++i) {
        window_normalization_constant_ +=
            is_hierarchical_ ? (alpha_h[0][i] + alpha_h[1][i]) : alpha[i];
    }
}

void xmm::SingleClassHMM::regression(
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
    std::vector<float> tmp_predicted_output(dimension_output);

    if (parameters.regression_estimator.get() ==
        HMM::RegressionEstimator::Likeliest) {
        states[results.likeliest_state].likelihood(observation_input);
        states[results.likeliest_state].regression(observation_input);
        results.output_values =
            states[results.likeliest_state].results.output_values;
        return;
    }

    unsigned int clip_min_state = (parameters.regression_estimator.get() ==
                                  HMM::RegressionEstimator::Full)
                                     ? 0
                                     : window_minindex_;
    unsigned int clip_max_state = (parameters.regression_estimator.get() ==
                                  HMM::RegressionEstimator::Full)
                                     ? parameters.states.get()
                                     : window_maxindex_;
    double normalization_constant = (parameters.regression_estimator.get() ==
                                     HMM::RegressionEstimator::Full)
                                        ? 1.0
                                        : window_normalization_constant_;

    if (normalization_constant <= 0.0) normalization_constant = 1.;

    // Compute Regression
    for (unsigned int i = clip_min_state; i < clip_max_state; ++i) {
        states[i].likelihood(observation_input);
        states[i].regression(observation_input);
        tmp_predicted_output = states[i].results.output_values;
        for (unsigned int d = 0; d < dimension_output; ++d) {
            if (is_hierarchical_) {
                results.output_values[d] += (alpha_h[0][i] + alpha_h[1][i]) *
                                            tmp_predicted_output[d] /
                                            normalization_constant;
                if (parameters.covariance_mode.get() ==
                    GaussianDistribution::CovarianceMode::Full) {
                    for (int d2 = 0; d2 < dimension_output; ++d2)
                        results.output_covariance[d * dimension_output + d2] +=
                            (alpha_h[0][i] + alpha_h[1][i]) *
                            (alpha_h[0][i] + alpha_h[1][i]) *
                            states[i]
                                .results
                                .output_covariance[d * dimension_output + d2] /
                            normalization_constant;
                } else {
                    results.output_covariance[d] +=
                        (alpha_h[0][i] + alpha_h[1][i]) *
                        (alpha_h[0][i] + alpha_h[1][i]) *
                        states[i].results.output_covariance[d] /
                        normalization_constant;
                }
            } else {
                results.output_values[d] +=
                    alpha[i] * tmp_predicted_output[d] / normalization_constant;
                if (parameters.covariance_mode.get() ==
                    GaussianDistribution::CovarianceMode::Full) {
                    for (int d2 = 0; d2 < dimension_output; ++d2)
                        results.output_covariance[d * dimension_output + d2] +=
                            alpha[i] * alpha[i] *
                            states[i]
                                .results
                                .output_covariance[d * dimension_output + d2] /
                            normalization_constant;

                } else {
                    results.output_covariance[d] +=
                        alpha[i] * alpha[i] *
                        states[i].results.output_covariance[d] /
                        normalization_constant;
                }
            }
        }
    }
}

void xmm::SingleClassHMM::updateResults() {
    likelihood_buffer_.push(log(results.instant_likelihood));
    results.log_likelihood = 0.0;
    unsigned int bufSize = likelihood_buffer_.size_t();
    for (unsigned int i = 0; i < bufSize; i++) {
        results.log_likelihood += likelihood_buffer_(0, i);
    }
    results.log_likelihood /= double(bufSize);

    results.progress = 0.0;
    for (int i = window_minindex_; i < window_maxindex_; ++i) {
        if (is_hierarchical_)
            results.progress +=
                (alpha_h[0][i] + alpha_h[1][i] + alpha_h[2][i]) * i /
                window_normalization_constant_;
        else
            results.progress += alpha[i] * i / window_normalization_constant_;
    }
    results.progress /= double(parameters.states.get() - 1);

    //    /////////////////////////
    //    results_progress = 0.0;
    //    for (unsigned int i=0 ; i<numStates; i++) {
    //        if (is_hierarchical_)
    //            results_progress += (alpha_h[0][i] + alpha_h[1][i] +
    //            alpha_h[2][i]) * i;
    //        else
    //            results_progress += alpha[i] * i;
    //    }
    //    results_progress /= double(numStates-1);
    //    /////////////////////////
}

#pragma mark -
#pragma mark File IO
Json::Value xmm::SingleClassHMM::toJson() const {
    check_training();
    Json::Value root = SingleClassProbabilisticModel::toJson();
    root["parameters"] = parameters.toJson();
    if (parameters.transition_mode.get() == HMM::TransitionMode::Ergodic)
        root["prior"] = vector2json(prior);
    root["transition"] = vector2json(transition);
    root["exitProbabilities"] = vector2json(exit_probabilities_);

    root["states"].resize(
        static_cast<Json::ArrayIndex>(parameters.states.get()));
    for (int s = 0; s < parameters.states.get(); s++) {
        root["states"][s] = states[s].toJson();
    }
    return root;
}

void xmm::SingleClassHMM::fromJson(Json::Value const& root) {
    check_training();
    try {
        SingleClassHMM tmp(shared_parameters, root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

#pragma mark -
#pragma mark Exit Probabilities

void xmm::SingleClassHMM::updateExitProbabilities(float* exitProbabilities) {
    unsigned int numStates = parameters.states.get();

    if (!is_hierarchical_)
        throw std::runtime_error(
            "Model is Not hierarchical: method cannot be used");
    if (exitProbabilities == NULL) {
        exit_probabilities_.resize(numStates, 0.0);
        exit_probabilities_[numStates - 1] =
            DEFAULT_EXITPROBABILITY_LAST_STATE();
    } else {
        exit_probabilities_.resize(numStates, 0.0);
        for (int i = 0; i < numStates; i++) try {
                exit_probabilities_[i] = exitProbabilities[i];
            } catch (std::exception& e) {
                throw std::invalid_argument(
                    "Wrong format for exit probabilities");
            }
    }
}

void xmm::SingleClassHMM::addExitPoint(int stateIndex, float proba) {
    if (!is_hierarchical_)
        throw std::runtime_error(
            "Model is Not hierarchical: method cannot be used");
    if (stateIndex >= parameters.states.get())
        throw std::out_of_range("State index out of bounds");
    exit_probabilities_[stateIndex] = proba;
}

//#pragma mark > Conversion & Extraction
// void xmm::SingleClassHMM::makeBimodal(unsigned int dimension_input)
//{
//    check_training();
//    if (bimodal_)
//        throw std::runtime_error("The model is already bimodal");
//    if (dimension_input >= dimension_)
//        throw std::out_of_range("Request input dimension exceeds the current
//        dimension");
//    flags_ = flags_ | BIMODAL;
//    bimodal_ = true;
//    dimension_input_ = dimension_input;
//    for (unsigned int i=0; i<numStates; i++) {
//        states[i].makeBimodal(dimension_input);
//    }
//    results_predicted_output.resize(dimension_ - dimension_input_);
//    results_output_variance.resize(dimension_ - dimension_input_);
//}
//
// void xmm::SingleClassHMM::makeUnimodal()
//{
//    check_training();
//    if (!bimodal_)
//        throw std::runtime_error("The model is already unimodal");
//    flags_ = NONE;
//    bimodal_ = false;
//    dimension_input_ = 0;
//    for (unsigned int i=0; i<numStates; i++) {
//        states[i].makeUnimodal();
//    }
//    results_predicted_output.clear();
//    results_output_variance.clear();
//}
//
// xmm::HMM xmm::SingleClassHMM::extractSubmodel(std::vector<unsigned int>&
// columns) const
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
//    HMM target_model(*this);
//    size_t new_dim = columns.size();
//    target_model.setTrainingCallback(NULL, NULL);
//    target_model.bimodal_ = false;
//    target_model.dimension_ = static_cast<unsigned int>(new_dim);
//    target_model.dimension_input_ = 0;
//    target_model.flags_ = (this->flags_ & HIERARCHICAL);
//    target_model.allocate();
//    for (unsigned int i=0; i<numStates; ++i) {
//        target_model.states[i] = states[i].extractSubmodel(columns);
//    }
//    return target_model;
//}
//
// xmm::HMM xmm::SingleClassHMM::extractSubmodel_input() const
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
// xmm::HMM xmm::SingleClassHMM::extractSubmodel_output() const
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
// xmm::HMM xmm::SingleClassHMM::extract_inverse_model() const
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
//    HMM target_model = extractSubmodel(columns);
//    target_model.makeBimodal(dimension_-dimension_input_);
//    return target_model;
//}
