/*
 * xmmHmmSingleClass.hpp
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

#ifndef xmmHmmSingleClass_hpp
#define xmmHmmSingleClass_hpp

#include "../gmm/xmmGmmSingleClass.hpp"
#include "xmmHmmParameters.hpp"
#include "xmmHmmResults.hpp"

namespace xmm {
/** @ingroup HMM
 @brief Single-Class Hidden Markov Model
 @details Hidden Markov Model with Multivariate Gaussian Mixture Model
 Observation Distributions.
 Supports Bimodal data and Hidden Markov Regression. Built for Hierarchical HMMs
 */
class SingleClassHMM : public SingleClassProbabilisticModel {
    template <typename SingleClassModel, typename ModelType>
    friend class Model;
    friend class HierarchicalHMM;

  public:
    static const float DEFAULT_EXITPROBABILITY_LAST_STATE() { return 0.1; }
    static const float TRANSITION_REGULARIZATION() { return 1.0e-5; }

    /**
     @brief Constructor
     @param p Shared Parameters (owned by a multiclass object)
     */
    SingleClassHMM(std::shared_ptr<SharedParameters> p = NULL);

    /**
     @brief Copy constructor
     @param src Source Model
     */
    SingleClassHMM(SingleClassHMM const& src);

    /**
     @brief Copy constructor
     @param p pointer to a shared parameters object (owned by a Model)
     @param root Json structure
     */
    explicit SingleClassHMM(std::shared_ptr<SharedParameters> p,
                            Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Model
     */
    SingleClassHMM& operator=(SingleClassHMM const& src);

    /** @name Accessors */
    ///@{

    /**
     @brief Set the exit probability of a specific state
     @details this method is only active in 'HIERARCHICAL' mode. The probability
     @param stateIndex index of the state to add the exit point
     @param proba probability to exit the gesture from this state
     @throws runtime_error if the model is not hierarchical
     @throws out_of_range if the state index is out of bounds
     */
    void addExitPoint(int stateIndex, float proba);

    ///@}

    /** @name Performance */
    ///@{

    /**
     @brief Resets the fitering process (recognition or regression)
     */
    void reset();

    /**
     @brief filters a incoming observation (performs recognition or regression)
     @details the results of the inference process are stored in the results
     attribute
     @param observation observation vector
     @return likelihood of the observation
     */
    double filter(std::vector<float> const& observation);

    ///@}

    /** @name Json I/O */
    ///@{

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    virtual Json::Value toJson() const;

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    virtual void fromJson(Json::Value const& root);

    ///@}

    //        /**
    //         @brief Convert to bimodal HMM in place
    //         @param dimension_input dimension of the input modality
    //         @throws runtime_error if the model is already bimodal
    //         @throws out_of_range if the requested input dimension is too
    //         large
    //         */
    //        void makeBimodal(unsigned int dimension_input);
    //
    //        /**
    //         @brief Convert to unimodal HMM in place
    //         @throws runtime_error if the model is already unimodal
    //         */
    //        void makeUnimodal();
    //
    //        /**
    //         @brief extract a submodel with the given columns
    //         @param columns columns indices in the target order
    //         @throws runtime_error if the model is training
    //         @throws out_of_range if the number or indices of the requested
    //         columns exceeds the current dimension
    //         @return a HMM from the current model considering only the target
    //         columns
    //         */
    //        HMM extractSubmodel(std::vector<unsigned int>& columns) const;
    //
    //        /**
    //         @brief extract the submodel of the input modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal HMM of the input modality from the current
    //         bimodal model
    //         */
    //        HMM extractSubmodel_input() const;
    //
    //        /**
    //         @brief extract the submodel of the output modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal HMM of the output modality from the current
    //         bimodal model
    //         */
    //        HMM extractSubmodel_output() const;
    //
    //        /**
    //         @brief extract the model with reversed input and output
    //         modalities
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a bimodal HMM  that swaps the input and output modalities
    //         */
    //        HMM extract_inverse_model() const;

    /**
     @brief Model Parameters
     */
    ClassParameters<HMM> parameters;

    /**
     @brief Results of the filtering process (recognition & regression)
     */
    ClassResults<HMM> results;

    /**
     @brief State probabilities estimated by the forward algorithm.
     */
    std::vector<double> alpha;

    /**
     @brief State probabilities estimated by the hierarchical forward algorithm.
     @details the variable is only allocated/used in hierarchical mode (see
     'HIERARCHICAL' flag)
     */
    std::vector<double> alpha_h[3];

    /**
     @brief States of the model (Gaussian Mixture Models)
     */
    std::vector<SingleClassGMM> states;

    /**
     @brief Prior probabilities
     */
    std::vector<float> prior;

    /**
     @brief Transition Matrix
     */
    std::vector<float> transition;

  protected:
    /**
     @brief Allocate model parameters
     */
    void allocate();

    /**
     @brief initialize model parameters to their default values
     */
    void initParametersToDefault(std::vector<float> const& dataStddev);

    /**
     @brief initialize the means of each state with all training phrases (single
     gaussian)
     */
    void initMeansWithAllPhrases(TrainingSet* trainingSet);

    /**
     @brief initialize the covariances of each state with all training phrases
     (single gaussian)
     */
    void initCovariances_fullyObserved(TrainingSet* trainingSet);

    /**
     @brief initialize the means and covariances of each state using GMM-EM on
     segments.
     */
    void initMeansCovariancesWithGMMEM(TrainingSet* trainingSet);

    /**
     @brief set the prior and transition matrix to ergodic
     */
    void setErgodic();

    /**
     @brief set the prior and transition matrix to left-right (no state skip)
     */
    void setLeftRight();

    /**
     @brief Normalize transition probabilities
     */
    void normalizeTransitions();

    /**
     @brief Initialization of the forward algorithm
     @param observation observation vector at time t. If the model is bimodal,
     this vector
     should be only the observation on the input modality.
     @param observation_output observation on the output modality (only used if
     the model is bimodal).
     If unspecified, the update is performed on the input modality only.
     @return instantaneous likelihood
     */
    double forward_init(const float* observation,
                        const float* observation_output = NULL);

    /**
     @brief Update of the forward algorithm
     @param observation observation vector at time t. If the model is bimodal,
     this vector
     should be only the observation on the input modality.
     @param observation_output observation on the output modality (only used if
     the model is bimodal).
     If unspecified, the update is performed on the input modality only.
     @return instantaneous likelihood
     */
    double forward_update(const float* observation,
                          const float* observation_output = NULL);

    /**
     @brief Initialization Backward algorithm
     @param ct inverse of the likelihood at time step t computed
     with the forward algorithm (see Rabiner 1989)
     */
    void backward_init(double ct);

    /**
     @brief Update of the Backward algorithm
     @param ct inverse of the likelihood at time step t computed
     with the forward algorithm (see Rabiner 1989)
     @param observation observation vector at time t. If the model is bimodal,
     this vector
     should be only the observation on the input modality.
     @param observation_output observation on the output modality (only used if
     the model is bimodal).
     If unspecified, the update is performed on the input modality only.
     */
    void backward_update(double ct, const float* observation,
                         const float* observation_output = NULL);

    /**
     @brief Initialization of the parameters before training
     */
    void emAlgorithmInit(TrainingSet* trainingSet);

    /**
     @brief Termination of the training algorithm
     */
    void emAlgorithmTerminate();

    /**
     @brief update method of the EM algorithm (calls Baum-Welch Algorithm)
     */
    virtual double emAlgorithmUpdate(TrainingSet* trainingSet);

    /**
     @brief Compute the forward-backward algorithm on a phrase of the training
     set
     @param currentPhrase pointer to the phrase of the training set
     @param phraseIndex index of the phrase
     @return lieklihood of the phrase given the model's current parameters
     */
    double baumWelch_forwardBackward(std::shared_ptr<Phrase> currentPhrase,
                                     int phraseIndex);

    /**
     @brief Update of the forward algorithm for Training (observation
     probabilities are pre-computed)
     @param observation_likelihoods likelihoods of the observations for each
     state
     @return instantaneous likelihood
     */
    double baumWelch_forward_update(
        std::vector<double>::iterator observation_likelihoods);

    /**
     @brief Update of the Backward algorithm for Training (observation
     probabilities are pre-computed)
     @param ct inverse of the likelihood at time step t computed
     with the forward algorithm (see Rabiner 1989)
     @param observation_likelihoods likelihoods of the observations for each
     state
     */
    void baumWelch_backward_update(
        double ct, std::vector<double>::iterator observation_likelihoods);

    /**
     @brief Compute the sum of the gamma variable (for use in EM)
     */
    void baumWelch_gammaSum(TrainingSet* trainingSet);

    /**
     @brief Estimate the Coefficients of the Gaussian Mixture for each state
     */
    void baumWelch_estimateMixtureCoefficients(TrainingSet* trainingSet);

    /**
     @brief Estimate the Means of the Gaussian Distribution for each state
     */
    void baumWelch_estimateMeans(TrainingSet* trainingSet);

    /**
     @brief Estimate the Covariances of the Gaussian Distribution for each state
     */
    void baumWelch_estimateCovariances(TrainingSet* trainingSet);

    /**
     @brief Estimate the Prior Probabilities
     */
    void baumWelch_estimatePrior(TrainingSet* trainingSet);

    /**
     @brief Estimate the Transition Probabilities
     */
    void baumWelch_estimateTransitions(TrainingSet* trainingSet);

    /**
     @brief Adds a cyclic Transition probability (from last state to first
     state)
     @details avoids getting stuck at the end of the model. this method is idle
     for a hierarchical model.
     @param proba probability of the transition form last to first state
     */
    void addCyclicTransition(double proba);

    /**
     @brief Estimates the likeliest state and compute the bounds of the windows
     over the states.
     @details The window is centered around the likeliest state, and its size is
     the number of states.
     The window is clipped to the first and last states.
     */
    void updateAlphaWindow();

    /**
     @brief Compute the regression for the case of a bimodal model, given the
     estimated state probabilities estimated by forward algorithm.
     @details predicted output parameters are stored in the result structure.
     @param observation_input observation on the input modality
     */
    void regression(std::vector<float> const& observation_input);

    /**
     @brief update the content of the likelihood buffer and return average
     likelihood.
     @details The method also updates the cumulative log-likelihood computed
     over a window (cumulativeloglikelihood)
     */
    void updateResults();

    /**
     @brief Update the exit probability vector given the probabilities
     @details this method is only active in 'HIERARCHICAL' mode. The probability
     vector defines the probability of exiting the gesture from each state. If
     unspecified,
     only the last state of the gesture has a non-zero probability.
     @param _exitProbabilities vector of exit probabilities (size must be
     nbStates)
     @throws runtime_error if the model is not hierarchical
     */
    void updateExitProbabilities(float* _exitProbabilities = NULL);

  protected:
    /**
     @brief Defines if the forward algorithm has been initialized
     */
    bool forward_initialized_;

    /**
     @brief used to store the alpha estimated at the previous time step
     */
    std::vector<double> previous_alpha_;

    /**
     @brief backward state probabilities
     */
    std::vector<double> beta_;

    /**
     @brief used to store the beta estimated at the previous time step
     */
    std::vector<double> previous_beta_;

    /**
     @brief Sequence of Gamma probabilities
     */
    std::vector<std::vector<double> > gamma_sequence_;

    /**
     @brief Sequence of Epsilon probabilities
     */
    std::vector<std::vector<double> > epsilon_sequence_;

    /**
     @brief Sequence of Gamma probabilities for each mixture component
     */
    std::vector<std::vector<std::vector<double> > > gamma_sequence_per_mixture_;

    /**
     @brief Sequence of alpha (forward) probabilities
     */
    std::vector<double> alpha_seq_;

    /**
     @brief Sequence of beta (backward) probabilities
     */
    std::vector<double> beta_seq_;

    /**
     @brief Used to store the sums of the gamma variable
     */
    std::vector<double> gamma_sum_;

    /**
     @brief Used to store the sums of the gamma variable for each mixture
     component
     */
    std::vector<double> gamma_sum_per_mixture_;

    /**
     @brief Defines if the model is a submodel of a hierarchical HMM.
     @details in practice this adds exit probabilities to each state. These
     probabilities are set
     to the last state by default.
     */
    bool is_hierarchical_;

    /**
     @brief Exit probabilities for a hierarchical model.
     */
    std::vector<float> exit_probabilities_;

    /**
     @brief minimum index of the alpha window (used for regression & time
     progression)
     */
    int window_minindex_;

    /**
     @brief minimum index of the alpha window (used for regression & time
     progression)
     */
    int window_maxindex_;

    /**
     @brief normalization constant of the alpha window (used for regression &
     time progression)
     */
    double window_normalization_constant_;
};
}

#endif
