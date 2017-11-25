/*
 * xmmJson.cpp
 *
 * Set of utility functions for JSON I/O
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

#include "xmmJson.hpp"

template <>
void xmm::json2array(Json::Value const& root, float* a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asFloat();
    }
}

template <>
void xmm::json2array(Json::Value const& root, double* a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asDouble();
    }
}

template <>
void xmm::json2array(Json::Value const& root, bool* a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asBool();
    }
}

template <>
void xmm::json2array(Json::Value const& root, std::string* a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asString();
    }
}

template <>
void xmm::json2vector(Json::Value const& root, std::vector<float>& a,
                      unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asFloat();
    }
}

template <>
void xmm::json2vector(Json::Value const& root, std::vector<double>& a,
                      unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asDouble();
    }
}

template <>
void xmm::json2vector(Json::Value const& root, std::vector<bool>& a,
                      unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asBool();
    }
}

template <>
void xmm::json2vector(Json::Value const& root, std::vector<std::string>& a,
                      unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asString();
    }
}
