/*
 * xmmAttribute.hpp
 *
 * Generic Attributes
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

#include "xmmAttribute.hpp"
#include <string>

template <>
void xmm::checkLimits<bool>(bool const& value, bool const& limit_min,
                            bool const& limit_max) {}

template <>
void xmm::checkLimits<unsigned char>(unsigned char const& value,
                                     unsigned char const& limit_min,
                                     unsigned char const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<char>(char const& value, char const& limit_min,
                            char const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<unsigned int>(unsigned int const& value,
                                    unsigned int const& limit_min,
                                    unsigned int const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<int>(int const& value, int const& limit_min,
                           int const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<long>(long const& value, long const& limit_min,
                            long const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<float>(float const& value, float const& limit_min,
                             float const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<double>(double const& value, double const& limit_min,
                              double const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error("Attribute value out of range. Range: [" +
                                std::to_string(limit_min) + " ; " +
                                std::to_string(limit_max) + "]");
}

template <>
void xmm::checkLimits<std::string>(std::string const& value,
                                   std::string const& limit_min,
                                   std::string const& limit_max) {}
