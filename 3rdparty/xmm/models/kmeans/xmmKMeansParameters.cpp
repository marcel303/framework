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

#include "xmmKMeansParameters.hpp"

xmm::ClassParameters<xmm::KMeans>::ClassParameters()
    : changed(true),
      clusters(1, 1),
      max_iterations(50, 1),
      relative_distance_threshold(1e-20, 0) {
    clusters.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
    max_iterations.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
    relative_distance_threshold.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
}

xmm::ClassParameters<xmm::KMeans>::ClassParameters(
    ClassParameters<KMeans> const& src)
    : changed(true),
      clusters(src.clusters),
      max_iterations(src.max_iterations),
      relative_distance_threshold(src.relative_distance_threshold) {
    clusters.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
    max_iterations.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
    relative_distance_threshold.onAttributeChange(
        this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
}

xmm::ClassParameters<xmm::KMeans>::ClassParameters(Json::Value const& root)
    : ClassParameters() {
    clusters.set(root["clusters"].asInt());
    max_iterations.set(root["max_iterations"].asInt());
    relative_distance_threshold.set(
        root["relative_distance_threshold"].asFloat());
}

xmm::ClassParameters<xmm::KMeans>& xmm::ClassParameters<xmm::KMeans>::operator=(
    ClassParameters<KMeans> const& src) {
    if (this != &src) {
        changed = true;
        clusters = src.clusters;
        max_iterations = src.max_iterations;
        relative_distance_threshold = src.relative_distance_threshold;
        clusters.onAttributeChange(
            this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
        max_iterations.onAttributeChange(
            this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
        relative_distance_threshold.onAttributeChange(
            this, &xmm::ClassParameters<xmm::KMeans>::onAttributeChange);
    }
    return *this;
}

Json::Value xmm::ClassParameters<xmm::KMeans>::toJson() const {
    Json::Value root;
    root["clusters"] = static_cast<int>(clusters.get());
    root["max_iterations"] = static_cast<int>(max_iterations.get());
    root["relative_distance_threshold"] = relative_distance_threshold.get();
    return root;
}

void xmm::ClassParameters<xmm::KMeans>::fromJson(Json::Value const& root) {
    try {
        ClassParameters<KMeans> tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

void xmm::ClassParameters<xmm::KMeans>::onAttributeChange(
    AttributeBase* attr_pointer) {
    changed = true;
    attr_pointer->changed = false;
}
