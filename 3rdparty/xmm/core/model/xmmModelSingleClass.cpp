/*
 * xmmModelSingleClass.hpp
 *
 * Abstract class for Single-class Probabilistic Machine learning models based
 * on the EM algorithm
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

#include "xmmModelSingleClass.hpp"
#include <cmath>

#pragma mark -
#pragma mark Constructors
xmm::SingleClassProbabilisticModel::SingleClassProbabilisticModel(
    std::shared_ptr<SharedParameters> p)
    : label(""),
      shared_parameters(p),
      training_status(this, label),
      is_training_(false),
      cancel_training_(false) {
    if (p == NULL) {
        throw std::runtime_error(
            "Cannot instantiate a probabilistic model without Shared "
            "parameters.");
    }
    likelihood_buffer_.resize(shared_parameters->likelihood_window.get());
}

xmm::SingleClassProbabilisticModel::SingleClassProbabilisticModel(
    SingleClassProbabilisticModel const& src)
    : label(src.label),
      shared_parameters(src.shared_parameters),
      training_status(this, label),
      is_training_(false),
      cancel_training_(false) {
    if (is_training_)
        throw std::runtime_error("Cannot copy: target model is still training");
    if (src.is_training_)
        throw std::runtime_error("Cannot copy: source model is still training");
}

xmm::SingleClassProbabilisticModel::SingleClassProbabilisticModel(
    std::shared_ptr<SharedParameters> p, Json::Value const& root)
    : label(""),
      shared_parameters(p),
      training_status(this, label),
      is_training_(false),
      cancel_training_(false) {
    if (p == NULL) {
        throw std::runtime_error(
            "Cannot instantiate a probabilistic model without Shared "
            "parameters.");
    }
    label = root["label"].asString();
    training_status.label = label;
}

xmm::SingleClassProbabilisticModel& xmm::SingleClassProbabilisticModel::
operator=(SingleClassProbabilisticModel const& src) {
    if (this != &src) {
        if (is_training_)
            throw std::runtime_error(
                "Cannot copy: target model is still training");
        if (src.is_training_)
            throw std::runtime_error(
                "Cannot copy: source model is still training");
        label = src.label;
        training_status = TrainingEvent(this, label),
        shared_parameters = src.shared_parameters;
        is_training_ = false;
        cancel_training_ = false;
    }
    return *this;
};

xmm::SingleClassProbabilisticModel::~SingleClassProbabilisticModel() {
    while (this->isTraining()) {
    }
}

#pragma mark -
#pragma mark Accessors
bool xmm::SingleClassProbabilisticModel::isTraining() const {
    return is_training_;
}

#pragma mark -
#pragma mark Training
void xmm::SingleClassProbabilisticModel::train(TrainingSet* trainingSet) {
    training_mutex_.lock();
    bool trainingError(false);

    training_status.status = TrainingEvent::Status::Run;

    if (trainingSet && !trainingSet->empty()) {
        this->allocate();
    } else {
        trainingError = true;
    }

    if (cancelTrainingIfRequested()) return;
    if (!trainingError) {
        try {
            this->emAlgorithmInit(trainingSet);
        } catch (std::exception& e) {
            trainingError = true;
        }
    }

    training_status.label = label;
    training_status.log_likelihood = -std::numeric_limits<double>::max();
    training_status.iterations = 0;
    double old_log_prob = training_status.log_likelihood;

    while (!emAlgorithmHasConverged(training_status.iterations,
                                    training_status.log_likelihood,
                                    old_log_prob)) {
        if (cancelTrainingIfRequested()) return;
        old_log_prob = training_status.log_likelihood;
        if (!trainingError) {
            try {
                training_status.log_likelihood =
                    this->emAlgorithmUpdate(trainingSet);
            } catch (std::exception& e) {
                trainingError = true;
            }
        }

        if (std::isnan(100. *
                       fabs((training_status.log_likelihood - old_log_prob) /
                            old_log_prob)) &&
            (training_status.iterations > 1))
            trainingError = true;

        if (trainingError) {
            is_training_ = false;
            training_mutex_.unlock();
            training_status.status = TrainingEvent::Status::Error;
            training_events.notifyListeners(training_status);
            return;
        }

        ++training_status.iterations;

        if (shared_parameters->em_algorithm_max_iterations.get() >
            shared_parameters->em_algorithm_min_iterations.get())
            training_status.progression =
                float(training_status.iterations) /
                float(shared_parameters->em_algorithm_max_iterations.get());
        else
            training_status.progression =
                float(training_status.iterations) /
                float(shared_parameters->em_algorithm_min_iterations.get());

        training_events.notifyListeners(training_status);
    }

    if (cancelTrainingIfRequested()) return;
    this->emAlgorithmTerminate();

    training_mutex_.unlock();
}

bool xmm::SingleClassProbabilisticModel::emAlgorithmHasConverged(
    int step, double log_prob, double old_log_prob) const {
    if (step >= 1000) return true;
    if (shared_parameters->em_algorithm_max_iterations.get() >=
        shared_parameters->em_algorithm_min_iterations.get())
        return (step >= shared_parameters->em_algorithm_max_iterations.get());
    else
        return (step >= shared_parameters->em_algorithm_min_iterations.get()) &&
               (100. * fabs((log_prob - old_log_prob) / log_prob) <=
                shared_parameters->em_algorithm_percent_chg.get());
}

void xmm::SingleClassProbabilisticModel::emAlgorithmTerminate() {
    training_status.status = TrainingEvent::Status::Done;
    training_events.notifyListeners(training_status);
    this->is_training_ = false;
}

void xmm::SingleClassProbabilisticModel::cancelTraining() {
    if (isTraining()) {
        cancel_training_ = true;
    }
}

bool xmm::SingleClassProbabilisticModel::cancelTrainingIfRequested() {
    if (!cancel_training_) return false;
    training_mutex_.unlock();
    training_status.label = label;
    training_status.status = TrainingEvent::Status::Cancel;
    training_events.notifyListeners(training_status);
    is_training_ = false;
    return true;
}

#pragma mark -
#pragma mark Performance
void xmm::SingleClassProbabilisticModel::reset() {
    check_training();
    likelihood_buffer_.resize(shared_parameters->likelihood_window.get());
    likelihood_buffer_.clear();
}

#pragma mark -
#pragma mark File IO
Json::Value xmm::SingleClassProbabilisticModel::toJson() const {
    check_training();
    Json::Value root;
    root["label"] = label;
    return root;
}