/*
 * hierarchical_hmm.h
 *
 * Hierarchical Hidden Markov Model for Continuous Recognition and Regression
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

#ifndef xmm_lib_hierarchical_hmm_h
#define xmm_lib_hierarchical_hmm_h

#include "../../core/model/xmmModel.hpp"
#include "../gmm/xmmGmm.hpp"
#include "xmmHmmSingleClass.hpp"

namespace xmm {
/**
 @ingroup HMM
 @brief Hierarchical Hidden Markov Model for Continuous Recognition and
 Regression (Multi-class)
 */
class HierarchicalHMM : public Model<SingleClassHMM, HMM> {
  public:
    /**
     @brief Default exit transition for the highest level
     */
    static const double DEFAULT_EXITTRANSITION() { return 0.1; }

    /**
     @brief Constructor
     @param bimodal specifies if the model should be used for regression
     */
    HierarchicalHMM(bool bimodal = false);

    /**
     @brief Copy Constructor
     @param src Source Model
     */
    HierarchicalHMM(HierarchicalHMM const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit HierarchicalHMM(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Model
     */
    HierarchicalHMM& operator=(HierarchicalHMM const& src);

    /** @name Class Manipulation */
    ///@{

    /**
     @brief Remove a specific class by label
     @param label label of the class to remove
     */
    virtual void removeClass(std::string const& label);

    /**
     @brief Remove All models
     */
    virtual void clear();

    ///@}

    /** @name Accessors */
    ///@{

    void addExitPoint(int state, float proba);

    ///@}

    /** @name Performance */
    ///@{

    /**
     @brief Resets the fitering process (recognition or regression)
     */
    virtual void reset();

    /**
     @brief filters a incoming observation (performs recognition or regression)
     @details the results of the inference process are stored in the results
     attribute
     @param observation observation vector
     */
    virtual void filter(std::vector<float> const& observation);

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
    //         @brief Convert to bimodal HierarchicalHMM in place
    //         @param dimension_input dimension of the input modality
    //         @throws runtime_error if the model is already bimodal
    //         @throws out_of_range if the requested input dimension is too
    //         large
    //         */
    //        void makeBimodal(unsigned int dimension_input);
    //
    //        /**
    //         @brief Convert to unimodal HierarchicalHMM in place
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
    //         @return a HierarchicalHMM from the current model considering only
    //         the target columns
    //         */
    //        HierarchicalHMM extractSubmodel(std::vector<unsigned int>&
    //        columns)
    //        const;
    //
    //        /**
    //         @brief extract the submodel of the input modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal GMM of the input modality from the current
    //         bimodal model
    //         */
    //        HierarchicalHMM extractSubmodel_input() const;
    //
    //        /**
    //         @brief extract the submodel of the output modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal HierarchicalHMM of the output modality from
    //         the current bimodal model
    //         */
    //        HierarchicalHMM extractSubmodel_output() const;
    //
    //        /**
    //         @brief extract the model with reversed input and output
    //         modalities
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a bimodal HierarchicalHMM  that swaps the input and
    //         output modalities
    //         */
    //        HierarchicalHMM extract_inverse_model() const;

    /**
     @brief Results of the Filtering Process (Recognition + Regression)
     */
    Results<HMM> results;

    /**
     @brief Prior probabilities of the models
     */
    std::vector<double> prior;

    /**
     @brief exit probabilities of the model (probability to finish and go back
     to the root)
     */
    std::vector<double> exit_transition;

    /**
     @brief Transition probabilities between models
     */
    std::vector<std::vector<double>> transition;

  protected:
    /**
     @brief Finishes the background training process by joining threads and
     deleting the models which training failed
     @details: updates transition parameters after joining
     */
    virtual void joinTraining();

    /**
     @brief update high-level parameters when a new primitive is learned
     @details  updated parameters: prior probabilities + transition matrix
     */
    void updateTransitionParameters();

    /**
     @brief ergodic learning update high-level prior probabilities -> equal
     prior probs
     */
    void updatePrior();

    /**
     @brief ergodic learning: update high-level transition matrix
     @details equal transition probabilities between primitive gestures
     */
    void updateTransition();

    /**
     @brief Update exit probabilities of each sub-model
     */
    void updateExitProbabilities();

    virtual void addModelForClass(std::string const& label);

    /**
     @brief Normalize segment level prior and transition matrices
     */
    void normalizeTransitions();

    /**
     @brief Initialization of the Forward Algorithm for the hierarchical HMM.
     see: Jules Francoise. Realtime Segmentation and Recognition of Gestures
     using Hierarchical Markov Models. Master’s Thesis, Université Pierre et
     Marie Curie, Ircam, 2011.
     [http://articles.ircam.fr/textes/Francoise11a/index.pdf]
     @param observation observation vector. If the model is bimodal, this should
     be allocated for
     both modalities, and should contain the observation on the input modality.
     The predicted
     output will be appended to the input modality observation
     */
    void forward_init(std::vector<float> const& observation);

    /**
     @brief Update of the Forward Algorithm for the hierarchical HMM.
     see: Jules Francoise. Realtime Segmentation and Recognition of Gestures
     using Hierarchical Markov Models. Master’s Thesis, Université Pierre et
     Marie Curie, Ircam, 2011.
     [http://articles.ircam.fr/textes/Francoise11a/index.pdf]
     @param observation observation vector. If the model is bimodal, this should
     be allocated for
     both modalities, and should contain the observation on the input modality.
     The predicted
     output will be appended to the input modality observation
     */
    void forward_update(std::vector<float> const& observation);

    /**
     @brief get instantaneous likelihood
     *
     get instantaneous likelihood on alpha variable for exit state exitNum.
     @param exitNum number of exit state (0=continue, 1=transition, 2=back to
     root). if -1, get likelihood over all exit states
     @param likelihoodVector likelihood vector (size nbPrimitives)
     */
    void likelihoodAlpha(int exitNum,
                         std::vector<double>& likelihoodVector) const;

    /**
     @brief Update the results (Likelihoods)
     */
    void updateResults();

    /**
     @brief Defines if the forward algorithm has been initialized
     */
    bool forward_initialized_;

    /**
     @brief intermediate Forward variable (used in Frontier algorithm)
     */
    std::vector<double> frontier_v1_;

    /**
     @brief intermediate Forward variable (used in Frontier algorithm)
     */
    std::vector<double> frontier_v2_;
};
}

#endif