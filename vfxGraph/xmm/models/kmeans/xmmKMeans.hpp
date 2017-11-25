/*
 * xmmKMeans.hpp
 *
 * K-Means clustering
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

#ifndef xmmKMeans_h
#define xmmKMeans_h

#include "../../core/model/xmmModelConfiguration.hpp"
#include "../../core/model/xmmModelSingleClass.hpp"
#include "xmmKMeansParameters.hpp"
#include "xmmKMeansResults.hpp"

namespace xmm {
/**
 @ingroup KMeans
 @brief K-Means Clustering algorithm
 */
class KMeans : public Writable {
  public:
    static const unsigned int DEFAULT_MAX_ITERATIONS = 50;
    static const float DEFAULT_RELATIVE_VARIATION_THRESHOLD() { return 1e-20; }

    /**
     @brief Type of initizalization of the K-Means algorithm
     */
    enum class InitializationMode {
        /**
         @brief random initialization (scaled using training set variance)
         */
        Random,

        /**
         @brief biased initialization: initialiazed with the first phrase
         */
        Biased
    };

    /**
     @brief Default Constructor
     @param clusters number of clusters
     */
    KMeans(unsigned int clusters = 1);

    /**
     @brief Copy Constructor
     @param src Source Model
     */
    KMeans(KMeans const& src);

    /**
     @brief Assignment
     @param src Source Model
     */
    KMeans& operator=(KMeans const& src);

    /**
     @brief Train the K-Means clutering from the given training set
     @param trainingSet Training Set
     */
    void train(TrainingSet* trainingSet);

    /**
     @brief Resets the fitering process (cluster association)
     */
    void reset();

    /**
     @brief filters a incoming observation (performs cluster association)
     @details the results of the inference process are stored in the results
     attribute
     @param observation observation vector
     */
    void filter(std::vector<float> const& observation);

    /** @name Json I/O */
    ///@{

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    Json::Value toJson() const;

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    void fromJson(Json::Value const& root);

    ///@}

    /**
     @brief Set of Parameters shared among classes
     */
    std::shared_ptr<SharedParameters> shared_parameters;

    /**
     @brief Configuration (default and class-specific parameters)
     */
    Configuration<KMeans> configuration;

    /**
     @brief Results of the cluster association after update with an observation
     */
    Results<KMeans> results;

    /**
     @brief Clusters centers
     */
    std::vector<float> centers;

    /**
     @brief Type of initialization for the K-Means Algorithm
     */
    KMeans::InitializationMode initialization_mode;

  protected:
    /**
     @brief randomzie Cluster Centers (normalized width data variance)
     of the first phrase of the training set
     */
    void randomizeClusters(std::vector<float> const& trainingSetVariance);

    /**
     @brief Initialize the clusters using a regular segmentation
     of the first phrase of the training set
     */
    void initClustersWithFirstPhrase(std::shared_ptr<Phrase> phrase);

    /**
     @brief Update method for training
     @details computes the cluster associated with each data points, and update
     Cluster centers
     */
    void updateCenters(std::vector<float>& previous_centers,
                       TrainingSet* trainingSet);
};

/**
 @brief Simple Euclidian distance measure
 @param vector1 first data point
 @param vector2 first data point
 @param dimension dimension of the data space
 @return euclidian distance between the 2 points
 */
template <typename T>
T euclidean_distance(const T* vector1, const T* vector2, unsigned int dimension);
}

#endif
