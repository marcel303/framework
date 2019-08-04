/*
 * xmmModelSharedParameters.cpp
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

#include "xmmModelSharedParameters.hpp"

xmm::SharedParameters::SharedParameters()
    : bimodal(false),
      dimension((bimodal.get()) ? 2 : 1, (bimodal.get()) ? 2 : 1),
      dimension_input((bimodal.get()) ? 1 : 0, 0,
                      (bimodal.get()) ? dimension.get() : 0),
      em_algorithm_min_iterations(10, 1),
      em_algorithm_max_iterations(0),
      em_algorithm_percent_chg(0.01, 0.),
      likelihood_window(1, 1) {
    bimodal.onAttributeChange(this, &xmm::SharedParameters::onAttributeChange);
    dimension.onAttributeChange(this,
                                &xmm::SharedParameters::onAttributeChange);
    dimension_input.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_min_iterations.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_max_iterations.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_percent_chg.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    likelihood_window.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    column_names.onAttributeChange(this,
                                   &xmm::SharedParameters::onAttributeChange);
}

xmm::SharedParameters::SharedParameters(SharedParameters const& src)
    : bimodal(src.bimodal),
      dimension(src.dimension),
      dimension_input(src.dimension_input),
      column_names(src.column_names),
      em_algorithm_min_iterations(src.em_algorithm_min_iterations),
      em_algorithm_max_iterations(src.em_algorithm_max_iterations),
      em_algorithm_percent_chg(src.em_algorithm_percent_chg),
      likelihood_window(src.likelihood_window) {
    bimodal.onAttributeChange(this, &xmm::SharedParameters::onAttributeChange);
    dimension.onAttributeChange(this,
                                &xmm::SharedParameters::onAttributeChange);
    dimension_input.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_min_iterations.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_max_iterations.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    em_algorithm_percent_chg.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    likelihood_window.onAttributeChange(
        this, &xmm::SharedParameters::onAttributeChange);
    column_names.onAttributeChange(this,
                                   &xmm::SharedParameters::onAttributeChange);
}

xmm::SharedParameters::SharedParameters(Json::Value const& root)
    : SharedParameters() {
    bimodal.set(root.get("bimodal", false).asBool());
    dimension.set(root.get("dimension", bimodal.get() ? 2 : 1).asInt());
    dimension_input.set(
        root.get("dimension_input", bimodal.get() ? 1 : 0).asInt());
    em_algorithm_min_iterations.set(
        root.get("em_algorithm_min_iterations", 10).asInt());
    em_algorithm_max_iterations.set(
        root.get("em_algorithm_max_iterations", 0).asInt());
    em_algorithm_percent_chg.set(
        root.get("em_algorithm_percent_chg", 0.01).asFloat());
    likelihood_window.set(root.get("likelihood_window", 1).asInt());
    std::vector<std::string> tmpColNames(dimension.get());
    for (int i = 0; i < tmpColNames.size(); i++)
        tmpColNames[i] = root["column_names"].get(i, "").asString();
    column_names.set(tmpColNames);
}

xmm::SharedParameters& xmm::SharedParameters::operator=(
    SharedParameters const& src) {
    if (this != &src) {
        bimodal = src.bimodal;
        dimension = src.dimension;
        dimension_input = src.dimension_input;
        column_names = src.column_names;
        em_algorithm_min_iterations = src.em_algorithm_min_iterations;
        em_algorithm_max_iterations = src.em_algorithm_max_iterations;
        em_algorithm_percent_chg = src.em_algorithm_percent_chg;
        likelihood_window = src.likelihood_window;
    }
    return *this;
}

Json::Value xmm::SharedParameters::toJson() const {
    Json::Value root;
    root["bimodal"] = bimodal.get();
    root["dimension"] = static_cast<int>(dimension.get());
    root["dimension_input"] = static_cast<int>(dimension_input.get());
    for (int i = 0; i < column_names.size(); i++)
        root["column_names"][i] = column_names.at(i);
    root["em_algorithm_min_iterations"] = em_algorithm_min_iterations.get();
    root["em_algorithm_max_iterations"] = em_algorithm_max_iterations.get();
    root["em_algorithm_percent_chg"] = em_algorithm_percent_chg.get();
    root["likelihood_window"] = static_cast<int>(likelihood_window.get());
    return root;
}

void xmm::SharedParameters::fromJson(Json::Value const& root) {
    try {
        SharedParameters tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

void xmm::SharedParameters::onAttributeChange(AttributeBase* attr_pointer) {
    if (attr_pointer == &bimodal) {
        if (bimodal.get()) {
            dimension.setLimitMin(2);
            if (dimension.get() < 2) dimension.set(2);
            dimension_input.setLimitMin(1);
            if (dimension_input.get() < 1) dimension_input.set(1);
        } else {
            dimension.setLimitMin(1);
            if (dimension.get() < 1) dimension.set(1);
            dimension_input.setLimitMax(0);
            dimension_input.set(0);
        }
    } else if (attr_pointer == &dimension) {
        dimension_input.setLimitMax(dimension.get() - 1);
        column_names.resize(dimension.get());
    }
    attr_pointer->changed = false;
}
