/*
 * xmmMatrix.hpp
 *
 * Matrix Utilities
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

#ifndef xmmMatrix_h
#define xmmMatrix_h

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace xmm {
/**
 @defgroup Common [Core] Common Classes and Utilities
 */

/**
 @ingroup Common
 @brief Dirty and very incomplete Matrix Class
 @details Contains few utilities for matrix operations, with possibility to
 share data with vectors
 @tparam T data type of the matrix (should be used with float/double)
 */
template <typename T>
class Matrix {
  public:
    /**
     @brief Epsilon value for Matrix inversion
     @details  defines
     */
    static const double kEpsilonPseudoInverse() { return 1.0e-9; }

    /**
     @brief number of rows of the matrix
     */
    unsigned int nrows;

    /**
     @brief number of columns of the matrix
     */
    unsigned int ncols;

    /**
     @brief Matrix Data if not shared
     */
    std::vector<T> _data;

    /**
     @brief Data iterator
     @details Can point to own data vector, or can be shared with another
     container.
     */
    typename std::vector<T>::iterator data;

    /**
     @brief Defines if the matrix has its own data
     */
    bool ownData;

    /**
     @brief Default Constructor
     @param ownData_ defines if the matrix stores the data itself (true by
     default)
     */
    Matrix(bool ownData_ = true) : nrows(0), ncols(0), ownData(ownData_) {}

    /**
     @brief Square Matrix Constructor
     @param nrows_ Number of rows (defines a square matrix)
     @param ownData_ defines if the matrix stores the data itself (true by
     default)
     */
    Matrix(unsigned int nrows_, bool ownData_ = true) {
        nrows = nrows_;
        ncols = nrows_;
        ownData = ownData_;
        if (ownData) {
            _data.assign(nrows * ncols, T(0.0));
            data = _data.begin();
        }
    }

    /**
     @brief Constructor
     @param nrows_ Number of rows
     @param ncols_ Number of columns
     @param ownData_ defines if the matrix stores the data itself (true by
     default)
     */
    Matrix(unsigned int nrows_, unsigned int ncols_, bool ownData_ = true) {
        nrows = nrows_;
        ncols = ncols_;
        ownData = ownData_;
        if (ownData) {
            _data.assign(nrows * ncols, T(0.0));
            data = _data.begin();
        }
    }

    /**
     @brief Constructor from vector (shared data)
     @param nrows_ Number of rows
     @param ncols_ Number of columns
     @param data_it iterator to the vector data
     */
    Matrix(unsigned int nrows_, unsigned int ncols_,
           typename std::vector<T>::iterator data_it) {
        nrows = nrows_;
        ncols = ncols_;
        ownData = false;
        data = data_it;
    }

    /**
     @brief Resize the matrix
     @param nrows_ Number of rows
     @param ncols_ Number of columns
     @throws runtime_error if the matrix is not square
     */
    void resize(unsigned int nrows_, unsigned int ncols_) {
        nrows = nrows_;
        ncols = ncols_;
        _data.resize(nrows * ncols);
    }

    /**
     @brief Resize a Square Matrix
     @param nrows_ Number of rows
     */
    void resize(unsigned int nrows_) {
        if (nrows != ncols) throw std::runtime_error("Matrix is not square");

        nrows = nrows_;
        ncols = nrows;
        _data.resize(nrows * ncols);
    }

    /**
     @brief Compute the Sum of the matrix
     @return sum of all elements in the matrix
     */
    float sum() const {
        float sum_(0.);
        for (unsigned int i = 0; i < nrows * ncols; i++) sum_ += data[i];
        return sum_;
    }

    /**
     @brief Print the matrix
     */
    void print() const {
        for (unsigned int i = 0; i < nrows; i++) {
            for (unsigned int j = 0; j < ncols; j++) {
                std::cout << data[i * ncols + j] << " ";
            }
            std::cout << std::endl;
        }
    }

    /**
     @brief Compute the transpose matrix
     @return pointer to the transpose Matrix
     @warning Memory is allocated for the new matrix (need to be freed)
     */
    Matrix<T> *transpose() const {
        Matrix<T> *out = new Matrix<T>(ncols, nrows);
        for (unsigned int i = 0; i < ncols; i++) {
            for (unsigned int j = 0; j < nrows; j++) {
                out->data[i * nrows + j] = data[j * ncols + i];
            }
        }
        return out;
    }

    /**
     @brief Compute the product of matrices
     @return pointer to the Matrix resulting of the product
     @warning Memory is allocated for the new matrix (need to be freed)
     @throws runtime_error if the matrices have wrong dimensions
     */
    Matrix<T> *product(Matrix const *mat) const {
        if (ncols != mat->nrows)
            throw std::runtime_error("Wrong dimensions for matrix product");

        Matrix<T> *out = new Matrix<T>(nrows, mat->ncols);
        for (unsigned int i = 0; i < nrows; i++) {
            for (unsigned int j = 0; j < mat->ncols; j++) {
                out->data[i * mat->ncols + j] = 0.;
                for (unsigned int k = 0; k < ncols; k++) {
                    out->data[i * mat->ncols + j] +=
                        data[i * ncols + k] * mat->data[k * mat->ncols + j];
                }
            }
        }
        return out;
    }

    /**
     @brief Compute the Pseudo-Inverse of a Matrix
     @param det Determinant (computed with the inversion)
     @return pointer to the inverse Matrix
     @warning Memory is allocated for the new matrix (need to be freed)
     */
    Matrix<T> *pinv(double *det) const {
        Matrix<T> *inverse = NULL;
        if (nrows == ncols) {
            inverse = gauss_jordan_inverse(det);
            if (inverse) {
                return inverse;
            }
        }

        inverse = new Matrix<T>(ncols, nrows);
        Matrix<T> *transp, *prod, *dst;
        transp = this->transpose();
        if (nrows >= ncols) {
            prod = transp->product(this);
            dst = prod->gauss_jordan_inverse(det);
            inverse = dst->product(transp);
        } else {
            prod = this->product(transp);
            dst = prod->gauss_jordan_inverse(det);
            inverse = transp->product(dst);
        }
        *det = 0;
        delete transp;
        delete prod;
        delete dst;
        return inverse;
    }

    /**
     @brief Compute the Gauss-Jordan Inverse of a Square Matrix
     @param det Determinant (computed with the inversion)
     @return pointer to the inverse Matrix
     @warning Memory is allocated for the new matrix (need to be freed)
     @throws runtime_error if the matrix is not square
     @throws runtime_error if the matrix is not invertible
     */
    Matrix<T> *gauss_jordan_inverse(double *det) const {
        if (nrows != ncols) {
            throw std::runtime_error(
                "Gauss-Jordan inversion: Can't invert Non-square matrix");
        }
        *det = 1.0f;
        Matrix<T> mat(nrows, ncols * 2);
        Matrix<T> new_mat(nrows, ncols * 2);

        unsigned int n = nrows;

        // Create matrix
        for (unsigned int i = 0; i < n; i++) {
            for (unsigned int j = 0; j < n; j++) {
                mat._data[i * 2 * n + j] = data[i * n + j];
            }
            mat._data[i * 2 * n + n + i] = 1;
        }

        for (unsigned int k = 0; k < n; k++) {
            unsigned int i(k);
            while (std::fabs(mat._data[i * 2 * n + k]) <
                   kEpsilonPseudoInverse()) {
                i++;
                if (i == n) {
                    throw std::runtime_error("Non-invertible matrix");
                }
            }
            *det *= mat._data[i * 2 * n + k];

            // if found > Exchange lines
            if (i != k) {
                mat.swap_lines(i, k);
            }

            new_mat._data = mat._data;

            for (unsigned int j = 0; j < 2 * n; j++) {
                new_mat._data[k * 2 * n + j] /= mat._data[k * 2 * n + k];
            }
            for (i = 0; i < n; i++) {
                if (i != k) {
                    for (unsigned int j = 0; j < 2 * n; j++) {
                        new_mat._data[i * 2 * n + j] -=
                            mat._data[i * 2 * n + k] *
                            new_mat._data[k * 2 * n + j];
                    }
                }
            }
            mat._data = new_mat._data;
        }

        Matrix<T> *dst = new Matrix<T>(nrows, ncols);
        for (unsigned int i = 0; i < n; i++)
            for (unsigned int j = 0; j < n; j++)
                dst->_data[i * n + j] = mat._data[i * 2 * n + n + j];
        return dst;
    }

    /**
     @brief Swap 2 lines of the matrix
     @param i index of the first line
     @param j index of the second line
     */
    void swap_lines(unsigned int i, unsigned int j) {
        T tmp;
        for (unsigned int k = 0; k < ncols; k++) {
            tmp = data[i * ncols + k];
            data[i * ncols + k] = data[j * ncols + k];
            data[j * ncols + k] = tmp;
        }
    }

    /**
     @brief Swap 2 columns of the matrix
     @param i index of the first column
     @param j index of the second column
     */
    void swap_columns(unsigned int i, unsigned int j) {
        T tmp;
        for (unsigned int k = 0; k < nrows; k++) {
            tmp = data[k * ncols + i];
            data[k * ncols + i] = data[k * ncols + j];
            data[k * ncols + j] = tmp;
        }
    }
};
}

#endif