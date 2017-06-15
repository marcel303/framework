/*
 * xmmPhrase.hpp
 *
 * Multimodal data phrase
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

#ifndef xmmPhrase_h
#define xmmPhrase_h

#include "../common/xmmAttribute.hpp"
#include "../common/xmmEvents.hpp"
#include "../common/xmmJson.hpp"
#include <cmath>

namespace xmm {
/**
 @defgroup TrainingSet [Core] Training Sets
 */

/**
 @ingroup TrainingSet
 @brief Type of memory management for training sets and phrases
 */
enum class MemoryMode {
    /**
     @brief memory is owned by the Phrase container.
     @details Phrases can be recorded directly
     */
    OwnMemory,

    /**
     @brief memory is shared with other data structures
     @details Phrases can be 'connected' to external data arrays
     */
    SharedMemory
};

/**
 @ingroup TrainingSet
 @brief Number of modalities in the data phrase
 */
enum class Multimodality {
    /**
     @brief single modality (i.e. 1 data array)
     @details can be used for recognition
     */
    Unimodal,

    /**
     @brief two modalities (i.e. 2 data arrays)
     @details can be used for regression (input/output)
     */
    Bimodal
};

class Phrase;

/**
 @ingroup TrainingSet
 @brief Event that can be thrown by a phrase to a training set
 */
class PhraseEvent {
  public:
    /**
     @brief Type of event
     */
    enum class Type {
        /**
         @brief Thrown when the label if the phrase is modified.
         */
        LabelChanged,
    };

    /**
     @brief Default constructor
     @param phrase_ pointer to the source phrase
     @param type_ type of event
     */
    PhraseEvent(Phrase* phrase_, Type type_) : phrase(phrase_), type(type_) {}

    /**
     @brief Copy constructor
     @param src source event
     */
    PhraseEvent(PhraseEvent const& src) : phrase(src.phrase), type(src.type) {}

    /**
     @brief pointer to the source phrase
     */
    Phrase* phrase;

    /**
     @brief Type of event
     */
    Type type;
};

/**
 @ingroup TrainingSet
 @brief Data phrase
 @details The Phrase class can be used to store unimodal and Bimodal data
 phrases.
 It can have an autonomous memory, or this memory can be shared with another
 data
 container. These attributes are specified at construction.
 */
class Phrase : public Writable {
  public:
    friend class TrainingSet;

    /**
     @brief Constructor
     @param memoryMode Memory mode (owned vs shared)
     @param multimodality Number of modalities
     */
    Phrase(MemoryMode memoryMode = MemoryMode::OwnMemory,
           Multimodality multimodality = Multimodality::Unimodal);

    /**
     @brief Copy Constructor
     @param src source Phrase
     */
    Phrase(Phrase const& src);

    /**
     @brief Constructor from Json Structure
     @param root Json Value
     */
    explicit Phrase(Json::Value const& root);

    /**
     @brief Assignment
     @param src source Phrase
     */
    Phrase& operator=(Phrase const& src);

    /**
     @brief Destructor.
     @details Data is only deleted if the memory is owned (construction with
     MemoryMode::OwnMemory)
     */
    virtual ~Phrase();

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

    /** @name Access Data */
    ///@{

    /**
     @brief get the number of frames in the phrase
     @return the number of frames in the phrase
     */
    unsigned int size() const;

    /**
     @brief get the number of frames in the input array of the phrase
     @return the number of frames in the input array of the phrase
     */
    unsigned int inputSize() const;

    /**
     @brief get the number of frames in the output array of the phrase
     @return the number of frames in the output array of the phrase
     */
    unsigned int outputSize() const;

    /**
     @brief check if the phrase is empty
     @return true if the phrase is empty (length 0)
     */
    bool empty() const;

    /**
     @brief Access data at a given time index and dimension.
     @param index time index
     @param dim dimension considered, indexed from 0 to the total dimension of
     the data across modalities
     @throws out_of_range if time index or dimension are out of bounds
     */
    float getValue(unsigned int index, unsigned int dim) const;

    /**
     @brief Get pointer to the data at a given time index
     @param index time index
     @warning  this method can be used only for unimodal phrases (construction
     with Multimodality::Unimodal)
     @throws out_of_range if time index is out of bounds
     @throws runtime_error if the phrase is bimodal (construction with
     Multimodality::Bimodal)
     @return pointer to the data array of the modality, for the given time index
     */
    float* getPointer(unsigned int index) const;

    /**
     @brief Get pointer to the data at a given time index for the input modality
     @warning  this method can be used only for bimodal phrases (construction
     with Multimodality::Bimodal)
     @param index time index
     @throws out_of_range if time index is out of bounds
     @throws runtime_error if the phrase is unimodal (construction with
     Multimodality::Unimodal)
     @return pointer to the data array of the modality, for the given time index
     */
    float* getPointer_input(unsigned int index) const;

    /**
     @brief Get pointer to the data at a given time index for the output
     modality
     @warning  this method can be used only for bimodal phrases (construction
     with Multimodality::Bimodal)
     @param index time index
     @throws out_of_range if time index is out of bounds
     @throws runtime_error if the phrase is unimodal (construction with
     Multimodality::Unimodal)
     @return pointer to the data array of the modality, for the given time index
     */
    float* getPointer_output(unsigned int index) const;

    ///@}

    /** @name Connect (MemoryMode = SharedData) */
    ///@{

    /**
     @brief Connect a unimodal phrase to a shared container
     @warning This method is only usable in Shared Memory (construction with
     MemoryMode::SharedMemory)
     @param pointer_to_data pointer to the data array
     @param length length of the data array
     @throws runtime_error if data is owned (construction with
     MemoryMode::OwnMemory flag)
     */
    void connect(float* pointer_to_data, unsigned int length);

    /**
     @brief Connect a Bimodal phrase to a shared container
     @warning This method is only usable in Shared Memory (construction with
     MemoryMode::SharedMemory)
     @param pointer_to_data_input pointer to the data array of the input
     modality
     @param pointer_to_data_output pointer to the data array of the output
     modality
     @param length length of the data array
     @throws runtime_error if data is owned (construction with
     MemoryMode::OwnMemory flag)
     */
    void connect(float* pointer_to_data_input, float* pointer_to_data_output,
                 unsigned int length);

    /**
     @brief Connect a Bimodal phrase to a shared container for the input
     modality
     @warning This method is only usable in Shared Memory (construction with
     MemoryMode::SharedMemory)
     @param pointer_to_data pointer to the data array of the input modality
     @param length length of the data array
     @throws runtime_error if data is owned (construction with
     MemoryMode::OwnMemory flag)
     */
    void connect_input(float* pointer_to_data, unsigned int length);

    /**
     @brief Connect a Bimodal phrase to a shared container for the output
     modality
     @warning This method is only usable in Shared Memory (construction with
     MemoryMode::SharedMemory)
     @param pointer_to_data pointer to the data array of the output modality
     @param length length of the data array
     @throws runtime_error if data is owned (construction with
     MemoryMode::OwnMemory flag)
     */
    void connect_output(float* pointer_to_data, unsigned int length);

    /**
     @brief Disconnect a phrase from a shared container
     @warning This method is only usable in Shared Memory (construction with
     MemoryMode::SharedMemory)
     @throws runtime_error if data is owned (construction with
     MemoryMode::OwnMemory flag)
     */
    void disconnect();

    ///@}

    /** @name Record (MemoryMode = OwnData) */
    ///@{

    /**
     @brief Record observation
     @details Appends the observation vector observation to the data array.\n
     This method is only usable in Own Memory (construction with
     MemoryMode::OwnMemory)
     @param observation observation vector (C-like array which must have the
     size of the total
     dimension of the data across all modalities)
     @throws runtime_error if data is shared (construction with
     MemoryMode::SharedMemory flag)
     */
    void record(std::vector<float> const& observation);

    /**
     @brief Record observation on input modality
     Appends the observation vector observation to the data array\n
     This method is only usable in Own Memory (construction with
     MemoryMode::OwnMemory)
     @param observation observation vector (C-like array which must have the
     size of the total
     dimension of the data across all modalities)
     @throws runtime_error if data is shared (construction with
     MemoryMode::SharedMemory flag)
     */
    void record_input(std::vector<float> const& observation);

    /**
     @brief Record observation on output modality
     Appends the observation vector observation to the data array\n
     This method is only usable in Own Memory (construction with
     MemoryMode::OwnMemory)
     @param observation observation vector (C-like array which must have the
     size of the total
     dimension of the data across all modalities)
     @throws runtime_error if data is shared (construction with
     MemoryMode::SharedMemory flag)
     */
    void record_output(std::vector<float> const& observation);

    /**
     @brief Reset length of the phrase to 0 ==> empty phrase\n
     This method is only usable in Own Memory (construction with
     MemoryMode::OwnMemory)
     @throws runtime_error if data is shared (construction with
     MemoryMode::SharedMemory flag)
     @warning the memory is not released (only done in destructor).
     */
    void clear();
    void clearInput();
    void clearOutput();

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
     @brief Compute the mean of the data phrase along the time axis
     @return mean of the phrase (along time axis, full-size)
     */
    std::vector<float> mean() const;

    /**
     @brief Compute the standard deviation of the data phrase along the time
     axis
     @return standard deviation of the phrase (along time axis, full-size)
     */
    std::vector<float> standardDeviation() const;

    /**
     @brief Compute the global min/max of the data phrase along the time axis
     @return vector of min/max pairs ofthe phrases (along time axis, full-size)
     */
    std::vector<std::pair<float, float>> minmax() const;

    /**
     @brief rescale a phrase given an offset and gain
     @param offset constant offset to be subtracted
     @param gain gain to be applied
     */
    void rescale(std::vector<float> offset, std::vector<float> gain);

    ///@}

    /**
     @brief Total dimension of the phrase
     */
    Attribute<unsigned int> dimension;

    /**
     @brief Used in bimodal mode: dimension of the input modality.
     */
    Attribute<unsigned int> dimension_input;

    /**
     @brief Main label of the phrase
     */
    Attribute<std::string> label;

    /**
     @brief labels of the columns of the phrase (e.g. descriptor names)
     */
    std::vector<std::string> column_names;

  protected:
    /**
     @brief trim phrase to minimal length of modalities
     */
    void trim();

    /**
     @brief Memory Allocation
     @details used record mode (no SHARED_MEMORY flag), the data vector is
     reallocated
     with a block size ALLOC_BLOCKSIZE
     */
    void reallocateLength();

    /**
     @brief notification function called when a member attribute is changed
     */
    virtual void onAttributeChange(AttributeBase* attr_pointer);

    static const unsigned int AllocationBlockSize = 256;

    /**
     @brief Defines if the phrase stores the data itself.
     */
    bool own_memory_;

    /**
     @brief Defines if the phrase is bimodal (true) or unimodal (false)
     */
    bool bimodal_;

    /**
     @brief true if the phrase does not contain any data
     */
    bool empty_;

    /**
     @brief Length of the phrase. If bimodal, it is the minimal length between
     modalities
     */
    unsigned int length_;

    /**
     @brief Length of the array of the input modality
     */
    unsigned int input_length_;

    /**
     @brief Length of the array of the output modality
     */
    unsigned int output_length_;

    /**
     @brief Allocated length (only used in own memory mode)
     */
    unsigned int max_length_;

    /**
     @brief Pointer to the Data arrays
     @details data has a size 1 in unimodal mode, 2 in bimodal mode.
     */
    float** data_;

    EventGenerator<PhraseEvent> events;
};

/**
 @brief Reallocate a C-like array (using c++ std::copy)
 @param src source array
 @param dim_src initial dimension
 @param dim_dst target dimension
 @return resized array (content is conserved)
 */
template <typename T>
T* reallocate(T* src, unsigned int dim_src, unsigned int dim_dst) {
    T* dst = new T[dim_dst];

    if (!src) return dst;

    if (dim_dst > dim_src) {
        std::copy(src, src + dim_src, dst);
    } else {
        std::copy(src, src + dim_dst, dst);
    }
    delete[] src;
    return dst;
}
}

#endif
