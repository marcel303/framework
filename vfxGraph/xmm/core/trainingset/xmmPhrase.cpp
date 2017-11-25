/*
 * xmmPhrase.cpp
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

#include "xmmPhrase.hpp"
#include <limits>
#include <algorithm>

xmm::Phrase::Phrase(MemoryMode memoryMode, Multimodality multimodality)
    : own_memory_(memoryMode == MemoryMode::OwnMemory),
      bimodal_(multimodality == Multimodality::Bimodal),
      empty_(true),
      length_(0),
      input_length_(0),
      output_length_(0),
      max_length_(0) {
    dimension.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    dimension_input.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    label.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    dimension.setLimitMin((bimodal_) ? 2 : 1);
    dimension.set((bimodal_) ? 2 : 1, true);
    dimension_input.setLimits((bimodal_) ? 1 : 0, (bimodal_) ? 2 : 0);
    dimension_input.set((bimodal_) ? 1 : 0, true);
    data_ = new float*[bimodal_ ? 2 : 1];
    data_[0] = NULL;
    if (bimodal_) data_[1] = NULL;
}

xmm::Phrase::Phrase(Phrase const& src)
    : dimension(src.dimension),
      dimension_input(src.dimension_input),
      label(src.label),
      column_names(src.column_names),
      own_memory_(src.own_memory_),
      bimodal_(src.bimodal_),
      empty_(src.empty_),
      length_(src.length_),
      input_length_(src.input_length_),
      output_length_(src.output_length_),
      max_length_(src.max_length_) {
    if (own_memory_) {
        data_ = new float*[bimodal_ ? 2 : 1];
        if (max_length_ > 0) {
            unsigned int modality_dim =
                bimodal_ ? dimension_input.get() : dimension.get();
            data_[0] = new float[max_length_ * modality_dim];
            std::copy(src.data_[0], src.data_[0] + max_length_ * modality_dim,
                      data_[0]);
            if (bimodal_) {
                modality_dim = dimension.get() - dimension_input.get();
                data_[1] = new float[max_length_ * modality_dim];
                std::copy(src.data_[1],
                          src.data_[1] + max_length_ * modality_dim, data_[1]);
            }
        }
    } else {
        data_[0] = src.data_[0];
        if (bimodal_) data_[1] = src.data_[1];
    }
    dimension.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    dimension_input.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    label.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
}

xmm::Phrase::Phrase(Json::Value const& root)
    : own_memory_(true),
      bimodal_(false),
      empty_(true),
      length_(0),
      input_length_(0),
      output_length_(0),
      max_length_(0) {
    if (!own_memory_)
        throw std::runtime_error("Cannot read Phrase with Shared memory");

    dimension.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    dimension_input.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    label.onAttributeChange(this, &xmm::Phrase::onAttributeChange);

    bimodal_ = root.get("bimodal", false).asBool();
    dimension.setLimitMin((bimodal_) ? 2 : 1);
    dimension_input.setLimits((bimodal_) ? 1 : 0, (bimodal_) ? 2 : 0);
    dimension.set(root.get("dimension", bimodal_ ? 2 : 1).asInt(), true);
    dimension_input.set(root.get("dimension_input", bimodal_ ? 1 : 0).asInt(),
                        true);
    data_ = new float*[bimodal_ ? 2 : 1];
    data_[0] = NULL;
    if (bimodal_) data_[1] = NULL;

    column_names.resize(dimension.get(), "");
    for (int i = 0; i < root["column_names"].size(); i++) {
        column_names[i] = root["column_names"].get(i, "").asString();
    }

    label.set(root["label"].asString());

    length_ = static_cast<unsigned int>(root.get("length", 0).asInt());
    max_length_ = length_;
    input_length_ = length_;
    output_length_ = length_;
    empty_ = (length_ == 0 && input_length_ == 0 && output_length_ == 0);

    if (bimodal_) {
        data_[0] =
            reallocate<float>(data_[0], max_length_ * dimension_input.get(),
                              length_ * dimension_input.get());
        data_[1] = reallocate<float>(
            data_[1], max_length_ * (dimension.get() - dimension_input.get()),
            length_ * (dimension.get() - dimension_input.get()));
        json2array(root["data_input"], data_[0],
                   length_ * dimension_input.get());
        json2array(root["data_output"], data_[1],
                   length_ * (dimension.get() - dimension_input.get()));
    } else {
        data_[0] = reallocate<float>(data_[0], max_length_ * dimension.get(),
                                     length_ * dimension.get());
        json2array(root["data"], data_[0], length_ * dimension.get());
    }
}

xmm::Phrase& xmm::Phrase::operator=(Phrase const& src) {
    if (this != &src) {
        if (own_memory_) {
            if (data_) {
                if (bimodal_) try {
                        delete[] data_[1];
                    } catch (std::exception& e) {
                    }
                try {
                    delete[] data_[0];
                } catch (std::exception& e) {
                }
            }
            try {
                delete[] data_;
            } catch (std::exception& e) {
            }
            data_ = NULL;
        }
        own_memory_ = src.own_memory_;
        bimodal_ = src.bimodal_;
        empty_ = src.empty_;
        dimension = src.dimension;
        dimension_input = src.dimension_input;
        max_length_ = src.max_length_;
        length_ = src.length_;
        input_length_ = src.input_length_;
        output_length_ = src.output_length_;
        column_names = src.column_names;
        label = src.label;

        if (own_memory_) {
            data_ = new float*[bimodal_ ? 2 : 1];
            if (max_length_ > 0) {
                unsigned int modality_dim =
                    bimodal_ ? dimension_input.get() : dimension.get();
                data_[0] = new float[max_length_ * modality_dim];
                std::copy(src.data_[0],
                          src.data_[0] + max_length_ * modality_dim, data_[0]);
                if (bimodal_) {
                    modality_dim = dimension.get() - dimension_input.get();
                    data_[1] = new float[max_length_ * modality_dim];
                    std::copy(src.data_[1],
                              src.data_[1] + max_length_ * modality_dim,
                              data_[1]);
                }
            }
        } else {
            data_[0] = src.data_[0];
            if (bimodal_) data_[1] = src.data_[1];
        }
        dimension.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
        dimension_input.onAttributeChange(this,
                                          &xmm::Phrase::onAttributeChange);
        label.onAttributeChange(this, &xmm::Phrase::onAttributeChange);
    }
    return *this;
}

xmm::Phrase::~Phrase() {
    if (own_memory_) {
        if (bimodal_) {
            delete[] data_[1];
        }
        delete[] data_[0];
    }
    delete[] data_;
}

bool xmm::Phrase::ownMemory() const { return own_memory_; }

bool xmm::Phrase::bimodal() const { return bimodal_; }

unsigned int xmm::Phrase::size() const { return length_; }

unsigned int xmm::Phrase::inputSize() const { return input_length_; }

unsigned int xmm::Phrase::outputSize() const { return output_length_; }

bool xmm::Phrase::empty() const { return empty_; }

float xmm::Phrase::getValue(unsigned int index, unsigned int dim) const {
    if (dim >= dimension.get())
        throw std::out_of_range("Phrase: dimension out of bounds");
    if (bimodal_) {
        if (dim < dimension_input.get()) {
            if (index >= input_length_)
                throw std::out_of_range("Phrase: index out of bounds");
            return data_[0][index * dimension_input.get() + dim];
        } else {
            if (index >= output_length_)
                throw std::out_of_range("Phrase: index out of bounds");
            return data_[1][index * (dimension.get() - dimension_input.get()) +
                            dim - dimension_input.get()];
        }
    } else {
        if (index >= length_)
            throw std::out_of_range("Phrase: index out of bounds");
        return data_[0][index * dimension.get() + dim];
    }
}

float* xmm::Phrase::getPointer(unsigned int index) const {
    if (index >= length_)
        throw std::out_of_range("Phrase: index out of bounds");
    if (bimodal_)
        throw std::runtime_error(
            "this phrase is bimodal_, use 'get_dataPointer_input' and "
            "'get_dataPointer_output'");
    return data_[0] + index * dimension.get();
}

float* xmm::Phrase::getPointer_input(unsigned int index) const {
    if (index >= length_)
        throw std::out_of_range("Phrase: index out of bounds");
    if (!bimodal_)
        throw std::runtime_error(
            "this phrase is unimodal, use 'get_dataPointer'");
    return data_[0] + index * dimension_input.get();
}

float* xmm::Phrase::getPointer_output(unsigned int index) const {
    if (index >= length_)
        throw std::out_of_range("Phrase: index out of bounds");
    if (!bimodal_)
        throw std::runtime_error(
            "this phrase is unimodal, use 'get_dataPointer'");
    return data_[1] + index * (dimension.get() - dimension_input.get());
}

void xmm::Phrase::connect(float* pointer_to_data, unsigned int length) {
    if (own_memory_)
        throw std::runtime_error("Cannot connect a phrase with own data");
    if (bimodal_)
        throw std::runtime_error(
            "Cannot connect a single array, use 'connect_input' and "
            "'connect_output'");

    data_[0] = pointer_to_data;
    input_length_ = length;
    length_ = length;
    empty_ = false;
}

void xmm::Phrase::connect(float* pointer_to_data_input,
                          float* pointer_to_data_output, unsigned int length) {
    if (own_memory_)
        throw std::runtime_error("Cannot connect a phrase with own data");
    if (!bimodal_)
        throw std::runtime_error("This phrase is unimodal, use 'connect'");

    data_[0] = pointer_to_data_input;
    data_[1] = pointer_to_data_output;
    input_length_ = length;
    output_length_ = length;
    trim();
    empty_ = false;
}

void xmm::Phrase::connect_input(float* pointer_to_data, unsigned int length) {
    if (own_memory_)
        throw std::runtime_error("Cannot connect a phrase with own data");
    if (!bimodal_)
        throw std::runtime_error("This phrase is unimodal, use 'connect'");

    data_[0] = pointer_to_data;
    input_length_ = length;
    trim();
    empty_ = false;
}

void xmm::Phrase::connect_output(float* pointer_to_data, unsigned int length) {
    if (own_memory_)
        throw std::runtime_error("Cannot connect a phrase with own data");
    if (!bimodal_)
        throw std::runtime_error("This phrase is unimodal, use 'connect'");

    data_[1] = pointer_to_data;
    output_length_ = length;
    trim();
    empty_ = false;
}

void xmm::Phrase::disconnect() {
    if (own_memory_)
        throw std::runtime_error("Cannot disconnect a phrase with own data");
    data_[0] = NULL;
    if (bimodal_) data_[1] = NULL;
    length_ = 0;
    input_length_ = 0;
    output_length_ = 0;
    empty_ = true;
}

void xmm::Phrase::record(std::vector<float> const& observation) {
    if (!own_memory_)
        throw std::runtime_error("Cannot record in shared data phrase");
    if (bimodal_ && input_length_ != output_length_)
        throw std::runtime_error(
            "Cannot record bimodal_ phrase in synchronous mode: modalities "
            "have different length");
    if (observation.size() != dimension.get())
        throw std::invalid_argument("Observation has wrong dimension");

    if (length_ >= max_length_ || max_length_ == 0) {
        reallocateLength();
    }

    if (bimodal_) {
        copy(observation.begin(), observation.begin() + dimension_input.get(),
             data_[0] + input_length_ * dimension_input.get());
        copy(observation.begin() + dimension_input.get(),
             observation.begin() + dimension.get(),
             data_[1] +
                 output_length_ * (dimension.get() - dimension_input.get()));
        input_length_++;
        output_length_++;
    } else {
        copy(observation.begin(), observation.end(),
             data_[0] + length_ * dimension.get());
        input_length_++;
    }

    length_++;
    empty_ = false;
}

void xmm::Phrase::record_input(std::vector<float> const& observation) {
    if (!own_memory_)
        throw std::runtime_error("Cannot record in shared data phrase");
    if (!bimodal_)
        throw std::runtime_error("this phrase is unimodal, use 'record'");
    if (observation.size() != dimension_input.get())
        throw std::invalid_argument("Observation has wrong dimension");

    if (input_length_ >= max_length_ || max_length_ == 0) {
        reallocateLength();
    }

    copy(observation.begin(), observation.end(),
         data_[0] + input_length_ * dimension_input.get());
    input_length_++;
    trim();
    empty_ = false;
}

void xmm::Phrase::record_output(std::vector<float> const& observation) {
    if (!own_memory_)
        throw std::runtime_error("Cannot record in shared data phrase");
    if (!bimodal_)
        throw std::runtime_error("this phrase is unimodal, use 'record'");

    if (observation.size() != dimension.get() - dimension_input.get())
        throw std::invalid_argument("Observation has wrong dimension");

    if (output_length_ >= max_length_ || max_length_ == 0) {
        reallocateLength();
    }

    copy(observation.begin(), observation.end(),
         data_[1] + output_length_ * (dimension.get() - dimension_input.get()));
    output_length_++;
    trim();
    empty_ = false;
}

void xmm::Phrase::clear() {
    if (!own_memory_)
        throw std::runtime_error("Cannot clear a shared data phrase");

    length_ = 0;
    input_length_ = 0;
    output_length_ = 0;
    empty_ = true;
}

void xmm::Phrase::clearInput() {
    if (!own_memory_)
        throw std::runtime_error("Cannot clear a shared data phrase");
    if (!bimodal_) length_ = 0;
    input_length_ = 0;
    trim();
}

void xmm::Phrase::clearOutput() {
    if (!own_memory_)
        throw std::runtime_error("Cannot clear a shared data phrase");
    if (!bimodal_) length_ = 0;
    output_length_ = 0;
    trim();
}

Json::Value xmm::Phrase::toJson() const {
    Json::Value root;
    root["bimodal"] = bimodal_;
    root["dimension"] = static_cast<int>(dimension.get());
    root["dimension_input"] = static_cast<int>(dimension_input.get());
    root["length"] = static_cast<int>(length_);
    root["label"] = label.get();
    for (int i = 0; i < column_names.size(); i++)
        root["column_names"][i] = column_names[i];
    if (bimodal_) {
        root["data_input"] =
            array2json(data_[0], length_ * dimension_input.get());
        root["data_output"] = array2json(
            data_[1], length_ * (dimension.get() - dimension_input.get()));
    } else {
        root["data"] = array2json(data_[0], length_ * dimension.get());
    }
    return root;
}

void xmm::Phrase::fromJson(Json::Value const& root) {
    try {
        Phrase tmp(root);
        *this = tmp;
    } catch (JsonException& e) {
        throw e;
    }
}

std::vector<float> xmm::Phrase::mean() const {
    std::vector<float> mean(dimension.get());
    for (unsigned int d = 0; d < dimension.get(); d++) {
        mean[d] = 0.;
        for (unsigned int t = 0; t < length_; t++) {
            mean[d] += getValue(t, d);
        }
        mean[d] /= float(length_);
    }
    return mean;
}

std::vector<float> xmm::Phrase::standardDeviation() const {
    std::vector<float> stddev(dimension.get());
    std::vector<float> _mean = mean();
    for (unsigned int d = 0; d < dimension.get(); d++) {
        stddev[d] = 0.;
        for (unsigned int t = 0; t < length_; t++) {
            stddev[d] +=
                (getValue(t, d) - _mean[d]) * (getValue(t, d) - _mean[d]);
        }
        stddev[d] /= float(length_);
        stddev[d] = sqrtf(stddev[d]);
    }
    return stddev;
}

std::vector<std::pair<float, float>> xmm::Phrase::minmax() const {
    std::vector<std::pair<float, float>> minmax(
        dimension.get(), {std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::lowest()});
    for (unsigned int d = 0; d < dimension.get(); d++) {
        for (unsigned int t = 0; t < length_; t++) {
            minmax[d].first = std::min(getValue(t, d), minmax[d].first);
            minmax[d].second = std::max(getValue(t, d), minmax[d].second);
        }
    }
    return minmax;
}

void xmm::Phrase::rescale(std::vector<float> offset, std::vector<float> gain) {
    for (int t = 0; t < size(); t++) {
        float* p;
        if (bimodal_) {
            p = getPointer_input(t);
            for (int d = 0; d < dimension_input.get(); d++) {
                p[d] -= offset[d];
                p[d] *= gain[d];
            }
            p = getPointer_output(t);
            for (int d = dimension_input.get(); d < dimension.get(); d++) {
                p[d] -= offset[d];
                p[d] *= gain[d];
            }
        } else {
            p = getPointer(t);
            for (int d = 0; d < dimension.get(); d++) {
                p[d] -= offset[d];
                p[d] *= gain[d];
            }
        }
    }
}

void xmm::Phrase::trim() {
    if (bimodal_) {
        length_ = std::min(input_length_, output_length_);
        empty_ = std::max(input_length_, output_length_) == 0;
    }
}

void xmm::Phrase::reallocateLength() {
    unsigned int modality_dim =
        bimodal_ ? dimension_input.get() : dimension.get();
    data_[0] =
        reallocate<float>(data_[0], max_length_ * modality_dim,
                          (max_length_ + AllocationBlockSize) * modality_dim);
    if (bimodal_) {
        modality_dim = dimension.get() - dimension_input.get();
        data_[1] = reallocate<float>(
            data_[1], max_length_ * modality_dim,
            (max_length_ + AllocationBlockSize) * modality_dim);
    }
    max_length_ += AllocationBlockSize;
}

void xmm::Phrase::onAttributeChange(xmm::AttributeBase* attr_pointer) {
    if (attr_pointer == &dimension || attr_pointer == &dimension_input) {
        length_ = 0;
        input_length_ = 0;
        output_length_ = 0;
        max_length_ = 0;
        empty_ = true;
        if (own_memory_) {
            delete data_[0];
        }
        data_[0] = nullptr;
        if (bimodal_) {
            if (own_memory_) {
                data_[1] = nullptr;
            }
            delete data_[1];
        }
        column_names.resize(dimension.get());
    }
    if (attr_pointer == &dimension)
        dimension_input.setLimitMax(dimension.get() - 1);
    if (attr_pointer == &label) {
        PhraseEvent event(this, PhraseEvent::Type::LabelChanged);
        events.notifyListeners(event);
    }
    attr_pointer->changed = false;
}
