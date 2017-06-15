/*
 * xmmGmm.hpp
 *
 * Gaussian Mixture Model for Continuous Recognition and Regression
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

#ifndef xmmGmm_h
#define xmmGmm_h

#include "../../core/distributions/xmmGaussianDistribution.hpp"
#include "../../core/model/xmmModel.hpp"
#include "xmmGmmSingleClass.hpp"

namespace xmm {
/**
 @ingroup GMM
 @brief Gaussian Mixture Model for Continuous Recognition and Regression
 (Multi-class)
 */
class GMM : public Model<SingleClassGMM, GMM> {
  public:
    /**
     @brief Constructor
     @param bimodal specifies if the model should be used for regression
     */
    GMM(bool bimodal = false);

    /**
     @brief Copy Constructor
     @param src Source Model
     */
    GMM(GMM const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit GMM(Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Model
     */
    GMM& operator=(GMM const& src);

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
     @brief Results of the Filtering Process (Recognition + Regression)
     */
    Results<GMM> results;

  protected:
    /**
     @brief Update the results (Likelihoods)
     */
    virtual void updateResults();
};
}

#endif
