/*
 * xmmGmmParameters.hpp
 *
 * Parameters of Gaussian Mixture Models
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

#ifndef xmmGMMParameters_hpp
#define xmmGMMParameters_hpp

#include "../../core/distributions/xmmGaussianDistribution.hpp"
#include "../../core/model/xmmModelParameters.hpp"

namespace xmm {
/**
 @defgroup GMM [Models] Gaussian Mixture Models
 */

/**
 Dummy structure for template specialization
 */
class GMM;

/**
 @ingroup GMM
 @brief Parameters specific to each class of a Gaussian Mixture Model
 */
template <>
class ClassParameters<GMM> : public Writable {
  public:
    /**
     @brief Default Constructor
     */
    ClassParameters();

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    ClassParameters(ClassParameters<GMM> const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit ClassParameters(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Object
     */
    ClassParameters& operator=(ClassParameters<GMM> const& src);

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

  protected:
    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);
};
}

#endif
