/*
 * xmm.h
 *
 * XMM - Probabilistic Models for Continuous Motion Recognition and Mapping
 * ============================================================================
 *
 * XMM is a portable, cross-platform C++ library that implements Gaussian
 * Mixture Models and Hidden Markov Models for recognition and regression.
 * The XMM library was developed for movement interaction in creative
 * applications and implements an interactive machine learning workflow with
 * fast training and continuous, real-time inference.
 *
 * Contact:
 * - Jules Francoise: <jules.francoise@ircam.fr>
 *
 *
 * Authors:
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
 * This project is released under the GPLv3 license.
 * For commercial applications, a proprietary license is available upon
 * request to Frederick Rousseau <frederick.rousseau@ircam.fr>.
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
 *
 * Citing this work:
 * If you use this code for research purposes, please cite one of the following
 * publications:
 *
 * - J. Francoise, N. Schnell, R. Borghesi, and F. Bevilacqua,
 *   Probabilistic Models for Designing Motion and Sound Relationships.
 *   In Proceedings of the 2014 International Conference on New Interfaces
 *   for Musical Expression, NIME’14, London, UK, 2014.
 * - J. Francoise, N. Schnell, and F. Bevilacqua, A Multimodal Probabilistic
 *   Model for Gesture-based Control of Sound Synthesis. In Proceedings of the
 *   21st ACM international conference on Multimedia (MM’13), Barcelona,
 *   Spain, 2013.
 */

#ifndef xmm_lib_xmm_h
#define xmm_lib_xmm_h

#include "models/gmm/xmmGmm.hpp"
#include "models/hmm/xmmHierarchicalHmm.hpp"

/**
 @mainpage About
 @anchor mainpage
 @brief XMM is a portable, cross-platform C++ library that implements Gaussian
 Mixture Models and Hidden Markov Models for
 recognition and regression. The XMM library was developed for movement
 interaction in creative applications and implements an
 interactive machine learning workflow with fast training and continuous,
 real-time inference.

 @par Contact
 Jules Francoise: <jules.francoise@ircam.fr>

 @author
 This code has been initially authored by <a
 href="http://julesfrancoise.com">Jules Francoise</a>
 during his PhD thesis, supervised by <a href="frederic-bevilacqua.net">Frederic
 Bevilacqua</a>, in the
 <a href="http://ismm.ircam.fr">Sound Music Movement Interaction</a> team of the
 <a href="http://www.ircam.fr/stms.html?&L=1">STMS Lab</a> - IRCAM - CNRS - UPMC
 (2011-2015).

 @copyright Copyright (C) 2015 UPMC, Ircam-Centre Pompidou.

 @par Licence
 This project is released under the <a
 href="http://www.gnu.org/licenses/gpl-3.0.en.html">GPLv3</a> license.
 For commercial applications, a proprietary license is available upon request to
 Frederick Rousseau <frederick.rousseau@ircam.fr>. @n@n
 XMM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version. @n@n
 XMM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. @n@n
 You should have received a copy of the GNU General Public License
 along with XMM.  If not, see <http://www.gnu.org/licenses/>.

 @par Citing this work
 If you use this code for research purposes, please cite one of the following
 publications:
 - J. Francoise, N. Schnell, R. Borghesi, and F. Bevilacqua, Probabilistic
 Models for Designing Motion and Sound Relationships. In Proceedings of the 2014
 International Conference on New Interfaces for Musical Expression, NIME’14,
 London, UK, 2014.
 - J. Francoise, N. Schnell, and F. Bevilacqua, A Multimodal Probabilistic Model
 for Gesture-based Control of Sound Synthesis. In Proceedings of the 21st ACM
 international conference on Multimedia (MM’13), Barcelona, Spain, 2013.
 */

/**
 @page GettingStarted Getting Started

 @section toc Table of Contents

 1. @subpage Introduction @n
 2. @subpage Compilation @n
 3. @subpage examples_cpp @n
 4. @subpage examples_py @n
*/

#pragma mark -
#pragma mark Introduction

/**
 @page Introduction Introduction
 @tableofcontents

 XMM is a portable, cross-platform C++ library that implements Gaussian Mixture
 Models and Hidden Markov Models for
 both recognition and regression. The XMM library was developed with interaction
 as a central constraint and allows
 for continuous, real-time use of the proposed methods.
 @section why Why another HMM Library?
 Several general machine learning toolkits have become popular over the years,
 such as Weka in Java, Sckits-Learn in Python,
 or more recently MLPack in C++. However, none of the above libraries were
 adapted for the purpose of this thesis. As a matter
 of fact, most HMM implementations are oriented towards classification and they
 often only implement offline inference using
 the Viterbi algorithm.

 In speech processing, the <a href="http://htk.eng.cam.ac.uk/">Hidden Markov
 Model Toolkit (HTK)</a> has now become a standard
 in Automatic Speech Recognition, and gave birth to a branch oriented towards
 synthesis, called <a href="http://hts.sp.nitech.ac.jp/">HTS</a>.
 Both libraries present many features specific to speech synthesis that do not
 yet match our use-cases in movement and sound processing,
 and have a really complex structure that does not facilitate embedding.

 Above all, we did not find any library explicitly implementing the Hierarchical
 HMM, nor the regression methods based on
 GMMs and HMMs. For these reasons, we decided to start of novel implementation
 of these methods with the following
 constraints:
 - __Real-Time:__ Inference must be performed in continuously, meaning that the
 models must update their internal state and prediction
 at each new observation to allow continuous recognition and generation.

 - __Interactive:__ The library must be compatible with an interactive learning
 workflow, that allows users to easily define and edit
 training sets, train models, and evaluate the results through direct
 interaction.
 All models must be able to learn from few examples (possibly a single
 demonstration).

 - __Portable:__ In order to be integrated within various software, platforms,
 the library must be portable, cross-platform, and lightweight.

 We chose C++ that is both efficient and easy to integrate within other software
 and languages such as Max and Python.
 We now detail the four models that are implemented to date, the architecture of
 the library as well as the proposed
 Max/MuBu implementation with several examples.
 @section fourmodels Four Models
 The implemented models are summarized in Table the following table.
 Each of the four model addresses a different combination of the multimodal and
 temporal aspects.
 We implemented two instantaneous models based on Gaussian Mixture Models and
 two temporal models with a hierarchical
 structure, based on an extension of the basic Hidden Markov Model (HMM)
 formalism.
 \ | Movement      | Multimodal
 ------------- | ------------- | -------------
 Instantaneous | Gaussian Mixture Model (GMM) | Gaussian Mixture Regression
 (GMR)
 Temporal  | Hierarchical Hidden Markov Model(HHMM)  | Multimodal Hierarchical
 Hidden Markov Model(MHMM)

 - __Gaussian Mixture Models (GMMs)__ are instantaneous movement models.
 The input data associated to a class defined by the training sets is abstracted
 by a mixture (i.e.\ a weighted sum) of Gaussian distributions.
 This representation allows recognition in the _performance_ phase: for each
 input frame the model calculates the likelihood of each
 class (Figure 1 (__a__)).

 - __Gaussian Mixture Regression (GMR)__ are a straightforward extension of
 Gaussian Mixture Models used for regression.
 Trained with multimodal data, GMR allows for predicting the features of one
 modality (e.g. sound) from the features
 of another (e.g. movement) through non-linear regression between both feature
 sets (Figure 1 (__b__)).

 - __Hierarchical HMM (HHMM)__ integrates a high-level structure that governs
 the transitions between classical HMM structures representing the
 temporal evolution of --- low-level --- movement segments. In the _performance_
 phase of the system, the hierarchical model estimates the likeliest
 gesture according to the transitions defined by the user. The system
 continuously estimates the likelihood for each model, as well as the time
 progression within the original training phrases (Figure 1 (__c__)).

 - __Multimodal Hierarchical HMM (MHMM)__ allows for predicting a stream of
 sound parameters from a stream of movement features.
 It simultaneously takes into account the temporal evolution of movement and
 sound as well as their dynamic relationship according
 to the given example phrases. In this way, it guarantees the temporal
 consistency of the generated sound, while realizing the trained
 temporal movement-sound mappings (Figure 1 (__d__)).

 @image html xmm_models.jpg "Figure 1: Schematic Representation of the 4
 implemented models"

 @section architecture Architecture
 Our implementation has a particular attention to the interactive training
 procedure, and to the respect of the
 real-time constraints of the _performance_ mode.
 The library is built upon four components representing phrases, training sets,
 models and model groups, as represented
 on Figure 2.
 A phrase is a multimodal data container used to store training examples.
 A training set is used to aggregate phrases associated with labels. It provides
 a set of function for interactive
 recording, editing and annotation of the phrases.
 Each instance of a model is connected to a training set that provides access to
 the training phrases.
 Performance functions are designed for real-time usage, updating the internal
 state of the model and the results for
 each new observation of a new movement.
 The library is portable and cross-platform. It defines a specific format for
 exchanging trained models, and provides
 Python bindings for scripting purpose or offline processing.
 @image html xmm_architecture.jpg "Figure 2: Architecture of the XMM library"

 @section relatedpubs Related Publications
 * J. Francoise, N. Schnell, R. Borghesi, and F. Bevilacqua, Probabilistic
 Models for Designing Motion and Sound Relationships. In Proceedings of the 2014
 International Conference on New Interfaces for Musical Expression, NIME’14,
 London, UK, 2014. <a
 href="http://julesfrancoise.com/blog/wp-content/uploads/2014/06/Fran%C3%A7oise-et-al.-2014-Probabilistic-Models-for-Designing-Motion-and-Sound-Relationships.pdf?1ce945">Download</a>

 * J. Francoise, N. Schnell, and F. Bevilacqua, A Multimodal Probabilistic Model
 for Gesture-based Control of Sound Synthesis. In Proceedings of the 21st ACM
 international conference on Multimedia (MM’13), Barcelona, Spain, 2013. <a
 href="http://architexte.ircam.fr/textes/Francoise13b/index.pdf">Download</a>

 <center>Prev: \ref mainpage "Home" | Next: \ref Compilation.</center>
 */

#pragma mark -
#pragma mark Compilation and Usage

/**
 @page Compilation Compilation and Usage
 @tableofcontents

 @section Dependencies
 The library depends on <a
 href="https://github.com/open-source-parsers/jsoncpp">jsoncpp</a> for JSON file
 I/O. The library is distributed with this softare. The units tests rely on the
 <a href="https://github.com/philsquared/Catch">Catch</a> testing framework@n

 @section lib Compiling as a static/dynamic library
 @subsection xcodelib XCode
 See the xcode project in "ide/xcode/"

 @subsection cmakelib CMake
 The library can be built using <a href="http://www.cmake.org/">CMake</a>.@n
 In the root directory, type the following command to generate the Makefiles:
 @code
 cmake . -G"Unix Makefiles"
 @endcode
 The following commands can be used to build the static library, run the unit
 tests, and generate the documentation:
 @code
 make
 make test
 make doc
 @endcode

 @subsection cppusage Usage

 The header file "xmm.h" includes all useful headers of the library.

 @section python Building the Python Library
 @subsection dependencies Dependencies
 - <a href="http://www.doxygen.org/">doxygen</a>
 - <a href="http://www.swig.org/">swig</a>
 - <a href="http://www.numpy.org/">Numpy</a>
 - <a href="http://matplotlib.org/">Matplotlib</a> (for plotting utilities)

 @subsection pythonbuild Building
 The python module can be built using <a
 href="http://www.cmake.org/">CMake</a>.@n
 In the python directory, type the following command to generate the Makefiles
 and build the python module:
 @code
 cmake . -G"Unix Makefiles"
 make
 @endcode
 The module should be installed in "${xmm_root}/python/bin/"

 @subsection usage Usage
 Place the built python library somewhere in your python path. To add personal
 libraries located in '/Path/To/Libs' to the python path, add the following
 lines to your ".bash_profile":

 @code
 PYTHONPATH=$PYTHONPATH:/Path/To/Libs
 export PYTHONPATH
 @endcode

 To import the library in python:
 @code
 >>> import xmm
 @endcode

 Additional utilities can be found in `xmm.util`.
 <center>Prev: \ref Introduction | Next: \ref examples_cpp.</center>
 */

#pragma mark -
#pragma mark Getting Started in C++

/**
 @page examples_cpp Getting Started in C++
 @tableofcontents

 TODO.

 <center>Prev: \ref Compilation | Next: \ref examples_py.</center>

 */

#pragma mark -
#pragma mark Getting Started in Python

/**
 @page examples_py Getting Started in Python
 @tableofcontents

 TODO.

 @section pyexamples Python Examples

 Several examples are included in the ipython notebook IPython Notebook:
 `python/examples/QuickStart_Python.ipynb` @n
 We report here the static html version of the notebook:

 @htmlinclude QuickStart_Python.html
 */

#pragma mark -
#pragma mark Documentation

/**
 @page Documentation Documentation

 This page contains the detailed documentation of the library (generated by
 Doxygen).

 <ul>
 <li><a href="modules.html">Modules</a></li>
 <li><a href="annotated.html">Class List</a></li>
 <li><a href="inherits.html">Class Hierarchy</a></li>
 </ul>

 */

#pragma mark -
#pragma mark Download

/**
 @page Download Download
 @tableofcontents

 @section cpplib C++ Library

 The source code is available on __Github__: https://github.com/Ircam-RnD/xmm

 @section maxmubu Max/MuBu Implementation
 The models are integrated with the <a
 href="http://ismm.ircam.fr/mubu/">MuBu</A> environment within
 <a href="http://cycling74.com/">Cycling 74 Max</a> that provides a consistent
 framework for motion/sound feature extraction
 and pre-processing; interactive recording, editing, and annotation of the
 training sets; and interactive sound synthesis.
 MuBu is freely available on <a
 href="http://forumnet.ircam.fr/product/mubu/">Ircam's Forumnet</a>.

 Max is a visual programming environment dedicated to music and interactive
 media.
 We provide an implementation of our library as a set of Max externals and
 abstractions articulated around the _MuBu_ collection of objects developed at
 Ircam.
 Training sets are built using _MuBu_, a generic container designed to store and
 process multimodal data such as audio, motion tracking data, sound descriptors,
 markers, etc.
 Each training phrase is stored in a buffer of the container, and movement and
 sound parameters are recorded into separate tracks of each buffer.
 Markers can be used to specify regions of interest within the recorded
 examples.
 Phrases are labeled using the markers or as an attribute of the buffer.
 This structure allows users to quickly record, modify, and annotate the
 training examples.
 Training sets are thus autonomous and can be used to train several models.

 Each model can be instantiated as a max object referring to a MuBu container
 that defines its training set.
 For training, the model connects to the container and transfers the training
 examples to its internal representation of phrases.
 The parameters of the model can be set manually as attributes of the object,
 such as the number of Gaussian components in the case of a GMM, or the number
 of states in the case of a HMM.
 The training is performed in background.

 For performance, each object processes an input stream of movement features and
 updates the results with the same rate.
 For movement models, the object output the list of likelihoods, complemented
 with the parameters estimated for each class, such as the time progression in
 the case of a temporal model, or the weight of each Gaussian component in the
 case of a GMM.
 For multimodal models, the object also outputs the most probable sound
 parameters estimated by the model, that can be directly used to drive the sound
 synthesis.

 @section ofx_addon OpenFrameworks Addon

 Coming Soon...

 */

#endif
