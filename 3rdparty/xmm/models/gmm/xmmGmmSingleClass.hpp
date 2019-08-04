/*
 * xmmGmmSingleClass.hpp
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

#ifndef xmmGmmSingleClass_hpp
#define xmmGmmSingleClass_hpp

#include "../../core/model/xmmModelResults.hpp"
#include "../../core/model/xmmModelSingleClass.hpp"
#include "xmmGmmParameters.hpp"
#include <memory>

namespace xmm {
const std::vector<float> null_vector_float;

/**
 @ingroup GMM
 @brief Single-Class Gaussian Mixture Model
 @details Multivariate Gaussian Mixture Model. Supports Bimodal data and
 Gaussian Mixture Regression.
 Can be either autonomous or a state of a HMM: defines observation probabilities
 for each state.
 */
class SingleClassGMM : public SingleClassProbabilisticModel {
  public:
    template <typename SingleClassModel, typename ModelType>
    friend class Model;
    friend class SingleClassHMM;
    friend class HierarchicalHMM;

    /**
     @brief Constructor
     @param p pointer to a shared parameters object (owned by a Model)
     */
    SingleClassGMM(std::shared_ptr<SharedParameters> p = NULL);

    /**
     @brief Copy constructor
     @param src Source GMM
     */
    SingleClassGMM(SingleClassGMM const& src);

    /**
     @brief Copy constructor
     @param p pointer to a shared parameters object (owned by a Model)
     @param root Json structure
     */
    explicit SingleClassGMM(std::shared_ptr<SharedParameters> p,
                            Json::Value const& root);

    /**
     @brief Assignment
     @param src Source GMM
     */
    SingleClassGMM& operator=(SingleClassGMM const& src);

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
    Json::Value toJson() const;

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    void fromJson(Json::Value const& root);

    ///@}

    //
    //        /**
    //         @brief Convert to bimodal GMM in place
    //         @param dimension_input dimension of the input modality
    //         @throws runtime_error if the model is already bimodal
    //         @throws out_of_range if the requested input dimension is too
    //         large
    //         */
    //        void makeBimodal(unsigned int dimension_input);
    //
    //        /**
    //         @brief Convert to unimodal GMM in place
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
    //         @return a GMM from the current model considering only the target
    //         columns
    //         */
    //        GMM extractSubmodel(std::vector<unsigned int>& columns) const;
    //
    //        /**
    //         @brief extract the submodel of the input modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal GMM of the input modality from the current
    //         bimodal model
    //         */
    //        GMM extractSubmodel_input() const;
    //
    //        /**
    //         @brief extract the submodel of the output modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal GMM of the output modality from the current
    //         bimodal model
    //         */
    //        GMM extractSubmodel_output() const;
    //
    //        /**
    //         @brief extract the model with reversed input and output
    //         modalities
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a bimodal GMM  that swaps the input and output modalities
    //         */
    //        GMM extract_inverse_model() const;

    /**
     @brief Model Parameters
     */
    ClassParameters<GMM> parameters;

    /**
     @brief Results of the filtering process (recognition & regression)
     */
    ClassResults<GMM> results;

    /**
     @brief Vector of Gaussian Components
     */
    std::vector<GaussianDistribution> components;

    /**
     @brief Mixture Coefficients
     */
    std::vector<float> mixture_coeffs;

    /**
     @brief Beta probabilities: likelihood of each component
     */
    std::vector<double> beta;

  protected:
    /**
     @brief Allocate model parameters
     */
    void allocate();

    /**
     @brief Observation probability
     @param observation observation vector (must be of size 'dimension')
     @param mixtureComponent index of the mixture component. if unspecified or
     negative,
     full mixture observation probability is computed
     @return likelihood of the observation given the model
     @throws out_of_range if the index of the Gaussian Mixture Component is out
     of bounds
     @throws runtime_error if the Covariance Matrix is not invertible
     */
    double obsProb(const float* observation, int mixtureComponent = -1) const;

    /**
     @brief Observation probability on the input modality
     @param observation_input observation vector of the input modality (must be
     of size 'dimension_input')
     @param mixtureComponent index of the mixture component. if unspecified or
     negative,
     full mixture observation probability is computed
     @return likelihood of the observation of the input modality given the model
     @throws runtime_error if the model is not bimodal
     @throws runtime_error if the Covariance Matrix of the input modality is not
     invertible
     */
    double obsProb_input(const float* observation_input,
                         int mixtureComponent = -1) const;

    /**
     @brief Observation probability for bimodal mode
     @param observation_input observation vector of the input modality (must be
     of size 'dimension_input')
     @param observation_output observation vector of the input output (must be
     of size 'dimension - dimension_input')
     @param mixtureComponent index of the mixture component. if unspecified or
     negative,
     full mixture observation probability is computed
     @return likelihood of the observation of the input modality given the model
     @throws runtime_error if the model is not bimodal
     @throws runtime_error if the Covariance Matrix is not invertible
     */
    double obsProb_bimodal(const float* observation_input,
                           const float* observation_output,
                           int mixtureComponent = -1) const;

    /**
     @brief Initialize the EM Training Algorithm
     @details Initializes the Gaussian Components from the first phrase
     of the Training Set
     */
    void emAlgorithmInit(TrainingSet* trainingSet);

    /**
     @brief Update Function of the EM algorithm
     @return likelihood of the data given the current parameters (E-step)
     */
    double emAlgorithmUpdate(TrainingSet* trainingSet);

    /**
     @brief Initialize model parameters to default values.
     @details Mixture coefficients are then equiprobable
     */
    void initParametersToDefault(std::vector<float> const& dataStddev);

    /**
     @brief Initialize the means of the Gaussian components with a Biased
     K-means
     */
    void initMeansWithKMeans(TrainingSet* trainingSet);

    /**
     @brief Initialize the Covariances of the Gaussian components using a fully
     observed approximation
     */
    void initCovariances_fullyObserved(TrainingSet* trainingSet);

    /**
     @brief Normalize mixture coefficients
     */
    void normalizeMixtureCoeffs();

    /**
     @brief Add offset to the diagonal of the covariance matrices
     @details Guarantees convergence through covariance matrix invertibility
     */
    void addCovarianceOffset();

    /**
     @brief Update inverse covariances of each Gaussian component
     @throws runtime_error if one of the covariance matrices is not invertible
     */
    void updateInverseCovariances();

    /**
     @brief Compute likelihood and estimate components probabilities
     @details If the model is bimodal, the likelihood is computed only on the
     input modality,
     except if 'observation_output' is specified.
     Updates the likelihood buffer used to smooth likelihoods.
     @param observation observation vector (full size for unimodal, input
     modality for bimodal)
     @param observation_output observation vector of the output modality
     */
    double likelihood(
        std::vector<float> const& observation,
        std::vector<float> const& observation_output = null_vector_float);

    /**
     @brief Compute Gaussian Mixture Regression
     @details Estimates the output modality using covariance-based regression
     weighted by components' likelihood
     @warning the function does not estimates the likelihoods, use 'likelihood'
     before performing
     the regression.
     @param observation_input observation vector of the input modality
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
     @brief vector containing the regularization values over each dimension
     @details computed by combining relative and absolute regularization with
     the
     training set
     */
    std::vector<double> current_regularization;
};
}

#endif
