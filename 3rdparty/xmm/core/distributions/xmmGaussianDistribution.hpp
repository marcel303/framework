/*
 * xmmGaussianDistribution.hpp
 *
 * Multivariate Gaussian Distribution
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

#ifndef xmmGaussianDistribution_h
#define xmmGaussianDistribution_h

#include "../common/xmmAttribute.hpp"
#include "../common/xmmJson.hpp"

namespace xmm {
/**
 @defgroup Distributions [Core] Distributions
 */

/**
 @ingroup Distributions
 @brief Structure for storing Ellipse parameters
 */
struct Ellipse {
    /**
     @brief x center position
     */
    float x;

    /**
     @brief y center position
     */
    float y;

    /**
     @brief width: minor axis length
     */
    float width;

    /**
     @brief height: major axis length
     */
    float height;

    /**
     @brief angle (radians)
     */
    float angle;
};

/**
 @ingroup Distributions
 @brief Multivariate Gaussian Distribution
 @details Full covariance, optionally multimodal with support for regression
 */
class GaussianDistribution : public Writable {
  public:
    /**
     @brief Covariance Mode
     */
    enum class CovarianceMode {
        /**
         @brief Full covariance
         */
        Full = 0,

        /**
         @brief Diagonal covariance (diagonal matrix)
         */
        Diagonal = 1
    };

    /**
     @brief Default Constructor
     @param bimodal specify if the distribution is bimodal for use in regression
     @param dimension dimension of the distribution
     @param dimension_input dimension of the input modality in bimodal mode.
     @param covariance_mode covariance mode (full vs diagonal)
     */
    GaussianDistribution(bool bimodal = false, unsigned int dimension = 1,
                         unsigned int dimension_input = 0,
                         CovarianceMode covariance_mode = CovarianceMode::Full);

    /**
     @brief Copy constructor
     @param src source distribution
     */
    GaussianDistribution(GaussianDistribution const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit GaussianDistribution(Json::Value const& root);

    /**
     @brief Assignment
     @param src source distribution
     */
    GaussianDistribution& operator=(GaussianDistribution const& src);

    /** @name Likelihood & Regression */
    ///@{

    /**
     @brief Get Likelihood of a data vector
     @param observation data observation (must be of size @a dimension)
     @return likelihood
     @throws runtime_error if the Covariance Matrix is not invertible
     */
    double likelihood(const float* observation) const;

    /**
     @brief Get Likelihood of a data vector for input modality
     @param observation_input observation (must be of size @a dimension_input)
     @return likelihood
     @throws runtime_error if the Covariance Matrix of the input modality is not
     invertible
     @throws runtime_error if the model is not bimodal
     */
    double likelihood_input(const float* observation_input) const;

    /**
     @brief Get Likelihood of a data vector for bimodal mode
     @param observation_input observation of the input modality
     @param observation_output observation of the output modality
     @throws runtime_error if the Covariance Matrix is not invertible
     @throws runtime_error if the model is not bimodal
     @return likelihood
     */
    double likelihood_bimodal(const float* observation_input,
                              const float* observation_output) const;

    /**
     @brief Linear Regression using the Gaussian Distribution (covariance-based)
     @param observation_input input observation (must be of size: @a
     dimension_input)
     @param predicted_output predicted output vector (size:
     dimension-dimension_input)
     @throws runtime_error if the model is not bimodal
     */
    void regression(std::vector<float> const& observation_input,
                    std::vector<float>& predicted_output) const;

    ///@}

    /** @name Utilities */
    ///@{

    /**
     @brief Add @a offset to the diagonal of the covariance matrix
     @details Ensures convergence + generalization on few examples
     */
    void regularize(std::vector<double> regularization);

    /**
     @brief Compute inverse covariance matrix
     @throws runtime_error if the covariance matrix is not invertible
     */
    void updateInverseCovariance();

    /**
     @brief Compute the 68%?? Confidence Interval ellipse of the Gaussian
     @details the ellipse is 2D, and is therefore projected over 2 axes
     @param dimension1 index of the first axis
     @param dimension2 index of the second axis
     @throws out_of_range if the dimensions are out of bounds
     @return ellipse parameters
     */
    Ellipse toEllipse(unsigned int dimension1, unsigned int dimension2);

    /**
     @brief Sets the parameters of the Gaussian distribution according to the
     68%?? Confidence Interval ellipse
     @details the ellipse is 2D, and is therefore projected over 2 axes
     @param gaussian_ellipse 68% Confidence Interval ellipse parameters (1x std)
     @param dimension1 index of the first axis
     @param dimension2 index of the second axis
     @throws out_of_range if the dimensions are out of bounds
     */
    void fromEllipse(Ellipse const& gaussian_ellipse, unsigned int dimension1,
                     unsigned int dimension2);

    ///@}

    /** @name JSON I/O */
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

    //        /** @name Conversion & Extraction */
    //        ///@{
    //
    //        /**
    //         @brief Convert to bimodal distribution in place
    //         @param dimension_input dimension of the input modality
    //         @throws runtime_error if the model is already bimodal
    //         @throws out_of_range if the requested input dimension is too
    //         large
    //         */
    //        void makeBimodal(unsigned int dimension_input);
    //
    //        /**
    //         @brief Convert to unimodal distribution in place
    //         @throws runtime_error if the model is already unimodal
    //         */
    //        void makeUnimodal();
    //
    //        /**
    //         @brief extract a sub-distribution with the given columns
    //         @param columns columns indices in the target order
    //         @throws runtime_error if the model is training
    //         @throws out_of_range if the number or indices of the requested
    //         columns exceeds the current dimension
    //         @return a Gaussian Distribution from the current model
    //         considering only the target columns
    //         */
    //        GaussianDistribution extractSubmodel(std::vector<unsigned int>&
    //        columns) const;
    //
    //        /**
    //         @brief extract the sub-distribution of the input modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal Gaussian Distribution of the input modality
    //         from the current bimodal model
    //         */
    //        GaussianDistribution extractSubmodel_input() const;
    //
    //        /**
    //         @brief extract the sub-distribution of the output modality
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a unimodal Gaussian Distribution of the output modality
    //         from the current bimodal model
    //         */
    //        GaussianDistribution extractSubmodel_output() const;
    //
    //        /**
    //         @brief extract the model with reversed input and output
    //         modalities
    //         @throws runtime_error if the model is training or if it is not
    //         bimodal
    //         @return a bimodal Gaussian Distribution  that swaps the input and
    //         output modalities
    //         */
    //        GaussianDistribution extract_inverse_model() const;
    //
    //        ///@}

    /**
     @brief Total Dimension of the multivariate normal
     */
    Attribute<unsigned int> dimension;

    /**
     @brief Input Dimension of the multivariate normal
     */
    Attribute<unsigned int> dimension_input;

    /**
     @brief Mean of the Gaussian Distribution
     */
    std::vector<double> mean;

    /**
     @brief Covariance Mode
     */
    Attribute<CovarianceMode> covariance_mode;

    /**
     @brief Covariance Matrix of the Gaussian Distribution
     */
    std::vector<double> covariance;

    /**
     @brief Conditional Output Variance (updated when covariances matrices are
     inverted)
     */
    std::vector<double> output_covariance;

  protected:
    /**
     @brief Resize Mean and Covariance Vectors to appropriate dimension.
     */
    void allocate();

    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);

    /**
     @brief Compute the conditional variance vector of the output modality
     (conditioned over the input).
     @throws runtime_error if the model is not bimodal
     */
    void updateOutputCovariance();

    /**
     @brief Defines if regression parameters need to be computed
     */
    bool bimodal_;

    /**
     @brief Determinant of the covariance matrix
     */
    double covariance_determinant_;

    /**
     @brief Inverse covariance matrix
     */
    std::vector<double> inverse_covariance_;

    /**
     @brief Determinant of the covariance matrix of the input modality
     */
    double covariance_determinant_input_;

    /**
     @brief Inverse covariance matrix of the input modality
     */
    std::vector<double> inverse_covariance_input_;
};

Ellipse covariance2ellipse(double c_xx, double c_xy, double c_yy);

template <>
void checkLimits<GaussianDistribution::CovarianceMode>(
    GaussianDistribution::CovarianceMode const& value,
    GaussianDistribution::CovarianceMode const& limit_min,
    GaussianDistribution::CovarianceMode const& limit_max);

template <>
GaussianDistribution::CovarianceMode
Attribute<GaussianDistribution::CovarianceMode>::defaultLimitMax();
}

#endif
