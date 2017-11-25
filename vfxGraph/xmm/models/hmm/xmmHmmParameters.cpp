/*
 * xmmHmmParameters.cpp
 *
 * Parameters of Hidden Markov Models
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

#include "xmmHmmParameters.hpp"

xmm::ClassParameters<xmm::HMM>::ClassParameters()
    : changed(true),
      states(10, 1),
      gaussians(1, 1),
      relative_regularization(1.0e-2, 1e-20),
      absolute_regularization(1.0e-3, 1e-20),
      covariance_mode(GaussianDistribution::CovarianceMode::Full),
      transition_mode(HMM::TransitionMode::LeftRight),
      regression_estimator(HMM::RegressionEstimator::Full),
      hierarchical(true) {
    states.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    gaussians.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    relative_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    absolute_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    transition_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    regression_estimator.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    hierarchical.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
}

xmm::ClassParameters<xmm::HMM>::ClassParameters(ClassParameters<HMM> const& src)
    : changed(true),
      states(src.states),
      gaussians(src.gaussians),
      relative_regularization(src.relative_regularization),
      absolute_regularization(src.absolute_regularization),
      covariance_mode(src.covariance_mode),
      transition_mode(src.transition_mode),
      regression_estimator(src.regression_estimator),
      hierarchical(src.hierarchical) {
    states.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    gaussians.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    relative_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    absolute_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    transition_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    regression_estimator.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    hierarchical.onAttributeChange(
        this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
}

xmm::ClassParameters<xmm::HMM>::ClassParameters(Json::Value const& root)
    : ClassParameters() {
    states.set(root["states"].asInt());
    gaussians.set(root["gaussians"].asInt());
    relative_regularization.set(root["relative_regularization"].asFloat());
    absolute_regularization.set(root["absolute_regularization"].asFloat());
    covariance_mode.set(static_cast<GaussianDistribution::CovarianceMode>(
        root["covariance_mode"].asInt()));
    transition_mode.set(
        static_cast<HMM::TransitionMode>(root["transition_mode"].asInt()));
    regression_estimator.set(static_cast<HMM::RegressionEstimator>(
        root["regression_estimator"].asInt()));
    hierarchical.set(root["hierarchical"].asBool());
}

xmm::ClassParameters<xmm::HMM>& xmm::ClassParameters<xmm::HMM>::operator=(
    ClassParameters<HMM> const& src) {
    if (this != &src) {
        changed = true;
        states = src.states;
        gaussians = src.gaussians;
        relative_regularization = src.relative_regularization;
        absolute_regularization = src.absolute_regularization;
        covariance_mode = src.covariance_mode;
        transition_mode = src.transition_mode;
        regression_estimator = src.regression_estimator;
        states.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        gaussians.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        relative_regularization.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        absolute_regularization.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        covariance_mode.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        transition_mode.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        regression_estimator.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
        hierarchical.onAttributeChange(
            this, &xmm::ClassParameters<xmm::HMM>::onAttributeChange);
    }
    return *this;
}

Json::Value xmm::ClassParameters<xmm::HMM>::toJson() const {
    Json::Value root;
    root["states"] = static_cast<int>(states.get());
    root["gaussians"] = static_cast<int>(gaussians.get());
    root["relative_regularization"] = relative_regularization.get();
    root["absolute_regularization"] = absolute_regularization.get();
    root["covariance_mode"] = static_cast<int>(covariance_mode.get());
    root["transition_mode"] = static_cast<int>(transition_mode.get());
    root["regression_estimator"] = static_cast<int>(regression_estimator.get());
    root["hierarchical"] = hierarchical.get();
    return root;
}

void xmm::ClassParameters<xmm::HMM>::fromJson(Json::Value const& root) {
    try {
        ClassParameters<HMM> tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

void xmm::ClassParameters<xmm::HMM>::onAttributeChange(
    AttributeBase* attr_pointer) {
    changed = true;
    attr_pointer->changed = false;
}

template <>
void xmm::checkLimits<xmm::HMM::TransitionMode>(
    xmm::HMM::TransitionMode const& value,
    xmm::HMM::TransitionMode const& limit_min,
    xmm::HMM::TransitionMode const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error(
            "Attribute value out of range. Range: [" +
            std::to_string(static_cast<int>(limit_min)) + " ; " +
            std::to_string(static_cast<int>(limit_max)) + "]");
}

template <>
xmm::HMM::TransitionMode
xmm::Attribute<xmm::HMM::TransitionMode>::defaultLimitMax() {
    return xmm::HMM::TransitionMode::LeftRight;
}

template <>
void xmm::checkLimits<xmm::HMM::RegressionEstimator>(
    xmm::HMM::RegressionEstimator const& value,
    xmm::HMM::RegressionEstimator const& limit_min,
    xmm::HMM::RegressionEstimator const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error(
            "Attribute value out of range. Range: [" +
            std::to_string(static_cast<int>(limit_min)) + " ; " +
            std::to_string(static_cast<int>(limit_max)) + "]");
}

template <>
xmm::HMM::RegressionEstimator
xmm::Attribute<xmm::HMM::RegressionEstimator>::defaultLimitMax() {
    return xmm::HMM::RegressionEstimator::Likeliest;
}
