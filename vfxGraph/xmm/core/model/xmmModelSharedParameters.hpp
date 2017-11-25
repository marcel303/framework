/*
 * xmmModelSharedParameters.hpp
 *
 * Shared Parameters class
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

#ifndef xmmModelSharedParameters_h
#define xmmModelSharedParameters_h

#include "../common/xmmEvents.hpp"
#include "../trainingset/xmmTrainingSet.hpp"
#include <mutex>

namespace xmm {
/**
 @defgroup Model [Core] Probabilistic Models
 */

/**
 @ingroup Model
 @brief Shared Parameters for models with multiple classes.
 @details This structure is shared by pointer between the class-specific models
 to avoid
 data duplication and ensures that all class-specific models share the same
 common attributes.
 */
class SharedParameters : public Writable {
  public:
    template <typename SingleClassModel, typename ModelType>
    friend class Model;
    friend class SingleClassProbabilisticModel;
    friend class SingleClassGMM;
    friend class SingleClassHMM;
    friend class HierarchicalHMM;
    friend class GMM;

    /**
     @brief Default Constructor
     */
    SharedParameters();

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    SharedParameters(SharedParameters const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit SharedParameters(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Object
     */
    SharedParameters& operator=(SharedParameters const& src);

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

    /**
     @brief defines if the phrase is bimodal (true) or unimodal (false)
     */
    Attribute<bool> bimodal;

    /**
     @brief total dimension of the training data
     */
    Attribute<unsigned int> dimension;

    /**
     @brief Dimension of the input modality
     */
    Attribute<unsigned int> dimension_input;

    /**
     @brief labels of the columns of input/output data (e.g. descriptor names)
     */
    Attribute<std::vector<std::string>> column_names;

    /**
     @brief Minimum number of iterations of the EM algorithm
     */
    Attribute<unsigned int> em_algorithm_min_iterations;

    /**
     @brief Maximum number of iterations of the EM algorithm.
     @details If this value is superior to
     minSteps, this criterion is used. Otherwise, only the
     em_algorithm_percent_chg criterion applies.
     */
    Attribute<unsigned int> em_algorithm_max_iterations;

    /**
     @brief log-likelihood difference threshold necessary to stop the EM
     algorithm.
     @details When the percent-change in likelihood of the training data given
     the
     estimated parameters gets under this threshold, the EM algorithm is
     stopped.
     */
    Attribute<double> em_algorithm_percent_chg;

    /**
     @brief Size of the window (in samples) used to compute the likelihoods
     */
    Attribute<unsigned int> likelihood_window;

  protected:
    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);
};
}

#endif
