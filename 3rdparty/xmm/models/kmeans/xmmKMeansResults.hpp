/*
 * xmmKMeansResults.hpp
 *
 * Results of the K-Means clustering Algorithm
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

#ifndef xmmKMeansResults_h
#define xmmKMeansResults_h

#include "../../core/model/xmmModelResults.hpp"

namespace xmm {
/**
 @ingroup KMeans
 @brief Results of the clustering process
 */
template <>
struct Results<KMeans> {
    /**
     @brief Distance of the observation to each cluster
     */
    std::vector<float> distances;

    /**
     @brief Likeliest Cluster
     */
    unsigned int likeliest;
};
}

#endif
