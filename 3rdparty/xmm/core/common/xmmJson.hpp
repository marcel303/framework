/*
 * xmmJson.hpp
 *
 * Set of utility functions for JSON I/O
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

#ifndef xmmJson_h
#define xmmJson_h

#include "../../3rdparty/jsoncpp/include/json.h"
#include <fstream>
#include <vector>

namespace xmm {
/**
 @ingroup Common
 @brief Abstract class for handling JSON + File I/O
 @details the JSON I/O methods need to be implemented. writeFile and readFile
 methods
 can be used in Python for file I/O. The __str__() Python method is implemented
 to use
 with "print" in Python. It return the pretty-printed JSON String.
 */
class Writable {
  public:
    virtual ~Writable() {}

    /** @name Json I/O */
    ///@{

    /**
     @brief Write the object to a JSON Structure
     @return Json value containing the object's information
     */
    virtual Json::Value toJson() const = 0;

    /**
     @brief Read the object from a JSON Structure
     @param root JSON value containing the object's information
     @throws JsonException if the JSON value has a wrong format
     */
    virtual void fromJson(Json::Value const& root) = 0;

///@}

#ifdef SWIGPYTHON
    /** @name Python File I/O */
    ///@{

    /**
     @brief write method for python wrapping ('write' keyword forbidden, name
     has to be different)
     @warning only defined if SWIGPYTHON is defined
     */
    void writeFile(char* fileName) const {
        std::ofstream outStream;
        outStream.open(fileName);
        outStream << this->toJson();
        outStream.close();
    }

    /**
     @brief read method for python wrapping ('read' keyword forbidden, name has
     to be different)
     @warning only defined if SWIGPYTHON is defined
     */
    void readFile(char* fileName) {
        std::string jsonstring;
        std::ifstream inStream;
        inStream.open(fileName);
        //            inStream.seekg(0, std::ios::end);
        //            jsonstring.reserve(inStream.tellg());
        //            inStream.seekg(0, std::ios::beg);
        //
        //            jsonstring.assign((std::istreambuf_iterator<char>(inStream)),
        //                              std::istreambuf_iterator<char>());
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(inStream, root)) {
            this->fromJson(root);
        } else {
            throw std::runtime_error("Cannot Parse Json File");
        }
        inStream.close();
    }

    /**
     @brief "print" method for python => returns the results of write method
     @warning only defined if SWIGPYTHON is defined
     */
    std::string __str__() const { return this->toJson().toStyledString(); }

///@}
#endif
};

/**
 @ingroup Common
 @brief Exception class for handling JSON parsing errors
 */
class JsonException : public std::exception {
  public:
    /**
     @brief Type of Json parsing errors
     */
    enum class JsonErrorType {
        /**
         @brief A Node is Missing in the Json Value
         */
        JsonMissingNode,

        /**
         @brief The current node has wrong data type
         */
        JsonTypeError,

        /**
         @brief The current node has an inadmissible value
         */
        JsonValueError
    };

    /**
     @brief Default Constructor
     @param errorType type of parsing error
     @param nodename name of the JSON node where the error occurred
     */
    JsonException(JsonErrorType errorType, std::string nodename = "")
        : errorType_(errorType) {
        nodename_.push_back(nodename);
    }

    /**
     @brief Constructor From exception message
     @param src Source Exception
     @param nodename name of the
     */
    explicit JsonException(JsonException const& src, std::string nodename)
        : errorType_(src.errorType_), nodename_(src.nodename_) {
        nodename_.push_back(nodename);
    }

    /**
     @brief Copy Constructor
     @param src Source exception
     */
    JsonException(JsonException const& src)
        : errorType_(src.errorType_), nodename_(src.nodename_) {}

    /**
     @brief Assigment
     @param src Source exception
     */
    JsonException& operator=(JsonException const& src) {
        if (this != &src) {
            errorType_ = src.errorType_;
            nodename_ = src.nodename_;
        }
        return *this;
    }

    /**
     @brief Get exception message
     @return exception message
     */
    virtual const char* what() const throw() {
        std::string message;
        switch (errorType_) {
            case JsonErrorType::JsonMissingNode:
                message = "Json Structure Error: Missing Node";
                break;

            case JsonErrorType::JsonTypeError:
                message = "Json Structure Error: Type Error";
                break;

            case JsonErrorType::JsonValueError:
                message = "Json Structure Error: Value Error";
                break;

            default:
                message = "Json unknown error type";
                break;
        }
        message += " (root";
        for (auto& node : nodename_) message += " > " + node;
        message += ")";
        return message.c_str();
    }

  private:
    /**
     @brief Type of Json Parsing Error
     */
    JsonErrorType errorType_;

    /**
     @brief Name of the Json Node presenting an error
     */
    std::vector<std::string> nodename_;
};

/**
 @ingroup Common
 @brief Writes a C-style array to a Json Value
 @param a array
 @param n array size
 @return Json Value containing the array in Json Format
 */
template <typename T>
Json::Value array2json(T const* a, unsigned int n) {
    Json::Value root;
    root.resize(static_cast<Json::ArrayIndex>(n));
    for (int i = 0; i < n; i++) {
        root[i] = a[i];
    }
    return root;
}

/**
 @ingroup Common
 @brief Reads a C-style array from a Json Value
 @param root Json Value containing the array in Json Format
 @param a array
 @param n array size
 */
template <typename T>
void json2array(Json::Value const& root, T* a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asInt();
    }
}

template <>
void json2array(Json::Value const& root, float* a, unsigned int n);
template <>
void json2array(Json::Value const& root, double* a, unsigned int n);
template <>
void json2array(Json::Value const& root, bool* a, unsigned int n);
template <>
void json2array(Json::Value const& root, std::string* a, unsigned int n);

/**
 @ingroup Common
 @brief Writes a vector to a Json Value
 @param a vector
 @return Json Value containing the vector data in Json Format
 */
template <typename T>
Json::Value vector2json(std::vector<T> const& a) {
    Json::Value root;
    root.resize(static_cast<Json::ArrayIndex>(a.size()));
    for (int i = 0; i < a.size(); i++) {
        root[i] = a[i];
    }
    return root;
}

/**
 @ingroup Common
 @brief Reads a vector from a Json Value
 @param root Json Value containing the vector data in Json Format
 @param a vector
 @param n vector size
 @warning the target vector must already have the appropriate size
 */
template <typename T>
void json2vector(Json::Value const& root, std::vector<T>& a, unsigned int n) {
    if (!root.isArray())
        throw JsonException(JsonException::JsonErrorType::JsonTypeError);
    if (root.size() != n)
        throw JsonException(JsonException::JsonErrorType::JsonValueError);
    unsigned int i = 0;
    for (auto it : root) {
        a[i++] = it.asInt();
    }
}

template <>
void json2vector(Json::Value const& root, std::vector<float>& a,
                 unsigned int n);
template <>
void json2vector(Json::Value const& root, std::vector<double>& a,
                 unsigned int n);
template <>
void json2vector(Json::Value const& root, std::vector<bool>& a, unsigned int n);
template <>
void json2vector(Json::Value const& root, std::vector<std::string>& a,
                 unsigned int n);
}

#endif
