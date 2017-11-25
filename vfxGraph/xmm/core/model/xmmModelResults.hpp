/*
 * xmmModelResults.hpp
 *
 * Results structures for probabilistic models
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

#ifndef xmmModelResults_h
#define xmmModelResults_h

#include <string>
#include <vector>

namespace xmm {
/**
 @ingroup Model
 @brief Class-specific Results of the filtering/inference process.
 @details The default results contain both likelihoods for recognition
 and vectors for the results of a regression
 */
template <typename ModelType>
struct ClassResults {
    /**
     @brief Instantaneous likelihood
     */
    double instant_likelihood;

    /**
     @brief Cumulative log-likelihood computed on a sliding window
     */
    double log_likelihood;

    /**
     @brief Predicted Output parameter vector (only used in regression mode)
     @warning this variable is not allocated if the model is not bimodal
     */
    std::vector<float> output_values;

    /**
     @brief Predicted Output variance associated with the generated parameter
     vector (only used in regression mode)
     @warning this variable is not allocated if the model is not bimodal
     */
    std::vector<float> output_covariance;
};

/**
 @ingroup Model
 @brief Results of the filtering/inference process (for a Model with multiple
 classes).
 @details The default results contain both likelihoods for recognition
 and vectors for the results of a regression
 */
template <typename ModelType>
struct Results {
    /**
     @brief Instantaneous likelihood of each class
     */
    std::vector<double> instant_likelihoods;

    /**
     @brief Normalized instantaneous likelihood of each class
     */
    std::vector<double> instant_normalized_likelihoods;

    /**
     @brief Smoothed likelihood of each class
     */
    std::vector<double> smoothed_likelihoods;

    /**
     @brief Normalized smoothed likelihood of each class
     */
    std::vector<double> smoothed_normalized_likelihoods;

    /**
     @brief Cumulative smoothed log-likelihood of each class
     */
    std::vector<double> smoothed_log_likelihoods;

    /**
     @brief Label of the likeliest class
     */
    std::string likeliest;

    /**
     @brief Output values estimated by regression
     @warning this variable is not allocated if the Model is not bimodal
     */
    std::vector<float> output_values;

    /**
     @brief Output variance over the values estimated by regression
     @warning this variable is not allocated if the Model is not bimodal
     */
    std::vector<float> output_covariance;
};
}

#endif
