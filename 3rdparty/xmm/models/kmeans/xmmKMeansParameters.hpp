/*
 * xmmKMeansParameters.hpp
 *
 * Parameters of the K-Means clustering Algorithm
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

#ifndef xmmKMeansParameters_hpp
#define xmmKMeansParameters_hpp

#include "../../core/model/xmmModelParameters.hpp"

namespace xmm {
/**
 @defgroup KMeans [Models] K-Means Algorithm
 */

/**
 Dummy structure for template specialization
 */
class KMeans;

/**
 @ingroup KMeans
 @brief Parameters specific to each class of a K-Means Algorithm
 */
template <>
class ClassParameters<KMeans> : public Writable {
  public:
    /**
     @brief Default Constructor
     */
    ClassParameters();

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    ClassParameters(ClassParameters<KMeans> const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit ClassParameters(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Object
     */
    ClassParameters& operator=(ClassParameters<KMeans> const& src);

    virtual ~ClassParameters() {}

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
    Attribute<unsigned int> clusters;

    /**
     @brief Maximum number of iterations of the training update
     */
    Attribute<unsigned int> max_iterations;

    /**
     @brief threshold (as relative distance between cluster) required to define
     convergence
     */
    Attribute<float> relative_distance_threshold;

  protected:
    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);
};
}

#endif
