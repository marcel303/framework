/*
 * xmmGaussianDistribution.cpp
 *
 * Multivariate Gaussian Distribution
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

#include "../common/xmmMatrix.hpp"
#include "xmmGaussianDistribution.hpp"
#include <algorithm>
#include <math.h>

#ifdef WIN32

#define M_PI 3.14159265358979323846264338328 /**< pi */
//#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#pragma mark Constructors
xmm::GaussianDistribution::GaussianDistribution(bool bimodal,
                                                unsigned int dimension_,
                                                unsigned int dimension_input_,
                                                CovarianceMode covariance_mode_)
    : dimension(dimension_, (bimodal) ? 2 : 1),
      dimension_input(dimension_input_, 0, (bimodal) ? dimension_ - 1 : 0),
      covariance_mode(covariance_mode_),
      bimodal_(bimodal),
      covariance_determinant_(0.),
      covariance_determinant_input_(0.) {
    dimension.onAttributeChange(this,
                                &xmm::GaussianDistribution::onAttributeChange);
    dimension_input.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
    allocate();
}

xmm::GaussianDistribution::GaussianDistribution(GaussianDistribution const& src)
    : dimension(src.dimension),
      dimension_input(src.dimension_input),
      mean(src.mean),
      covariance_mode(src.covariance_mode),
      covariance(src.covariance),
      bimodal_(src.bimodal_),
      covariance_determinant_(src.covariance_determinant_),
      inverse_covariance_(src.inverse_covariance_) {
    dimension.onAttributeChange(this,
                                &xmm::GaussianDistribution::onAttributeChange);
    dimension_input.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
    covariance_determinant_input_ = src.covariance_determinant_input_;
    inverse_covariance_input_ = src.inverse_covariance_input_;
    output_covariance = src.output_covariance;
}

xmm::GaussianDistribution::GaussianDistribution(Json::Value const& root)
    : covariance_determinant_(0.), covariance_determinant_input_(0.) {
    bimodal_ = root.get("bimodal", false).asBool();
    dimension.set(root.get("dimension", bimodal_ ? 2 : 1).asInt());
    dimension_input.set(root.get("dimension_input", bimodal_ ? 1 : 0).asInt());
    covariance_mode.set(
        static_cast<CovarianceMode>(root["covariance_mode"].asInt()));

    allocate();

    json2vector(root["mean"], mean, dimension.get());
    json2vector(root["covariance"], covariance,
                dimension.get() * dimension.get());

    // updateInverseCovariance();
    // read from json instead of calling updateInverseCovariance() :
    json2vector(root["inverse_covariance"], inverse_covariance_,
                dimension.get() * dimension.get());
    covariance_determinant_ = root.get("covariance_determinant", 0.).asDouble();
    json2vector(root["inverse_covariance_input"], inverse_covariance_input_,
                dimension_input.get() * dimension_input.get());
    covariance_determinant_input_ =
        root.get("covariance_determinant_input", 0.).asDouble();

    dimension.onAttributeChange(this,
                                &xmm::GaussianDistribution::onAttributeChange);
    dimension_input.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
    covariance_mode.onAttributeChange(
        this, &xmm::GaussianDistribution::onAttributeChange);
}

xmm::GaussianDistribution& xmm::GaussianDistribution::operator=(
    GaussianDistribution const& src) {
    if (this != &src) {
        bimodal_ = src.bimodal_;
        dimension = src.dimension;
        dimension_input = src.dimension_input;
        covariance_mode = src.covariance_mode;

        mean = src.mean;
        covariance = src.covariance;
        inverse_covariance_ = src.inverse_covariance_;
        covariance_determinant_ = src.covariance_determinant_;
        covariance_determinant_input_ = src.covariance_determinant_input_;
        inverse_covariance_input_ = src.inverse_covariance_input_;
        output_covariance = src.output_covariance;
    }
    return *this;
};

void xmm::GaussianDistribution::onAttributeChange(AttributeBase* attr_pointer) {
    if (attr_pointer == &dimension) {
        dimension_input.setLimitMax(dimension.get() - 1);
    }
    if (attr_pointer == &dimension || attr_pointer == &dimension_input) {
        allocate();
    }
    if (attr_pointer == &covariance_mode) {
        if (covariance_mode.get() == CovarianceMode::Diagonal) {
            std::vector<double> new_covariance(dimension.get());
            for (unsigned int d = 0; d < dimension.get(); ++d) {
                new_covariance[d] = covariance[d * dimension.get() + d];
            }
            covariance = new_covariance;
            inverse_covariance_.resize(dimension.get());
            if (bimodal_)
                inverse_covariance_input_.resize(dimension_input.get());
        } else if (covariance_mode.get() == CovarianceMode::Full) {
            std::vector<double> new_covariance(
                dimension.get() * dimension.get(), 0.0);
            for (unsigned int d = 0; d < dimension.get(); ++d) {
                new_covariance[d * dimension.get() + d] = covariance[d];
            }
            covariance = new_covariance;
            inverse_covariance_.resize(dimension.get() * dimension.get());
            if (bimodal_)
                inverse_covariance_input_.resize(dimension_input.get() *
                                                 dimension_input.get());
        }
        updateInverseCovariance();
        if (bimodal_) {
            updateOutputCovariance();
        }
    }
    attr_pointer->changed = false;
}

#pragma mark Likelihood & Regression
double xmm::GaussianDistribution::likelihood(const float* observation) const {
    if (covariance_determinant_ == 0.0)
        throw std::runtime_error("Covariance Matrix is not invertible");

    double euclidianDistance(0.0);
    if (covariance_mode.get() == CovarianceMode::Full) {
        for (int l = 0; l < dimension.get(); l++) {
            double tmp(0.0);
            for (int k = 0; k < dimension.get(); k++) {
                tmp += inverse_covariance_[l * dimension.get() + k] *
                       (observation[k] - mean[k]);
            }
            euclidianDistance += (observation[l] - mean[l]) * tmp;
        }
    } else {
        for (int l = 0; l < dimension.get(); l++) {
            euclidianDistance += inverse_covariance_[l] *
                                 (observation[l] - mean[l]) *
                                 (observation[l] - mean[l]);
        }
    }

    double p =
        exp(-0.5 * euclidianDistance) /
        sqrt(covariance_determinant_ * pow(2 * M_PI, double(dimension.get())));

    if (p < 1e-180 || std::isnan(p) || std::isinf(fabs(p))) p = 1e-180;

    return p;
}

double xmm::GaussianDistribution::likelihood_input(
    const float* observation_input) const {
    if (!bimodal_)
        throw std::runtime_error(
            "'likelihood_input' can't be used when 'bimodal_' is off.");

    if (covariance_determinant_input_ == 0.0)
        throw std::runtime_error(
            "Covariance Matrix of input modality is not invertible");

    double euclidianDistance(0.0);
    if (covariance_mode.get() == CovarianceMode::Full) {
        for (int l = 0; l < dimension_input.get(); l++) {
            double tmp(0.0);
            for (int k = 0; k < dimension_input.get(); k++) {
                tmp +=
                    inverse_covariance_input_[l * dimension_input.get() + k] *
                    (observation_input[k] - mean[k]);
            }
            euclidianDistance += (observation_input[l] - mean[l]) * tmp;
        }
    } else {
        for (int l = 0; l < dimension_input.get(); l++) {
            euclidianDistance += inverse_covariance_[l] *
                                 (observation_input[l] - mean[l]) *
                                 (observation_input[l] - mean[l]);
        }
    }

    double p = exp(-0.5 * euclidianDistance) /
               sqrt(covariance_determinant_input_ *
                    pow(2 * M_PI, double(dimension_input.get())));

    if (p < 1e-180 || std::isnan(p) || std::isinf(fabs(p))) p = 1e-180;

    return p;
}

double xmm::GaussianDistribution::likelihood_bimodal(
    const float* observation_input, const float* observation_output) const {
    if (!bimodal_)
        throw std::runtime_error(
            "'likelihood_bimodal' can't be used when 'bimodal_' is off.");

    if (covariance_determinant_ == 0.0)
        throw std::runtime_error("Covariance Matrix is not invertible");

    unsigned int dimension_output = dimension.get() - dimension_input.get();
    double euclidianDistance(0.0);
    if (covariance_mode.get() == CovarianceMode::Full) {
        for (int l = 0; l < dimension.get(); l++) {
            double tmp(0.0);
            for (int k = 0; k < dimension_input.get(); k++) {
                tmp += inverse_covariance_[l * dimension.get() + k] *
                       (observation_input[k] - mean[k]);
            }
            for (int k = 0; k < dimension_output; k++) {
                tmp +=
                    inverse_covariance_[l * dimension.get() +
                                        dimension_input.get() + k] *
                    (observation_output[k] - mean[dimension_input.get() + k]);
            }
            if (l < dimension_input.get())
                euclidianDistance += (observation_input[l] - mean[l]) * tmp;
            else
                euclidianDistance +=
                    (observation_output[l - dimension_input.get()] - mean[l]) *
                    tmp;
        }
    } else {
        for (int l = 0; l < dimension_input.get(); l++) {
            euclidianDistance += inverse_covariance_[l] *
                                 (observation_input[l] - mean[l]) *
                                 (observation_input[l] - mean[l]);
        }
        for (unsigned int l = dimension_input.get(); l < dimension.get(); l++) {
            euclidianDistance +=
                inverse_covariance_[l] *
                (observation_output[l - dimension_input.get()] - mean[l]) *
                (observation_output[l - dimension_input.get()] - mean[l]);
        }
    }

    double p =
        exp(-0.5 * euclidianDistance) /
        sqrt(covariance_determinant_ * pow(2 * M_PI, (double)dimension.get()));

    if (p < 1e-180 || std::isnan(p) || std::isinf(fabs(p))) p = 1e-180;

    return p;
}

void xmm::GaussianDistribution::regression(
    std::vector<float> const& observation_input,
    std::vector<float>& predicted_output) const {
    if (!bimodal_)
        throw std::runtime_error(
            "'regression' can't be used when 'bimodal_' is off.");

    unsigned int dimension_output = dimension.get() - dimension_input.get();
    predicted_output.resize(dimension_output);

    if (covariance_mode.get() == CovarianceMode::Full) {
        for (int d = 0; d < dimension_output; d++) {
            predicted_output[d] = mean[dimension_input.get() + d];
            for (int e = 0; e < dimension_input.get(); e++) {
                float tmp = 0.;
                for (int f = 0; f < dimension_input.get(); f++) {
                    tmp += inverse_covariance_input_[e * dimension_input.get() +
                                                     f] *
                           (observation_input[f] - mean[f]);
                }
                predicted_output[d] +=
                    covariance[(d + dimension_input.get()) * dimension.get() +
                               e] *
                    tmp;
            }
        }
    } else {
        for (int d = 0; d < dimension_output; d++) {
            predicted_output[d] = mean[dimension_input.get() + d];
        }
    }
}

#pragma mark JSON I/O
Json::Value xmm::GaussianDistribution::toJson() const {
    Json::Value root;
    root["bimodal"] = bimodal_;
    root["dimension"] = static_cast<int>(dimension.get());
    root["dimension_input"] = static_cast<int>(dimension_input.get());
    root["covariance_mode"] = static_cast<int>(covariance_mode.get());
    root["mean"] = vector2json(mean);
    root["covariance"] = vector2json(covariance);

    root["inverse_covariance"] = vector2json(inverse_covariance_);
    root["covariance_determinant"] = covariance_determinant_;
    root["inverse_covariance_input"] = vector2json(inverse_covariance_input_);
    root["covariance_determinant_input"] = covariance_determinant_input_;

    return root;
}

void xmm::GaussianDistribution::fromJson(Json::Value const& root) {
    try {
        GaussianDistribution tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

#pragma mark Utilities
void xmm::GaussianDistribution::allocate() {
    mean.resize(dimension.get());
    if (covariance_mode.get() == CovarianceMode::Full) {
        covariance.resize(dimension.get() * dimension.get());
        inverse_covariance_.resize(dimension.get() * dimension.get());
        if (bimodal_)
            inverse_covariance_input_.resize(dimension_input.get() *
                                             dimension_input.get());
    } else {
        covariance.resize(dimension.get());
        inverse_covariance_.resize(dimension.get());
        if (bimodal_) inverse_covariance_input_.resize(dimension_input.get());
    }
}

void xmm::GaussianDistribution::regularize(std::vector<double> regularization) {
    if (covariance_mode.get() == CovarianceMode::Full) {
        for (int d = 0; d < dimension.get(); ++d) {
            covariance[d * dimension.get() + d] += regularization[d];
        }
    } else {
        for (int d = 0; d < dimension.get(); ++d) {
            covariance[d] += regularization[d];
        }
    }
}

void xmm::GaussianDistribution::updateInverseCovariance() {
    if (covariance_mode.get() == CovarianceMode::Full) {
        Matrix<double> cov_matrix(dimension.get(), dimension.get(), false);

        Matrix<double>* inverseMat;
        double det;

        cov_matrix.data = covariance.begin();
        inverseMat = cov_matrix.pinv(&det);
        covariance_determinant_ = det;
        copy(inverseMat->data,
             inverseMat->data + dimension.get() * dimension.get(),
             inverse_covariance_.begin());
        delete inverseMat;
        inverseMat = NULL;

        // If regression active: create inverse covariance matrix for input
        // modality.
        if (bimodal_) {
            Matrix<double> cov_matrix_input(dimension_input.get(),
                                            dimension_input.get(), true);
            for (int d1 = 0; d1 < dimension_input.get(); d1++) {
                for (int d2 = 0; d2 < dimension_input.get(); d2++) {
                    cov_matrix_input._data[d1 * dimension_input.get() + d2] =
                        covariance[d1 * dimension.get() + d2];
                }
            }
            inverseMat = cov_matrix_input.pinv(&det);
            covariance_determinant_input_ = det;
            copy(inverseMat->data,
                 inverseMat->data +
                     dimension_input.get() * dimension_input.get(),
                 inverse_covariance_input_.begin());
            delete inverseMat;
            inverseMat = NULL;
        }
    } else  // DIAGONAL COVARIANCE
    {
        covariance_determinant_ = 1.;
        covariance_determinant_input_ = 1.;
        for (unsigned int d = 0; d < dimension.get(); ++d) {
            if (covariance[d] <= 0.0)
                throw std::runtime_error("Non-invertible matrix");
            inverse_covariance_[d] = 1. / covariance[d];
            covariance_determinant_ *= covariance[d];
            if (bimodal_ && d < dimension_input.get()) {
                inverse_covariance_input_[d] = 1. / covariance[d];
                covariance_determinant_input_ *= covariance[d];
            }
        }
    }
    if (bimodal_) {
        this->updateOutputCovariance();
    }
}

void xmm::GaussianDistribution::updateOutputCovariance() {
    if (!bimodal_)
        throw std::runtime_error(
            "'updateOutputVariances' can't be used when 'bimodal_' is off.");

    unsigned int dimension_output = dimension.get() - dimension_input.get();

    // CASE: DIAGONAL COVARIANCE
    if (covariance_mode.get() == CovarianceMode::Diagonal) {
        output_covariance.resize(dimension_output);
        copy(covariance.begin() + dimension_input.get(),
             covariance.begin() + dimension.get(), output_covariance.begin());
        return;
    }

    // CASE: FULL COVARIANCE
    Matrix<double>* inverseMat;
    double det;

    Matrix<double> cov_matrix_input(dimension_input.get(),
                                    dimension_input.get(), true);
    for (int d1 = 0; d1 < dimension_input.get(); d1++) {
        for (int d2 = 0; d2 < dimension_input.get(); d2++) {
            cov_matrix_input._data[d1 * dimension_input.get() + d2] =
                covariance[d1 * dimension.get() + d2];
        }
    }
    inverseMat = cov_matrix_input.pinv(&det);
    Matrix<double> covariance_gs(dimension_input.get(), dimension_output, true);
    for (int d1 = 0; d1 < dimension_input.get(); d1++) {
        for (int d2 = 0; d2 < dimension_output; d2++) {
            covariance_gs._data[d1 * dimension_output + d2] =
                covariance[d1 * dimension.get() + dimension_input.get() + d2];
        }
    }
    Matrix<double> covariance_sg(dimension_output, dimension_input.get(), true);
    for (int d1 = 0; d1 < dimension_output; d1++) {
        for (int d2 = 0; d2 < dimension_input.get(); d2++) {
            covariance_sg._data[d1 * dimension_input.get() + d2] =
                covariance[(dimension_input.get() + d1) * dimension.get() + d2];
        }
    }
    Matrix<double>* tmptmptmp = inverseMat->product(&covariance_gs);
    Matrix<double>* covariance_mod = covariance_sg.product(tmptmptmp);
    output_covariance.resize(dimension_output * dimension_output);
    for (int d1 = 0; d1 < dimension_output; d1++) {
        for (int d2 = 0; d2 < dimension_output; d2++) {
            output_covariance[d1 * dimension_output + d2] =
                covariance[(dimension_input.get() + d1) * dimension.get() +
                           dimension_input.get() + d2] -
                covariance_mod->data[d1 * dimension_output + d2];
        }
    }
    delete inverseMat;
    delete covariance_mod;
    delete tmptmptmp;
    inverseMat = NULL;
    covariance_mod = NULL;
    tmptmptmp = NULL;
}

xmm::Ellipse xmm::GaussianDistribution::toEllipse(unsigned int dimension1,
                                                  unsigned int dimension2) {
    if (dimension1 >= dimension.get() || dimension2 >= dimension.get())
        throw std::out_of_range("dimensions out of range");

    Ellipse gaussian_ellipse;
    gaussian_ellipse.x = mean[dimension1];
    gaussian_ellipse.y = mean[dimension2];

    // Represent 2D covariance with square matrix
    // |a b|
    // |b c|
    double a, b, c;
    if (covariance_mode.get() == CovarianceMode::Full) {
        a = covariance[dimension1 * dimension.get() + dimension1];
        b = covariance[dimension1 * dimension.get() + dimension2];
        c = covariance[dimension2 * dimension.get() + dimension2];
    } else {
        a = covariance[dimension1];
        b = 0.0;
        c = covariance[dimension2];
    }

    // Compute Eigen Values to get width, height and angle
    double trace = a + c;
    double determinant = a * c - b * b;
    double eigenVal1 = 0.5 * (trace + sqrt(trace * trace - 4 * determinant));
    double eigenVal2 = 0.5 * (trace - sqrt(trace * trace - 4 * determinant));
    gaussian_ellipse.width = sqrt(5.991 * eigenVal1);
    gaussian_ellipse.height = sqrt(5.991 * eigenVal2);
    gaussian_ellipse.angle = atan(b / (eigenVal1 - c));
    if (isnan(gaussian_ellipse.angle)) {
        gaussian_ellipse.angle = M_PI_2;
    }

    return gaussian_ellipse;
}

void xmm::GaussianDistribution::fromEllipse(Ellipse const& gaussian_ellipse,
                                            unsigned int dimension1,
                                            unsigned int dimension2) {
    if (dimension1 >= dimension.get() || dimension2 >= dimension.get())
        throw std::out_of_range("dimensions out of range");

    mean[dimension1] = gaussian_ellipse.x;
    mean[dimension2] = gaussian_ellipse.y;

    double eigenVal1 = gaussian_ellipse.width * gaussian_ellipse.width / 5.991;
    double eigenVal2 =
        gaussian_ellipse.height * gaussian_ellipse.height / 5.991;
    double tantheta = std::tan(gaussian_ellipse.angle);
    double a, b, c;
    b = (eigenVal1 - eigenVal2) * tantheta / (tantheta * tantheta + 1.);
    c = eigenVal1 - b / tantheta;
    a = eigenVal2 + b / tantheta;

    if (covariance_mode.get() == CovarianceMode::Full) {
        covariance[dimension1 * dimension.get() + dimension1] = a;
        covariance[dimension1 * dimension.get() + dimension2] = b;
        covariance[dimension2 * dimension.get() + dimension1] = b;
        covariance[dimension2 * dimension.get() + dimension2] = c;
    } else {
        covariance[dimension1] = a;
        covariance[dimension2] = c;
    }
    updateInverseCovariance();
}

// void xmm::GaussianDistribution::makeBimodal(unsigned int dimension_input_)
//{
//    if (bimodal_)
//        throw std::runtime_error("The model is already bimodal");
//    if (dimension_input_ >= dimension.get())
//        throw std::out_of_range("Request input dimension exceeds the current
//        dimension");
//    this->bimodal_ = true;
//    dimension_input.setLimitMax(dimension.get() - 1);
//    dimension_input.set(dimension_input_, true);
//    if (covariance_mode.get() == CovarianceMode::Full) {
//        this->inverse_covariance_input_.resize(dimension_input_*dimension_input_);
//    } else {
//        this->inverse_covariance_input_.resize(dimension_input_);
//    }
//    this->updateInverseCovariance();
//    this->updateOutputVariance();
//}
//
// void xmm::GaussianDistribution::makeUnimodal()
//{
//    if (!bimodal_)
//        throw std::runtime_error("The model is already unimodal");
//    this->bimodal_ = false;
//    this->dimension_input.set(0, true);
//    this->inverse_covariance_input_.clear();
//}
//
// xmm::GaussianDistribution
// xmm::GaussianDistribution::extractSubmodel(std::vector<unsigned int>&
// columns)
// const
//{
//    if (columns.size() > dimension.get())
//        throw std::out_of_range("requested number of columns exceeds the
//        dimension of the current model");
//    for (unsigned int column=0; column<columns.size(); ++column) {
//        if (columns[column] >= dimension.get())
//            throw std::out_of_range("Some column indices exceeds the dimension
//            of the current model");
//    }
//    size_t new_dim =columns.size();
//    GaussianDistribution target_distribution(NONE,
//    static_cast<unsigned int>(new_dim), 0, relative_regularization,
//    absolute_regularization);
//    target_distribution.allocate();
//    for (unsigned int new_index1=0; new_index1<new_dim; ++new_index1) {
//        unsigned int col_index1 = columns[new_index1];
//        target_distribution.mean[new_index1] = mean[col_index1];
//        target_distribution.scale[new_index1] = scale[col_index1];
//        if (covariance_mode.get() == CovarianceMode::Full) {
//            for (unsigned int new_index2=0; new_index2<new_dim; ++new_index2)
//            {
//                unsigned int col_index2 = columns[new_index2];
//                target_distribution.covariance[new_index1*new_dim+new_index2]
//                = covariance[col_index1*dimension.get()+col_index2];
//            }
//        } else {
//            target_distribution.covariance[new_index1] =
//            covariance[col_index1];
//        }
//    }
//    try {
//        target_distribution.updateInverseCovariance();
//    } catch (std::exception const& e) {
//    }
//    return target_distribution;
//}
//
// xmm::GaussianDistribution xmm::GaussianDistribution::extractSubmodel_input()
// const
//{
//    if (!bimodal_)
//        throw std::runtime_error("The distribution needs to be bimodal");
//    std::vector<unsigned int> columns_input(dimension_input.get());
//    for (unsigned int i=0; i<dimension_input.get(); ++i) {
//        columns_input[i] = i;
//    }
//    return extractSubmodel(columns_input);
//}
//
// xmm::GaussianDistribution xmm::GaussianDistribution::extractSubmodel_output()
// const
//{
//    if (!bimodal_)
//        throw std::runtime_error("The distribution needs to be bimodal");
//    std::vector<unsigned int> columns_output(dimension.get() -
//    dimension_input.get());
//    for (unsigned int i=dimension_input.get(); i<dimension.get(); ++i) {
//        columns_output[i-dimension_input.get()] = i;
//    }
//    return extractSubmodel(columns_output);
//}
//
// xmm::GaussianDistribution xmm::GaussianDistribution::extract_inverse_model()
// const
//{
//    if (!bimodal_)
//        throw std::runtime_error("The distribution needs to be bimodal");
//    std::vector<unsigned int> columns(dimension.get());
//    for (unsigned int i=0; i<dimension.get()-dimension_input.get(); ++i) {
//        columns[i] = i + dimension_input.get();
//    }
//    for (unsigned int i=dimension.get()-dimension_input.get(), j=0;
//    i<dimension.get(); ++i, ++j) {
//        columns[i] = j;
//    }
//    GaussianDistribution target_distribution = extractSubmodel(columns);
//    target_distribution.makeBimodal(dimension.get() - dimension_input.get());
//    return target_distribution;
//}

xmm::Ellipse xmm::covariance2ellipse(double c_xx, double c_xy, double c_yy) {
    Ellipse gaussian_ellipse;
    gaussian_ellipse.x = 0.;
    gaussian_ellipse.y = 0.;

    // Compute Eigen Values to get width, height and angle
    double trace = c_xx + c_yy;
    double determinant = c_xx * c_yy - c_xy * c_xy;
    double eigenVal1 = 0.5 * (trace + sqrt(trace * trace - 4 * determinant));
    double eigenVal2 = 0.5 * (trace - sqrt(trace * trace - 4 * determinant));
    gaussian_ellipse.width = sqrt(5.991 * eigenVal1);
    gaussian_ellipse.height = sqrt(5.991 * eigenVal2);
    gaussian_ellipse.angle = atan(c_xy / (eigenVal1 - c_yy));
    if (isnan(gaussian_ellipse.angle)) {
        gaussian_ellipse.angle = M_PI_2;
    }

    return gaussian_ellipse;
}

template <>
void xmm::checkLimits<xmm::GaussianDistribution::CovarianceMode>(
    xmm::GaussianDistribution::CovarianceMode const& value,
    xmm::GaussianDistribution::CovarianceMode const& limit_min,
    xmm::GaussianDistribution::CovarianceMode const& limit_max) {
    if (value < limit_min || value > limit_max)
        throw std::domain_error(
            "Attribute value out of range. Range: [" +
            std::to_string(static_cast<int>(limit_min)) + " ; " +
            std::to_string(static_cast<int>(limit_max)) + "]");
}

template <>
xmm::GaussianDistribution::CovarianceMode
xmm::Attribute<xmm::GaussianDistribution::CovarianceMode>::defaultLimitMax() {
    return xmm::GaussianDistribution::CovarianceMode::Diagonal;
}
