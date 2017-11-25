/*
 * xmmGmmParameters.cpp
 *
 * Parameters of Gaussian Mixture Models
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

#include "xmmGmmParameters.hpp"

xmm::ClassParameters<xmm::GMM>::ClassParameters()
    : changed(true),
      gaussians(10, 1),
      relative_regularization(1.0e-2, 1e-20),
      absolute_regularization(1.0e-3, 1e-20),
      covariance_mode(GaussianDistribution::CovarianceMode::Full) {
    gaussians.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    relative_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    absolute_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
}

xmm::ClassParameters<xmm::GMM>::ClassParameters(ClassParameters<GMM> const& src)
    : changed(true),
      gaussians(src.gaussians),
      relative_regularization(src.relative_regularization),
      absolute_regularization(src.absolute_regularization),
      covariance_mode(src.covariance_mode) {
    gaussians.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    relative_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    absolute_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
}

xmm::ClassParameters<xmm::GMM>::ClassParameters(Json::Value const& root)
    : changed(true),
      gaussians(10, 1),
      relative_regularization(1.0e-2, 1e-20),
      absolute_regularization(1.0e-3, 1e-20),
      covariance_mode(GaussianDistribution::CovarianceMode::Full) {
    gaussians.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    relative_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    absolute_regularization.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);

    gaussians.set(root["gaussians"].asInt());
    relative_regularization.set(root["relative_regularization"].asFloat());
    absolute_regularization.set(root["absolute_regularization"].asFloat());
    covariance_mode.set(static_cast<GaussianDistribution::CovarianceMode>(
        root["covariance_mode"].asInt()));
}

xmm::ClassParameters<xmm::GMM>& xmm::ClassParameters<xmm::GMM>::operator=(
    ClassParameters<GMM> const& src) {
    if (this != &src) {
        changed = true;
        gaussians = src.gaussians;
        relative_regularization = src.relative_regularization;
        absolute_regularization = src.absolute_regularization;
        covariance_mode = src.covariance_mode;

        gaussians.onAttributeChange(
            this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
        relative_regularization.onAttributeChange(
            this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
        absolute_regularization.onAttributeChange(
            this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
        covariance_mode.onAttributeChange(
            this, &xmm::ClassParameters<xmm::GMM>::onAttributeChange);
    }
    return *this;
}

Json::Value xmm::ClassParameters<xmm::GMM>::toJson() const {
    Json::Value root;
    root["gaussians"] = static_cast<int>(gaussians.get());
    root["relative_regularization"] = relative_regularization.get();
    root["absolute_regularization"] = absolute_regularization.get();
    root["covariance_mode"] = static_cast<int>(covariance_mode.get());
    return root;
}

void xmm::ClassParameters<xmm::GMM>::fromJson(Json::Value const& root) {
    try {
        ClassParameters<GMM> tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

void xmm::ClassParameters<xmm::GMM>::onAttributeChange(
    AttributeBase* attr_pointer) {
    changed = true;
    attr_pointer->changed = false;
}
