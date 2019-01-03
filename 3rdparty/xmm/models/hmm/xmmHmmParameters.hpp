/*
 * xmmHmmParameters.hpp
 *
 * Parameters of Hidden Markov Models
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

#ifndef xmmHmmParameters_hpp
#define xmmHmmParameters_hpp

#include "../../core/distributions/xmmGaussianDistribution.hpp"
#include "../../core/model/xmmModelParameters.hpp"

namespace xmm {
/**
 @defgroup HMM [Models] Hidden Markov Models
 */
class HMM {
  public:
    /**
     @brief Type of Transition Matrix
     */
    enum class TransitionMode {
        /**
         @brief Ergodic Transition Matrix
         */
        Ergodic = 0,

        /**
         @brief Left-Right Transition model
         @details The only authorized transitions are: auto-transition and
         transition to the next state
         */
        LeftRight = 1
    };

    /**
     @enum RegressionEstimator
     @brief Estimator for the regression with HMMs
     */
    enum class RegressionEstimator {
        /**
         @brief The output is estimated by a weighted regression over all states
         */
        Full = 0,

        /**
         @brief The output is estimated by a weighted regression over a window
         centered around
         the likeliest state
         */
        Windowed = 1,

        /**
         @brief The output is estimated by a regression using the likeliest
         state only.
         */
        Likeliest = 2
    };
};

/**
 @ingroup HMM
 @brief Parameters specific to each class of a Hidden Markov Model
 */
template <>
class ClassParameters<HMM> {
  public:
    /**
     @brief Default Constructor
     */
    ClassParameters();

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    ClassParameters(ClassParameters const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit ClassParameters(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Object
     */
    ClassParameters& operator=(ClassParameters const& src);

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
    virtual void fromJson(Json::Value const& root);

    ///@}

    /**
     @brief specifies if parameters have changed (model is invalid)
     */
    bool changed = false;

    /**
     @brief Number of hidden states
     */
    Attribute<unsigned int> states;

    /**
     @brief Number of Gaussian Mixture Components
     */
    Attribute<unsigned int> gaussians;

    /**
     @brief Offset Added to the diagonal of covariance matrices for convergence
     (Relative to Data Variance)
     */
    Attribute<double> relative_regularization;

    /**
     @brief Offset Added to the diagonal of covariance matrices for convergence
     (minimum value)
     */
    Attribute<double> absolute_regularization;

    /**
     @brief Covariance Mode
     */
    Attribute<GaussianDistribution::CovarianceMode> covariance_mode;

    /**
     @brief Transition matrix of the model (left-right vs ergodic)
     */
    Attribute<HMM::TransitionMode> transition_mode;

    /**
     @brief Type of regression estimator
     */
    Attribute<HMM::RegressionEstimator> regression_estimator;

    /**
     @brief specifies if the decoding algorithm is hierarchical or
     class-conditional
     */
    Attribute<bool> hierarchical;

  protected:
    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);
};

template <>
void checkLimits<HMM::TransitionMode>(HMM::TransitionMode const& value,
                                      HMM::TransitionMode const& limit_min,
                                      HMM::TransitionMode const& limit_max);

template <>
HMM::TransitionMode Attribute<HMM::TransitionMode>::defaultLimitMax();

template <>
void checkLimits<HMM::RegressionEstimator>(
    HMM::RegressionEstimator const& value,
    HMM::RegressionEstimator const& limit_min,
    HMM::RegressionEstimator const& limit_max);

template <>
HMM::RegressionEstimator Attribute<HMM::RegressionEstimator>::defaultLimitMax();
}

#endif
