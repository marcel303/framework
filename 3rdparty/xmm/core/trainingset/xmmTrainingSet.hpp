/*
 * xmmTrainingSet.hpp
 *
 * Multimodal Training Set
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

#ifndef xmmTrainingSet_h
#define xmmTrainingSet_h

#include "xmmPhrase.hpp"
#include <map>
#include <set>
#include <memory>

namespace xmm {
/**
 @ingroup TrainingSet
 @brief Base class for the definition of training sets
 @details Training sets host a collection of phrases
 */
class TrainingSet : public Writable {
  public:
    /**
     @brief Constructor
     @param memoryMode Memory mode (owned vs shared)
     @param multimodality Number of modalities
     */
    TrainingSet(MemoryMode memoryMode = MemoryMode::OwnMemory,
                Multimodality multimodality = Multimodality::Unimodal);

    /**
     @brief Copy Constructor
     @param src source Training Set
     */
    TrainingSet(TrainingSet const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit TrainingSet(Json::Value const& root);

    /**
     @brief Assignment Operator
     @param src source Training Set
     */
    TrainingSet& operator=(TrainingSet const& src);

    /**
     @brief Destructor
     @warning phrases are only deleted if the training set is unlocked
     @see lock()
     */
    virtual ~TrainingSet();

    /** @name Accessors */
    ///@{

    /**
     @brief checks if the training set is owns the data
     @return true if the training set owns data (construction with
     MemoryMode::OwnMemory)
     */
    bool ownMemory() const;

    /**
     @brief checks if the training set is bimodal
     @return true if the training set is bimodal (construction with
     Multimodality::Bimodal)
     */
    bool bimodal() const;

    ///@}

    /** @name Manipulate Phrases */
    ///@{

    /**
     @brief checks if the training set is empty
     @return true if the training set is empty (no training phrases)
     */
    bool empty() const;

    /**
     @brief Size of the training set
     @return size of the training set (number of phrases)
     */
    unsigned int size() const;

    /**
     @brief iterator to the beginning of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::iterator begin();

    /**
     @brief iterator to the end of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::iterator end();

    /**
     @brief reverse iterator to the beginning of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::reverse_iterator rbegin();

    /**
     @brief reverse iterator to the end of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::reverse_iterator rend();

    /**
     @brief constant iterator to the beginning of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::const_iterator cbegin() const;

    /**
     @brief constant iterator to the end of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::const_iterator cend() const;

    /**
     @brief constant reverse iterator to the beginning of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::const_reverse_iterator
    crbegin() const;

    /**
     @brief constant reverse iterator to the end of phrases
     */
    std::map<int, std::shared_ptr<xmm::Phrase>>::const_reverse_iterator crend()
        const;

    /**
     @brief add a new phrase, or reset the phrase if existing
     @details The phrase is set to an empty phrase with the current attributes
     (dimensions, etc).
     The phrase is created if it does not exists at the given index.
     @param phraseIndex index of the data phrase in the trainingSet
     @param label Label of the phrase
     */
    void addPhrase(int phraseIndex, std::string label = "");

    /**
     @brief add a new phrase, or reset the phrase if existing
     @details The phrase is set to a copy of the phrase passed in argument
     The phrase is created if it does not exists at the given index.
     @param phraseIndex index of the data phrase in the trainingSet
     @param phrase source phrase
     */
    void addPhrase(int phraseIndex, Phrase const& phrase);

    /**
     @brief add a new phrase, or reset the phrase if existing
     @details The phrase is set to point to the existing phrase passed in
     argument.
     The phrase is created if it does not exists at the given index.
     @param phraseIndex index of the data phrase in the trainingSet
     @param phrase pointer to an existing phrase
     */
    void addPhrase(int phraseIndex, std::shared_ptr<Phrase> phrase);

    /**
     @brief delete a phrase
     @warning if the training set is locked, the phrase iself is not deleted
     (only the reference)
     @param phraseIndex index of the phrase
     @throws out_of_bounds if the phrase does not exist
     */
    void removePhrase(int phraseIndex);

    /**
     @brief delete all phrases of a given class
     @warning if the training set is locked, the phrases themselves are not
     deleted (only the references)
     @param label label of the class to delete
     @throws out_of_bounds if the label does not exist
     */
    void removePhrasesOfClass(std::string const& label);

    /**
     @brief Access Phrase by index
     @param phraseIndex index of the phrase
     @return pointer to the phrase if it exists, else nullptr
     */
    std::shared_ptr<xmm::Phrase> getPhrase(int phraseIndex) const;

    /**
     @brief get the pointer to the sub-training set containing all phrases with
     a given label
     @warning in order to protect the phrases in the current training set, the
     sub-training set returned is locked
     @param label target label
     @return pointer to the sub-training set containing all phrases with a given
     label
     @throws out_of_range if the label does not exist
     */
    TrainingSet* getPhrasesOfClass(std::string const& label);

    /**
     @brief delete all phrases
     @warning if the training set is locked, the phrases themselves are not
     deleted (only their references)
     */
    void clear();

    /**
     @brief get the list of labels currently in the training set
     @return reference to the set of labels in the training set
     */
    const std::set<std::string>& labels() const { return labels_; }

    ///@}

    /** @name JSON I/O */
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

    /** @name Utilities */
    ///@{

    /**
     @brief Compute the global mean of all data phrases along the time axis
     @return global mean of all phrases (along time axis, full-size)
     */
    std::vector<float> mean() const;

    /**
     @brief Compute the global standard deviation of all data phrases along the
     time axis
     @return global standard deviation of all phrases (along time axis,
     full-size)
     */
    std::vector<float> standardDeviation() const;

    /**
     @brief Compute the global min/max of all data phrases along the time axis
     @return vector of min/max pairs across all phrases (along time axis,
     full-size)
     */
    std::vector<std::pair<float, float>> minmax() const;

    /**
     @brief rescale a phrase given an offset and gain
     @param offset constant offset to be subtracted
     @param gain gain to be applied
     */
    void rescale(std::vector<float> offset, std::vector<float> gain);

    /**
     @brief normalize the training set by rescaling all phrases to the mean/std
     of the whole training set
     */
    void normalize();

    ///@}

    /**
     @brief total dimension of the training data
     */
    Attribute<unsigned int> dimension;

    /**
     @brief dimension of the input modality in bimodal mode
     */
    Attribute<unsigned int> dimension_input;

    /**
     @brief labels of the columns of the training set (e.g. descriptor names)
     */
    Attribute<std::vector<std::string>> column_names;

  protected:
    /**
     @brief Monitors the training of each Model of the group.
     */
    void onPhraseEvent(PhraseEvent const& e);

    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);

    /**
     @brief create all the sub-training sets: one for each label
     @details each subset contains only the phrase for the given label
     */
    virtual void update();

    /**
     @brief defines if the phrase has its own memory
     */
    bool own_memory_;

    /**
     @brief defines if the phrase is bimodal
     */
    bool bimodal_;

    /**
     @brief Set containing all the labels present in the training set
     */
    std::set<std::string> labels_;

    /**
     @brief Training Phrases
     @details Phrases are stored in a map: allows the easy addition/deletion of
     phrases by index.
     */
    std::map<int, std::shared_ptr<Phrase>> phrases_;

    /**
     @brief Sub-ensembles of the training set for specific classes
     */
    std::map<std::string, TrainingSet> sub_training_sets_;
};
}

#endif