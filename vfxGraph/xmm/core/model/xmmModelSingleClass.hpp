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

#ifndef xmmModelSingleClass_h
#define xmmModelSingleClass_h

#include "../common/xmmCircularbuffer.hpp"
#include "../common/xmmEvents.hpp"
#include "../trainingset/xmmTrainingSet.hpp"
#include "xmmModelSharedParameters.hpp"
#include <memory>
#include <mutex>

namespace xmm {
/**
 @ingroup Model
 @brief Event for monitoring the training process
 */
class TrainingEvent {
  public:
    /**
     @brief Status of the training process
     */
    enum Status {
        /**
         @brief Training is still running
         */
        Run,

        /**
         @brief An error occured during training
         */
        Error,

        /**
         @brief The training has been cancelled.
         */
        Cancel,

        /**
         @brief Training is done without error
         */
        Done,

        /**
         @brief The training of all classes has finished
         */
        Alldone
    };

    /**
     @brief Type of Training Error
     */
    enum class ErrorType {
        /**
         @brief Convergence Errors (numerical errors in the training process).
         @details these errors can be due to issues with the training data (e.g.
         nan values), or
         bad choice of parameters (e.g. regularization too low).
         */
        ConvergenceError
    };

    /**
     @brief Constructor
     @param model_ Source model
     @param label_ Label of the model sending the notification
     @param status_ status of the training process
     */
    TrainingEvent(void* model_, std::string label_,
                  Status status_ = Status::Run)
        : model(model_),
          label(label_),
          status(status_),
          progression(0),
          log_likelihood(0),
          iterations(0) {}

    /**
     @brief Copy constructor
     @param src source event
     */
    TrainingEvent(TrainingEvent const& src)
        : model(src.model),
          label(src.label),
          status(src.status),
          progression(src.progression),
          log_likelihood(src.log_likelihood),
          iterations(src.iterations) {}

    /**
     @brief Assignment operator
     @param src source event
     */
    TrainingEvent& operator=(TrainingEvent const& src) {
        if (this != &src) {
            model = src.model;
            label = src.label;
            status = src.status;
            progression = src.progression;
            log_likelihood = src.log_likelihood;
            iterations = src.iterations;
        }
        return *this;
    }

    /**
     @brief Source Model
     */
    void* model;

    /**
     @brief Label of the source model, if any.
     */
    std::string label;

    /**
     @brief Status of the training process
     */
    Status status;

    /**
     @brief progression within the training algorithm
     */
    float progression;

    /**
     @brief Log-likelihood of the data given the model's parameters at the en of
     training
     */
    double log_likelihood;

    /**
     @brief Number of EM iterations
     */
    double iterations;
};

/**
 @ingroup Model
 @brief Generic Template for Machine Learning Probabilistic models based on the
 EM algorithm
 */
class SingleClassProbabilisticModel : public Writable {
  public:
    /**
     @brief Constructor
     @param p pointer to a shared parameters object (owned by a Model)
     */
    SingleClassProbabilisticModel(std::shared_ptr<SharedParameters> p = NULL);

    /**
     @brief Copy Constructor
     @param src Source Model
     */
    SingleClassProbabilisticModel(SingleClassProbabilisticModel const& src);

    /**
     @brief Constructor from Json
     @param p pointer to a shared parameters object (owned by a Model)
     @param root Json structure
     */
    explicit SingleClassProbabilisticModel(std::shared_ptr<SharedParameters> p,
                                           Json::Value const& root);

    /**
     @brief Assignment
     @param src Source Model
     */
    SingleClassProbabilisticModel& operator=(
        SingleClassProbabilisticModel const& src);

    /**
     @brief Destructor
     */
    virtual ~SingleClassProbabilisticModel();

    /** @name Training */
    ///@{

    /**
     @brief Checks if the model is training
     @return true if the model is training
     */
    bool isTraining() const;

    /**
     @brief Main training method based on the EM algorithm
     @details the method performs a loop over the pure virtual method
     emAlgorithmUpdate() until convergence.
     The @a emAlgorithmUpdate method computes both E and M steps of the EM
     algorithm.
     @param trainingSet Training Set to train the model.
     @see emAlgorithmUpdate
     */
    void train(TrainingSet* trainingSet);

    /**
     @brief Cancels the training process : sets a flag so that the training
     stops at the next
     possible exit in the training process.
     @warning the model is still training when this function returns. This
     function only requests the training
     process to cancel.
     */
    void cancelTraining();

    ///@}

    /** @name Performance */
    ///@{

    /**
     @brief Resets the fitering process (recognition or regression)
     */
    virtual void reset();

    /**
     @brief filters a incoming observation (performs recognition or regression)
     @details the results of the inference process are stored in the results
     attribute
     @param observation observation vector
     @return likelihood of the observation
     */
    virtual double filter(std::vector<float> const& observation) = 0;

    ///@}

    /** @name Json I/O */
    ///@{

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    virtual Json::Value toJson() const;

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    virtual void fromJson(Json::Value const& root) = 0;

    ///@}

    /**
     @brief label associated with the given model
     */
    std::string label;

    /**
     @brief Pointer to the shared parameters owned by a multi-class model
     */
    std::shared_ptr<SharedParameters> shared_parameters;

    /**
     @brief Generator for events monitoring the training process
     */
    EventGenerator<TrainingEvent> training_events;

    /**
     @brief Event containing information on the current status of the training
     process
     */
    TrainingEvent training_status;

  protected:
    /**
     @brief Allocate memory for the model's parameters
     @details called when dimensions are modified
     */
    virtual void allocate() = 0;

    /**
     @brief Initialize the training algorithm
     */
    virtual void emAlgorithmInit(TrainingSet* trainingSet) = 0;

    /**
     @brief Update Method of the EM algorithm
     @details performs E and M steps of the EM algorithm.
     @return likelihood of the training data given the current model parameters
     (before re-estimation).
     */
    virtual double emAlgorithmUpdate(TrainingSet* trainingSet) = 0;

    /**
     @brief Terminate the training algorithm
     */
    virtual void emAlgorithmTerminate();

    /**
     @brief checks if the training has converged according to the object's EM
     stop criterion
     @param step index of the current step of the EM algorithm
     @param log_prob log-likelihood returned by the EM update
     @param old_log_prob log-likelihood returned by the EM update at the
     previous step
     */
    bool emAlgorithmHasConverged(int step, double log_prob,
                                 double old_log_prob) const;

    /**
     @brief checks if a cancel request has been sent and accordingly cancels the
     training process
     @return true if the training has been canceled.
     */
    bool cancelTrainingIfRequested();

    /**
     @brief Checks if the model is still training
     @throws runtime_error if the model is training.
     */
    inline void check_training() const {
        if (this->isTraining())
            throw std::runtime_error("The model is training");
    }

    /**
     @brief Likelihood buffer used for smoothing
     */
    CircularBuffer<double> likelihood_buffer_;

    /**
     @brief Mutex used in Concurrent Mode
     */
    std::mutex training_mutex_;

    /**
     @brief defines if the model is being trained.
     */
    bool is_training_;

    /**
     @brief defines if the model received a request to cancel training
     */
    bool cancel_training_;
};
}

#endif
