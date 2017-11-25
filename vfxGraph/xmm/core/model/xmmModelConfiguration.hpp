/*
 * xmmModelConfiguration.hpp
 *
 * Configuration for probabilistic models with multiple classes
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

#ifndef xmmModelConfiguration_h
#define xmmModelConfiguration_h

#include "xmmModelParameters.hpp"
#include <map>

namespace xmm {
/**
 @brief Regression estimator for multiclass models
 */
enum class MultiClassRegressionEstimator {
    /**
     @brief the output is estimated as the output values of the likeliest class
     */
    Likeliest = 0,

    /**
     @brief the output is estimated as a weight sum of the output values of each
     class
     */
    Mixture = 1
};

/**
 @brief Multithreading mode for multiple-class training
 */
enum class MultithreadingMode {
    /**
     @brief No multithreading: all classes are trained sequentially
     */
    Sequential,

    /**
     @brief Multithreading: all classes are trained in parallel in different
     threads. the train
     function returns after all classes have finished training.
     */
    Parallel,

    /**
     @brief Multithreading in Background: all classes are trained in parallel in
     different threads. the train
     function returns after the training has started
     @warning when the train function return, models are still training in
     background.
     */
    Background
};

/**
 @ingroup Model
 @brief Model configuration
 @details The object contains both default parameters and class-specific
 parameters
 */
template <typename ModelType>
class Configuration : public ClassParameters<ModelType> {
  public:
    template <typename SingleClassModel, typename ModelType_>
    friend class Model;

    /**
     @brief Default Constructor
     */
    Configuration()
        : multithreading(MultithreadingMode::Parallel),
          multiClass_regression_estimator(
              MultiClassRegressionEstimator::Likeliest) {}

    /**
     @brief Copy Constructor
     @param src Source Object
     */
    Configuration(Configuration const& src)
        : ClassParameters<ModelType>(src),
          multithreading(src.multithreading),
          multiClass_regression_estimator(src.multiClass_regression_estimator),
          class_parameters_(src.class_parameters_) {}

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit Configuration(Json::Value const& root) {
        ClassParameters<ModelType>::fromJson(root["default_parameters"]);
        multithreading = static_cast<MultithreadingMode>(
            root.get("multithreading", 0).asInt());
        multiClass_regression_estimator =
            static_cast<MultiClassRegressionEstimator>(
                root.get("multiClass_regression_estimator", 0).asInt());
        class_parameters_.clear();
        for (auto p : root["class_parameters"]) {
            class_parameters_[p["label"].asString()].fromJson(p);
        }
    }

    /**
     @brief Assignment
     @param src Source Object
     */
    Configuration& operator=(Configuration const& src) {
        if (this != &src) {
            ClassParameters<ModelType>::operator=(src);
            class_parameters_ = src.class_parameters_;
            multithreading = src.multithreading;
            multiClass_regression_estimator =
                src.multiClass_regression_estimator;
        }
        return *this;
    }

    /**
     @brief access the parameters of a given class by label
     @details If the parameters have not been edited for this class yet, they
     are set to the
     default parameters.
     @param label class label
     @return a reference to the parameters of the given class
     */
    ClassParameters<ModelType>& operator[](std::string label) {
        if (class_parameters_.count(label) == 0) {
            class_parameters_[label] = *this;
        }
        return class_parameters_[label];
    }

    /**
     @brief Reset the parameters of all classes to default
     */
    void reset() { class_parameters_.clear(); }

    /**
     @brief Reset the parameters of a given classes to default
     @param label class label
     */
    void reset(std::string label) {
        if (class_parameters_.count(label) > 0) {
            class_parameters_.erase(label);
        }
    }

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    virtual Json::Value toJson() const {
        Json::Value root;
        root["multithreading"] = static_cast<int>(multithreading);
        root["multiClass_regression_estimator"] =
            static_cast<int>(multiClass_regression_estimator);
        root["default_parameters"] = ClassParameters<ModelType>::toJson();
        int i = 0;
        for (auto p : class_parameters_) {
            root["class_parameters"][i] = p.second.toJson();
            root["class_parameters"][i]["label"] = p.first;
            i++;
        }
        return root;
    }

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    virtual void fromJson(Json::Value const& root) {
        try {
            Configuration<ModelType> tmp(root);
            *this = tmp;
        } catch (JsonException& e) {
            throw e;
        }
    }

    /**
     @brief Multithreading Training Mode
     */
    MultithreadingMode multithreading;

    /**
     @brief Regression mode for multiple class (prediction from likeliest class
     vs interpolation)
     */
    MultiClassRegressionEstimator multiClass_regression_estimator;

  protected:
    /**
     @brief Parameters for each class
     */
    std::map<std::string, ClassParameters<ModelType>> class_parameters_;
};
}

#endif
