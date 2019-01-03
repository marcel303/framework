/*
 * xmmHmmResults.hpp
 *
 * Results of Hidden Markov Models for a single class
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

#ifndef xmmHmmResults_hpp
#define xmmHmmResults_hpp

#include "../../core/model/xmmModelResults.hpp"

namespace xmm {
/**
 @ingroup HMM
 @brief Results of Hidden Markov Models for a single class
 */
template <>
struct ClassResults<HMM> {
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
     @warning this variable only allocated if the model is bimodal
     */
    std::vector<float> output_values;

    /**
     @brief Predicted Output covariance associated with the generated parameter
     vector (only used in regression mode)
     @warning this variable only allocated if the model is bimodal
     */
    std::vector<float> output_covariance;

    /**
     @brief Estimated time progression.
     @details The time progression is computed as the centroid of the state
     probability distribution estimated by the forward algorithm
     */
    double progress;

    /**
     @brief Likelihood to exit the gesture on the next time step
     */
    double exit_likelihood;

    /**
     @brief Likelihood to exit the gesture on the next time step (normalized -/-
     total likelihood)
     */
    double exit_ratio;

    /**
     @brief Index of the likeliest state
     */
    unsigned int likeliest_state;
};
}

#endif
