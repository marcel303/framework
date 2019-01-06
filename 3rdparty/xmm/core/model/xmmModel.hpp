/*
 * xmmModel.hpp
 *
 * Probabilistic machine learning model for multiclass recognition and
 * regression
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

#ifndef xmmModel_h
#define xmmModel_h

#include "xmmModelConfiguration.hpp"
#include "xmmModelResults.hpp"
#include "xmmModelSingleClass.hpp"
#include <atomic>
#include <thread>

namespace xmm {
/**
 @ingroup Model
 @brief Probabilistic machine learning model for multiclass recognition and
 regression
 @tparam SingleClassModel Type of the associated Single-class Model
 @tparam ModelType Model Type Name
 */
template <typename SingleClassModel, typename ModelType>
class Model : public Writable {
  public:
    /**
     @brief Constructor
     @param bimodal use true for a use with Regression / Generation.
     */
    Model(bool bimodal = false)
        : shared_parameters(std::make_shared<SharedParameters>()),
          cancel_required_(false),
          is_training_(false),
          is_joining_(false),
          models_still_training_(0) {
        shared_parameters->bimodal.set(bimodal);
        if (shared_parameters->bimodal.get()) {
            shared_parameters->dimension.set(2, true);
            shared_parameters->dimension_input.set(1, true);
        } else {
            shared_parameters->dimension.set(1, true);
            shared_parameters->dimension_input.set(0, true);
        }
    }

    /**
     @brief Copy Constructor
     @param src Source Model
     */
    Model(Model<SingleClassModel, ModelType> const& src)
        : shared_parameters(
              std::make_shared<SharedParameters>(*src.shared_parameters)),
          configuration(src.configuration),
          training_events(src.training_events),
          cancel_required_(false),
          is_training_(false),
          is_joining_(false),
          models_still_training_(0) {
        if (src.is_training_)
            throw std::runtime_error(
                "Cannot copy: source model is still training");
        models = src.models;
        for (auto& model : models) {
            model.second.training_events.removeListeners();
            model.second.training_events.addListener(
                this,
                &xmm::Model<SingleClassModel, ModelType>::onTrainingEvent);
        }
    }

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit Model(Json::Value const& root)
        : shared_parameters(std::make_shared<SharedParameters>()),
          cancel_required_(false),
          is_training_(false),
          is_joining_(false),
          models_still_training_(0) {
        shared_parameters->fromJson(root["shared_parameters"]);
        configuration.fromJson(root["configuration"]);
        models.clear();
        for (auto p : root["models"]) {
            std::string l = p["label"].asString();
            models.insert(std::pair<std::string, SingleClassModel>(
                l, SingleClassModel(shared_parameters, p)));
            models[l].training_events.removeListeners();
            models[l].training_events.addListener(
                this,
                &xmm::Model<SingleClassModel, ModelType>::onTrainingEvent);
        }
    }

    /**
     @brief Assignment
     @param src Source Model
     */
    Model<SingleClassModel, ModelType>& operator=(
        Model<SingleClassModel, ModelType> const& src) {
        if (this != &src) {
            if (is_training_)
                throw std::runtime_error(
                    "Cannot copy: target model is still training");
            if (src.is_training_)
                throw std::runtime_error(
                    "Cannot copy: source model is still training");
            shared_parameters =
                std::make_shared<SharedParameters>(*src.shared_parameters);
            configuration = src.configuration;
            training_events = src.training_events;
            is_joining_ = false;
            is_training_ = false;
            cancel_required_ = false;
            models_still_training_ = 0;

            models.clear();
            models = src.models;
            for (auto& model : this->models) {
                model.second.training_events.removeListeners();
                model.second.training_events.addListener(
                    this,
                    &xmm::Model<SingleClassModel, ModelType>::onTrainingEvent);
            }
        }
        return *this;
    }

    /**
     @brief Destructor
     */
    virtual ~Model() {
        cancelTraining();
        clear();
    }

    /** @name Class Manipulation */
    ///@{

    /**
     @brief Get the number of classes in the model
     @return the number of classes in the model
     */
    unsigned int size() const {
        return static_cast<unsigned int>(models.size());
    }

    /**
     @brief Checks if a class exists
     @param label class label
     @return true if the class labeled 'label' exists
     */
    bool hasClass(std::string const& label) const {
        checkTraining();
        return (models.count(label) > 0);
    }

    /**
     @brief Checks if a class exists
     @param label class label
     @return true if the class labeled 'label' exists
     */
    int getIndex(std::string const& label) const {
        checkTraining();
        int i(-1);
        for (auto& m : models) {
            i++;
            if (m.first == label) {
                return i;
            }
        }
        return -1;
    }

    /**
     @brief Remove a specific class by label
     @param label label of the class to remove
     */
    virtual void removeClass(std::string const& label) {
        while (is_joining_) {
        }
        cancelTraining(label);
        auto it = models.find(label);
        if (it == models.end())
            throw std::out_of_range("Class " + label + " does not exist");
        models.erase(it);
        reset();
    }

    /**
     @brief Remove all classes
     */
    virtual void clear() {
        if (is_training_) cancelTraining();
        models.clear();
        reset();
    }

    ///@}

    /** @name Training */
    ///@{

    /**
     @brief Checks if the model is trained (training finished and not empty)
     */
    bool trained() const { return (!is_training_ && size() > 0); }

    /**
     @brief Checks if the model is still training
     */
    bool training() const { return is_training_; }

    /**
     @brief Train all classes from the training set passed in argument
     @param trainingSet Training Set
     */
    virtual void train(TrainingSet* trainingSet) {
        if (!trainingSet) return;
        cancelTraining();
        clear();

        is_training_ = true;

        // Fetch training set parameters
        shared_parameters->dimension.set(trainingSet->dimension.get());
        if (shared_parameters->bimodal.get()) {
            shared_parameters->dimension_input.set(
                trainingSet->dimension_input.get());
        }
        shared_parameters->column_names.set(trainingSet->column_names.get());

        // Update models
        bool contLoop(true);
        while (contLoop) {
            contLoop = false;
            for (auto it = models.begin(); it != models.end(); ++it) {
                if (trainingSet->labels().count(it->first) == 0) {
                    models.erase(it->first);
                    contLoop = true;
                    break;
                }
            }
        }
        for (typename std::set<std::string>::iterator it =
                 trainingSet->labels().begin();
             it != trainingSet->labels().end(); ++it) {
            addModelForClass(*it);
        }

        // Start class training
        for (auto it = this->models.begin(); it != this->models.end(); ++it) {
            it->second.is_training_ = true;
            it->second.cancel_training_ = false;
            models_still_training_++;
            if ((configuration.multithreading ==
                 MultithreadingMode::Parallel) ||
                (configuration.multithreading ==
                 MultithreadingMode::Background)) {
                training_threads_[it->first] =
                    std::thread(&SingleClassModel::train, &it->second,
                                trainingSet->getPhrasesOfClass(it->first));
            } else {
                it->second.train(trainingSet->getPhrasesOfClass(it->first));
            }
        }
        if (configuration.multithreading == MultithreadingMode::Parallel) {
            joinTraining();
        }
        if (configuration.multithreading == MultithreadingMode::Sequential) {
            is_training_ = false;
        }
    }

    /**
     @brief Train a specific class from the training set passed in argument
     @param trainingSet Training Set
     @param label label of the class to train
     @throw out_of_range if the label does not exist
     @throw runtime_error if the dimensions of the training set don't match the
     dimensions
     of the current model
     */
    virtual void train(TrainingSet* trainingSet, std::string const& label) {
        if (!trainingSet) return;
        if (trainingSet->labels().count(label) == 0)
            throw std::out_of_range("Class " + label + " does not exist");

        // Cancel Training of the class
        cancelTraining(label);

        // Check Consistency of the new Training Set
        if (size() > 0) {
            if ((shared_parameters->dimension.get() !=
                 trainingSet->dimension.get()) ||
                (shared_parameters->bimodal.get() &&
                 (shared_parameters->dimension_input.get() !=
                  trainingSet->dimension_input.get())))
                throw std::runtime_error(
                    "Dimensions of the new Training Set do not match existing "
                    "models");
        }

        is_training_ = true;

        // Fetch training set parameters
        shared_parameters->dimension.set(trainingSet->dimension.get());
        if (shared_parameters->bimodal.get()) {
            shared_parameters->dimension_input.set(
                trainingSet->dimension_input.get());
        }
        shared_parameters->column_names.set(trainingSet->column_names.get());

        addModelForClass(label);

        // Start class training
        models[label].is_training_ = true;
        models[label].cancel_training_ = false;
        models_still_training_++;
        if (configuration.multithreading == MultithreadingMode::Sequential) {
            models[label].train(trainingSet->getPhrasesOfClass(label));
        } else {
            training_threads_[label] =
                std::thread(&SingleClassModel::train, &(this->models[label]),
                            trainingSet->getPhrasesOfClass(label));
        }
        if (configuration.multithreading == MultithreadingMode::Parallel) {
            joinTraining();
        }
        if (configuration.multithreading == MultithreadingMode::Sequential) {
            is_training_ = false;
        }
    }

    /**
     @brief Cancels the training of all models
     */
    void cancelTraining() {
        if (is_training_) {
            for (auto& it : this->models) {
                cancel_required_ = true;
                it.second.cancelTraining();
                while (it.second.isTraining()) {
                }
            }
            joinTraining();
        }
        cancel_required_ = false;
    }

    /**
     @brief Cancels the training of a given class
     @param label label of the class to cancel
     */
    void cancelTraining(std::string const& label) {
        if (is_training_ && (this->models.count(label) > 0)) {
            cancel_required_ = true;
            models[label].cancelTraining();
            while (models[label].isTraining()) {
            }
            if (models_still_training_ == 0) {
                joinTraining();
            }
        }
        cancel_required_ = false;
    }

    ///@}

    /** @name Performance */
    ///@{

    /**
     @brief Resets the fitering process (recognition or regression)
     */
    virtual void reset() {
        checkTraining();
        // checkConfigurationChanges();
        for (auto it = this->models.begin(); it != this->models.end(); ++it) {
            it->second.reset();
        }
    }

    /**
     @brief filters a incoming observation (performs recognition or regression)
     @details the results of the inference process are stored in the results
     attribute
     @param observation observation vector
     */
    virtual void filter(std::vector<float> const& observation) {
        checkTraining();
        // checkConfigurationChanges();
    }

    ///@}

    /** @name Json I/O */
    ///@{

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    virtual Json::Value toJson() const {
        Json::Value root;
        root["shared_parameters"] = shared_parameters->toJson();
        root["configuration"] = configuration.toJson();
        root["models"].resize(static_cast<Json::ArrayIndex>(size()));
        Json::ArrayIndex modelIndex(0);
        for (auto& it : models) {
            root["models"][modelIndex++] = it.second.toJson();
        }
        return root;
    }

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    void fromJson(Json::Value const& root) {
        try {
            EventGenerator<TrainingEvent> _tmp_training_events(training_events);
            Model<SingleClassModel, ModelType> tmp(root);
            *this = tmp;
            training_events = _tmp_training_events;
        } catch (JsonException& e) {
            throw e;
        }
    }

    ///@}

    /**
     @brief Set of Parameters shared among classes
     */
    std::shared_ptr<SharedParameters> shared_parameters;

    /**
     @brief Configuration (default and class-specific parameters)
     */
    Configuration<ModelType> configuration;

    /**
     @brief Generator for training process events
     */
    EventGenerator<TrainingEvent> training_events;

    /**
     @brief models stored in a map. Each Model is associated with a label
     */
    std::map<std::string, SingleClassModel> models;

  protected:
    /**
     @brief Finishes the background training process by joining threads and
     deleting the models which training failed
     */
    virtual void joinTraining() {
        if (!is_training_) return;
        bool expectedState(false);
        if (is_joining_.compare_exchange_weak(expectedState, true)) {
            while (!training_threads_.empty()) {
                training_threads_.begin()->second.join();
                training_threads_.erase(training_threads_.begin());
            }
            bool classesUntrained(true);
            while (classesUntrained) {
                classesUntrained = false;
                for (auto it = this->models.begin(); it != this->models.end();
                     it++) {
                    if (it->second.training_status.status ==
                            TrainingEvent::Status::Cancel ||
                        it->second.training_status.status ==
                            TrainingEvent::Status::Error) {
                        models.erase(it);
                        classesUntrained = true;
                        break;
                    }
                }
            }
            is_joining_ = false;
            is_training_ = false;
            reset();
            TrainingEvent event(this, "", TrainingEvent::Status::Alldone);
            training_events.notifyListeners(event);
        } else {
            while (is_joining_) {
            }
        }
    }

    /**
     @brief Monitors the training of each Model of the group.
     */
    void onTrainingEvent(TrainingEvent const& e) {
        event_mutex_.lock();
        TrainingEvent event(e);
        event.model = this;
        training_events.notifyListeners(event);
        if (e.status != TrainingEvent::Status::Run) {
            models_still_training_--;
            ((SingleClassModel*)e.model)->is_training_ = false;
            if (configuration.multithreading ==
                    MultithreadingMode::Background &&
                !is_joining_ &&       // avoid to call "joinTraining" if already
                                      // called by the main thread
                !cancel_required_ &&  // avoid to call "joinTraining" if cancel
                                      // required by the main thread
                models_still_training_ == 0) {
                is_joining_ = false;
                std::thread(&Model<SingleClassModel, ModelType>::joinTraining,
                            this)
                    .detach();
            }
        }
        event_mutex_.unlock();
    }

    /**
     @brief Checks if the Model is still training
     @throws runtime_error if the Model is training.
     */
    inline void checkTraining() const {
        if (is_training_) throw std::runtime_error("The Model is training");
        while (is_joining_) {
        }
    }

    /**
     @brief Look for configuration changes and throws an exception if the model
     is not up to date.
     */
    inline void checkConfigurationChanges() const {
        bool configChanged(false);
        if (configuration.changed) configChanged = true;
        for (auto& config : configuration.class_parameters_) {
            if (config.second.changed) configChanged = true;
        }
        if (configChanged) {
            throw std::runtime_error(
                "Configuration has changed, models need to be trained.");
        }
    }

    /**
     @brief Update training set for a specific label
     @param label label of the sub-training set to update
     @throws out_of_range if the label does not exist
     */
    virtual void addModelForClass(std::string const& label) {
        if (models.count(label) > 0 && models[label].isTraining())
            throw std::runtime_error("The Model is already training");

        if (models.find(label) == models.end()) {
            models.insert(std::pair<std::string, SingleClassModel>(
                label, shared_parameters));
            models[label].training_events.addListener(
                this,
                &xmm::Model<SingleClassModel, ModelType>::onTrainingEvent);
            models[label].label = label;
        }
        if (configuration.class_parameters_.count(label) > 0) {
            models[label].parameters = configuration.class_parameters_[label];
        } else {
            models[label].parameters = configuration;
        }
    }

    /**
     @brief Training Threads
     */
    std::map<std::string, std::thread> training_threads_;

    /**
     @brief locks the Model while the models are training
     Used in cancel method
     */
    std::atomic<bool> cancel_required_;

    /**
     @brief locks the Model while the models are training
     Used in cancel method
     */
    std::atomic<bool> is_training_;

    /**
     @brief specifies if a thread for joining the training process has been
     launched.
     */
    std::atomic<bool> is_joining_;

    /**
     @brief Number of models that are still training
     */
    unsigned int models_still_training_;

    /**
     @brief Mutex that prevents concurrent calls to onEvent()
     */
    std::mutex event_mutex_;
};
}

#endif
