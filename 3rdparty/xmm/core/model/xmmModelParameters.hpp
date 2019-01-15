/*
 * xmmModelParameters.hpp
 *
 * Model Parameters for each class (label)
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

#ifndef xmmModelParameters_h
#define xmmModelParameters_h

#include "../common/xmmAttribute.hpp"
#include "../common/xmmJson.hpp"

namespace xmm {
/**
 @ingroup Model
 @brief Class-specific Model Parameters.
 @details this structure is then encapsulated in a Configuration object that
 propagates default parameters
 or class-specific parameters to the SingleClassModel.
 */
template <typename ModelType>
class ClassParameters : public Writable {
  public:
    /**
     @brief Default Constructor
     */
    ClassParameters() : changed(true) {}

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    ClassParameters(ClassParameters const& src) : changed(true) {}

    /**
     @brief Constructor from Json Structure
     @param root Json value
     */
    explicit ClassParameters(Json::Value const& root) : changed(true) {}

    /**
     @brief Assignment
     @param src Source Object
     */
    ClassParameters& operator=(ClassParameters const& src) { changed = true; }

    /**
     @brief specifies if parameters have changed (model is invalid)
     */
    bool changed;
};
}

#endif
