/*
 * xmmAttribute.hpp
 *
 * Generic Attributes
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

#ifndef xmmAttribute_h
#define xmmAttribute_h

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace xmm {

#pragma mark -
#pragma mark === Functions checkLimits ===
/**
 @ingroup Common
 @brief checks the validity of the requested value with respect to the current
 limits
 @param value requested attribute value
 @param limit_min minimum value
 @param limit_max maximum value
 */
template <typename T>
void checkLimits(T const& value, T const& limit_min, T const& limit_max) {
    throw std::runtime_error(
        "Attribute limits are not implemented for the current type.");
}

template <>
void checkLimits<bool>(bool const& value, bool const& limit_min,
                       bool const& limit_max);

template <>
void checkLimits<unsigned char>(unsigned char const& value,
                                unsigned char const& limit_min,
                                unsigned char const& limit_max);

template <>
void checkLimits<char>(char const& value, char const& limit_min,
                       char const& limit_max);

template <>
void checkLimits<unsigned int>(unsigned int const& value,
                               unsigned int const& limit_min,
                               unsigned int const& limit_max);

template <>
void checkLimits<int>(int const& value, int const& limit_min,
                      int const& limit_max);

template <>
void checkLimits<long>(long const& value, long const& limit_min,
                       long const& limit_max);

template <>
void checkLimits<float>(float const& value, float const& limit_min,
                        float const& limit_max);

template <>
void checkLimits<double>(double const& value, double const& limit_min,
                         double const& limit_max);

template <>
void checkLimits<std::string>(std::string const& value,
                              std::string const& limit_min,
                              std::string const& limit_max);

#pragma mark -
#pragma mark === Class AttributeBase ===
/**
 @ingroup Common
 @brief Base Class for Generic Attributes
 */
class AttributeBase {
  public:
    virtual ~AttributeBase() {}

    /**
     @brief Default Constructor
     */
    AttributeBase() : changed(false) {}

    /**
     @brief Defines if the value has been changed
     */
    bool changed;
};

#pragma mark -
#pragma mark === Class Attribute ===
/**
 @ingroup Common
 @brief Generic Attribute
 @tparam T attribute type
 */
template <typename T>
class Attribute : public AttributeBase {
  public:
    typedef std::function<void(AttributeBase*)> AttributeChangeCallback;

    template <typename U, typename args, class ListenerClass>
    void onAttributeChange(U* owner,
                           void (ListenerClass::*listenerMethod)(args)) {
        using namespace std::placeholders;
        if (owner)
            callback_ = std::bind(listenerMethod, owner, _1);
        else
            callback_ = nullptr;
    }

    /**
     @brief Default Constructor
     @param value attribute value
     @param limit_min minimum limit
     @param limit_max maximum limit
     */
    Attribute(T const& value = T(), T const& limit_min = defaultLimitMin(),
              T const& limit_max = defaultLimitMax())
        : limit_min_(limit_min), limit_max_(limit_max) {
        set(value, true);
        changed = false;
    }

    /**
     @brief Copy Constructor
     @param src source attribute
     @warning the listener object is not copied from the source attribute
     */
    Attribute(Attribute const& src)
        : value_(src.value_),
          limit_min_(src.limit_min_),
          limit_max_(src.limit_max_) {
        changed = false;
    }

    /**
     @brief Assignment operator
     @param src source attribute
     @return copy of the source Attribute object
     @warning the listener object is not copied from the source attribute
     */
    template <typename U>
    Attribute& operator=(Attribute<U> const& src) {
        if (this != &src) {
            value_ = src.value_;
            limit_min_ = src.limit_min_;
            limit_max_ = src.limit_max_;
            changed = false;
        }
        return *this;
    }

    /**
     @brief Set the attribute value
     @param value requested value
     @param silently if true, don't notify the listener object
     @throws domain_error exception if the value exceeds the limits
     @throws runtime_error exception if the limit checking are not implemented
     for the current type
     */
    void set(T const& value, bool silently = false) {
        checkLimits(value, limit_min_, limit_max_);
        value_ = value;
        changed = true;
        if (!silently && callback_) callback_(this);
    }

    /**
     @brief get the attribute's current value
     @return the attribute's current value
     */
    T get() const { return value_; }

    /**
     @brief set the attribute's minimum value
     @param limit_min minimum value
     */
    void setLimitMin(T const& limit_min = defaultLimitMin()) {
        limit_min_ = limit_min;
    }

    /**
     @brief set the attribute's maximum value
     @param limit_max maximum value
     */
    void setLimitMax(T const& limit_max = defaultLimitMax()) {
        limit_max_ = limit_max;
    }

    /**
     @brief set the attribute's limit values
     @param limit_min minimum value
     @param limit_max maximum value
     */
    void setLimits(T const& limit_min = defaultLimitMin(),
                   T const& limit_max = defaultLimitMax()) {
        setLimitMin(limit_min);
        setLimitMax(limit_max);
    }

  protected:
    /**
     @brief Attribute default minimum value
     */
    static T defaultLimitMin() { return std::numeric_limits<T>::lowest(); }

    /**
     @brief Attribute default maximum value
     */
    static T defaultLimitMax() { return std::numeric_limits<T>::max(); }

    /**
     @brief Current value of the attribute
     */
    T value_;

    /**
     @brief Minimum value of the attribute
     */
    T limit_min_;

    /**
     @brief Maximum value of the attribute
     */
    T limit_max_;

    /**
     @brief Callback function to be called on attribute change
     */
    AttributeChangeCallback callback_;
};

#pragma mark -
#pragma mark === Class Attribute: vector specialization ===
/**
 @ingroup Common
 @brief Generic Attribute (Vector Specialization)
 @tparam T Vector base type
 */
template <typename T>
class Attribute<std::vector<T>> : public AttributeBase {
  public:
    typedef std::function<void(AttributeBase*)> AttributeChangeCallback;

    template <typename U, typename args, class ListenerClass>
    void onAttributeChange(U* owner,
                           void (ListenerClass::*listenerMethod)(args)) {
        using namespace std::placeholders;
        if (owner)
            callback_ = std::bind(listenerMethod, owner, _1);
        else
            callback_ = nullptr;
    }

    /**
     @brief Default Constructor
     @param value attribute value
     @param limit_min minimum limit
     @param limit_max maximum limit
     @param size size of the vector attribute (if 0, no size checking on set)
     */
    Attribute(std::vector<T> const& value = std::vector<T>(),
              T const& limit_min = defaultLimitMin(),
              T const& limit_max = defaultLimitMax(), unsigned int size = 0)
        : limit_min_(limit_min), limit_max_(limit_max), size_(size) {
        set(value, true);
        changed = false;
    }

    /**
     @brief Copy Constructor
     @param src source attribute
     @warning the listener object is not copied from the source attribute
     */
    Attribute(Attribute const& src)
        : value_(src.value_),
          limit_min_(src.limit_min_),
          limit_max_(src.limit_max_),
          size_(src.size_) {
        changed = false;
    }

    /**
     @brief Assignment operator
     @param src source attribute
     @return copy of the source Attribute object
     @warning the listener object is not copied from the source attribute
     */
    template <typename U>
    Attribute& operator=(Attribute<U> const& src) {
        if (this != &src) {
            value_ = src.value_;
            limit_min_ = src.limit_min_;
            limit_max_ = src.limit_max_;
            size_ = src.size_;
            changed = false;
        }
        return *this;
    }

    /**
     @brief Set the attribute value
     @param value requested value
     @param silently if true, don't notify the listener object
     @throws domain_error exception if the value exceeds the limits, or if the
     value does not
     match the attribute's size (if the attribute's size is > 0)
     @throws runtime_error exception if the limit checking are not implemented
     for the current type
     */
    void set(std::vector<T> const& value, bool silently = false) {
        if (size_ > 0 && value.size() != size_) {
            throw std::domain_error("Attribute value has the wrong size");
        }
        for (auto& val : value) {
            checkLimits(val, limit_min_, limit_max_);
        }
        value_ = value;
        changed = true;
        if (!silently && callback_) callback_(this);
    }

    /**
     @brief get the attribute's current value
     @return the attribute's current value
     */
    std::vector<T> get() const { return value_; }

    /**
     @brief get the attribute's current value at a given index
     @param index index in the value vector
     @return the attribute's current value at a given index
     */
    T at(unsigned int index) const {
        if (index < size_) {
            return value_[index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }

    /**
     @brief set the attribute's minimum value
     @param limit_min minimum value
     */
    void setLimitMin(T const& limit_min = defaultLimitMin()) {
        limit_min_ = limit_min;
    }

    /**
     @brief set the attribute's maximum value
     @param limit_max maximum value
     */
    void setLimitMax(T const& limit_max = defaultLimitMax()) {
        limit_max_ = limit_max;
    }

    /**
     @brief set the attribute's limit values
     @param limit_min minimum value
     @param limit_max maximum value
     */
    void setLimits(T const& limit_min = defaultLimitMin(),
                   T const& limit_max = defaultLimitMax()) {
        setLimitMin(limit_min);
        setLimitMax(limit_max);
    }

    /**
     @brief set the attribute's size (vector Attribute). if 0, there is not
     size-checking on set.
     @param size vector size
     */
    void resize(unsigned int size) {
        value_.resize(size, T());
        size_ = size;
    }

    /**
     @brief get the attribute's current size (vector Attribute)
     @return value vector size
     */
    unsigned int size() const { return value_.size(); }

  protected:
    /**
     @brief Attribute default minimum value
     */
    static T defaultLimitMin() { return std::numeric_limits<T>::lowest(); }

    /**
     @brief Attribute default maximum value
     */
    static T defaultLimitMax() { return std::numeric_limits<T>::max(); }

    /**
     @brief Current value of the attribute
     */
    std::vector<T> value_;

    /**
     @brief Minimum value of the attribute
     */
    T limit_min_;

    /**
     @brief Maximum value of the attribute
     */
    T limit_max_;

    /**
     @brief Size of the vector of values
     */
    unsigned int size_;

    /**
     @brief Callback function to be called on attribute change
     */
    AttributeChangeCallback callback_;
};

#pragma mark -
#pragma mark === Class Attribute: vector<string> specialization ===
/**
 @ingroup Common
 @brief Generic Attribute (Vector Specialization)
 */
template <>
class Attribute<std::vector<std::string>> : public AttributeBase {
  public:
    typedef std::function<void(AttributeBase*)> AttributeChangeCallback;

    template <typename U, typename args, class ListenerClass>
    void onAttributeChange(U* owner,
                           void (ListenerClass::*listenerMethod)(args)) {
        using namespace std::placeholders;
        if (owner)
            callback_ = std::bind(listenerMethod, owner, _1);
        else
            callback_ = nullptr;
    }

    /**
     @brief Default Constructor
     @param value attribute value
     @param limit_min minimum limit
     @param limit_max maximum limit
     @param size size of the vector attribute (if 0, no size checking on set)
     */
    Attribute(std::vector<std::string> const& value = {},
              std::string const& limit_min = "",
              std::string const& limit_max = "", unsigned int size = 0)
        : limit_min_(limit_min), limit_max_(limit_max), size_(size) {
        set(value, true);
        changed = false;
    }

    /**
     @brief Copy Constructor
     @param src source attribute
     @warning the listener object is not copied from the source attribute
     */
    Attribute(Attribute const& src)
        : value_(src.value_),
          limit_min_(src.limit_min_),
          limit_max_(src.limit_max_),
          size_(src.size_) {
        changed = false;
    }

    /**
     @brief Assignment operator
     @param src source attribute
     @return copy of the source Attribute object
     @warning the listener object is not copied from the source attribute
     */
    template <typename U>
    Attribute& operator=(Attribute<U> const& src) {
        if (this != &src) {
            value_ = src.value_;
            limit_min_ = src.limit_min_;
            limit_max_ = src.limit_max_;
            size_ = src.size_;
            changed = false;
        }
        return *this;
    }

    /**
     @brief Set the attribute value
     @param value requested value
     @param silently if true, don't notify the listener object
     @throws domain_error exception if the value exceeds the limits, or if the
     value does not
     match the attribute's size (if the attribute's size is > 0)
     @throws runtime_error exception if the limit checking are not implemented
     for the current type
     */
    void set(std::vector<std::string> const& value, bool silently = false) {
        if (size_ > 0 && value.size() != size_) {
            throw std::domain_error("Attribute value has the wrong size");
        }
        for (auto& val : value) {
            checkLimits(val, limit_min_, limit_max_);
        }
        value_ = value;
        changed = true;
        if (!silently && callback_) callback_(this);
    }

    /**
     @brief get the attribute's current value
     @return the attribute's current value
     */
    std::vector<std::string> get() const { return value_; }

    /**
     @brief get the attribute's current value at a given index
     @param index index in the value vector
     @return the attribute's current value at a given index
     */
    std::string at(unsigned int index) const {
        if (index < size_) {
            return value_[index];
        } else {
            throw std::out_of_range("Index out of range");
        }
    }

    /**
     @brief set the attribute's minimum value
     @param limit_min minimum value
     */
    void setLimitMin(std::string const& limit_min = defaultLimitMin()) {
        limit_min_ = limit_min;
    }

    /**
     @brief set the attribute's maximum value
     @param limit_max maximum value
     */
    void setLimitMax(std::string const& limit_max = defaultLimitMax()) {
        limit_max_ = limit_max;
    }

    /**
     @brief set the attribute's limit values
     @param limit_min minimum value
     @param limit_max maximum value
     */
    void setLimits(std::string const& limit_min = defaultLimitMin(),
                   std::string const& limit_max = defaultLimitMax()) {
        setLimitMin(limit_min);
        setLimitMax(limit_max);
    }

    /**
     @brief set the attribute's size (vector Attribute). if 0, there is not
     size-checking on set.
     @param size vector size
     */
    void resize(unsigned int size) {
        value_.resize(size, "");
        size_ = size;
    }

    /**
     @brief get the attribute's current size (vector Attribute)
     @return value vector size
     */
    unsigned int size() const {
        return static_cast<unsigned int>(value_.size());
    }

  protected:
    /**
     @brief Attribute default minimum value
     */
    static std::string defaultLimitMin() { return ""; }

    /**
     @brief Attribute default maximum value
     */
    static std::string defaultLimitMax() { return ""; }

    /**
     @brief Current value of the attribute
     */
    std::vector<std::string> value_;

    /**
     @brief Minimum value of the attribute
     */
    std::string limit_min_;

    /**
     @brief Maximum value of the attribute
     */
    std::string limit_max_;

    /**
     @brief Size of the vector of values
     */
    unsigned int size_;

    /**
     @brief Callback function to be called on attribute change
     */
    AttributeChangeCallback callback_;
};
}

#endif
