/*
 * xmmCircularBuffer.hpp
 *
 * Simple Circular Buffer Utility
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

#ifndef xmmCircularbuffer_h
#define xmmCircularbuffer_h

#include <exception>
#include <stdexcept>
#include <vector>

namespace xmm {
/**
 @ingroup Common
 @brief Simple CircularBuffer Class
 @details Multichannel Circular Buffer
 @tparam T Data type
 @tparam channels number of channels
 */
template <typename T, unsigned int channels = 1>
class CircularBuffer {
  public:
    /**
     @brief Constructor
     @param length length of the CircularBuffer
     */
    CircularBuffer(unsigned int length = 1) {
        length_ = length;
        for (int c = 0; c < channels; c++) {
            data_[c].resize(length_);
        }
        current_index_ = 0;
        full_ = false;
    }

    /**
     @brief Access data by index & channel
     @return value at the given index and channel
     */
    T operator()(unsigned int channel, unsigned int index) const {
        if (channel >= channels)
            throw std::out_of_range("CircularBuffer: channel out of bounds");
        unsigned int m = full_ ? length_ : current_index_;
        if (index >= m)
            throw std::out_of_range("CircularBuffer: index out of bounds");
        return data_[channel][index];
    }

    /**
     @brief Clear the content of the buffer
     */
    void clear() {
        current_index_ = 0;
        full_ = false;
    }

    /**
     @brief Add an element to the buffer (single-channel method)
     @param value element to add to the buffer
     @throws invalid_argument if the buffer is multi-channel
     */
    void push(T const value) {
        if (channels > 1)
            throw std::invalid_argument("You must pass a vector or array");
        data_[0][current_index_] = value;
        current_index_++;
        if (current_index_ == length_) full_ = true;
        current_index_ %= length_;
    }

    /**
     @brief Add an element to the buffer (multi-channel method)
     @param value element to add to the buffer
     */
    void push(T const *value) {
        for (int c = 0; c < channels; c++) {
            data_[c][current_index_] = value[c];
        }
        current_index_++;
        if (current_index_ == length_) full_ = true;
        current_index_ %= length_;
    }

    /**
     @brief Add an element to the buffer (multi-channel method)
     @param value element to add to the buffer
     */
    void push(std::vector<T> const &value) {
        for (int c = 0; c < channels; c++) {
            data_[c][current_index_] = value[c];
        }
        current_index_++;
        if (current_index_ == length_) full_ = true;
        current_index_ %= length_;
    }

    /**
     @brief Get the size of the CircularBuffer
     @return size of the CircularBuffer (length)
     */
    unsigned int size() const { return length_; }

    /**
     @brief Get the actual size of the CircularBuffer (< size() if the buffer is
     not full)
     @return actual size of the CircularBuffer (length)
     */
    unsigned int size_t() const { return (full_ ? length_ : current_index_); }

    /**
     @brief Resize the buffer to a specific length
     @param length target length of the CircularBuffer
     */
    void resize(unsigned int length) {
        if (length == length_) return;
        if (length > length_) {
            full_ = false;
        } else if (current_index_ >= length) {
            full_ = true;
            current_index_ = 0;
        }
        length_ = length;
        for (int c = 0; c < channels; c++) {
            data_[c].resize(length_);
        }
    }

    /**
     @brief Compute the mean of the buffer
     @return vector containing the mean of the buffer
     */
    std::vector<T> mean() const {
        std::vector<T> _mean(channels, 0.0);
        int size = full_ ? length_ : current_index_;
        for (int c = 0; c < channels; c++) {
            for (int i = 0; i < size; i++) {
                _mean[c] += data_[c][i];
            }
            _mean[c] /= T(size);
        }
        return _mean;
    }

  protected:
    /**
     @brief buffer data
     */
    std::vector<T> data_[channels];

    /**
     @brief length of the buffer
     */
    unsigned int length_;

    /**
     @brief current index in the buffer
     */
    unsigned int current_index_;

    /**
     @brief Defines if the CircularBuffer is already full
     */
    bool full_;
};
}

#endif