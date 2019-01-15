/*
 * xmmTrainingSet.cpp
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
#include "xmmTrainingSet.hpp"
#include <limits>
#include <algorithm>

xmm::TrainingSet::TrainingSet(MemoryMode memoryMode,
                              Multimodality multimodality)
    : own_memory_(memoryMode == MemoryMode::OwnMemory),
      bimodal_(multimodality == Multimodality::Bimodal) {
    dimension.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);
    dimension_input.onAttributeChange(this,
                                      &xmm::TrainingSet::onAttributeChange);
    column_names.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);

    dimension.setLimitMin((bimodal_) ? 2 : 1);
    dimension.set((bimodal_) ? 2 : 1, true);
    dimension_input.setLimits((bimodal_) ? 1 : 0, (bimodal_) ? 2 : 0);
    dimension_input.set((bimodal_) ? 1 : 0, true);
    column_names.resize(dimension.get());
}

xmm::TrainingSet::TrainingSet(TrainingSet const &src)
    : dimension(src.dimension),
      dimension_input(src.dimension_input),
      column_names(src.column_names),
      own_memory_(src.own_memory_),
      bimodal_(src.bimodal_),
      labels_(src.labels_),
      phrases_(src.phrases_) {
    dimension.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);
    dimension_input.onAttributeChange(this,
                                      &xmm::TrainingSet::onAttributeChange);
    column_names.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);
    for (auto phrase : src.phrases_) {
        phrases_.insert(std::pair<int, std::shared_ptr<xmm::Phrase>>(
            phrase.first, std::make_shared<Phrase>(*phrase.second)));
        phrases_[phrase.first]->events.addListener(
            this, &xmm::TrainingSet::onPhraseEvent);
    }
    update();
}

xmm::TrainingSet::TrainingSet(Json::Value const &root)
    : own_memory_(true), bimodal_(false) {
    if (!own_memory_)
        throw std::runtime_error("Cannot read Training Set with Shared memory");

    dimension.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);
    dimension_input.onAttributeChange(this,
                                      &xmm::TrainingSet::onAttributeChange);
    column_names.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);

    bimodal_ = root.get("bimodal", false).asBool();

    dimension.setLimitMin((bimodal_) ? 2 : 1);
    dimension_input.setLimits((bimodal_) ? 1 : 0, (bimodal_) ? 2 : 0);
    dimension.set(root.get("dimension", bimodal_ ? 2 : 1).asInt());
    dimension_input.set(root.get("dimension_input", bimodal_ ? 1 : 0).asInt());
    std::vector<std::string> tmpColNames(dimension.get());
    for (int i = 0; i < root["column_names"].size(); i++) {
        tmpColNames[i] = root["column_names"].get(i, "").asString();
    }
    column_names.set(tmpColNames);

    // Get Phrases
    phrases_.clear();
    for (auto p : root["phrases"]) {
        phrases_.insert(std::pair<int, std::shared_ptr<xmm::Phrase>>(
            p["index"].asInt(), std::make_shared<Phrase>(p)));
        phrases_[p["index"].asInt()]->events.addListener(
            this, &xmm::TrainingSet::onPhraseEvent);
    }
    update();
}

xmm::TrainingSet &xmm::TrainingSet::operator=(TrainingSet const &src) {
    if (this != &src) {
        clear();
        own_memory_ = src.own_memory_;
        bimodal_ = src.bimodal_;
        dimension = src.dimension;
        dimension_input = src.dimension_input;
        column_names = src.column_names;
        labels_ = src.labels_;
        dimension.onAttributeChange(this, &xmm::TrainingSet::onAttributeChange);
        dimension_input.onAttributeChange(this,
                                          &xmm::TrainingSet::onAttributeChange);
        column_names.onAttributeChange(this,
                                       &xmm::TrainingSet::onAttributeChange);
        for (auto phrase : src.phrases_) {
            phrases_.insert(std::pair<int, std::shared_ptr<xmm::Phrase>>(
                phrase.first, std::make_shared<Phrase>(*phrase.second)));
            phrases_[phrase.first]->events.addListener(
                this, &xmm::TrainingSet::onPhraseEvent);
        }
        update();
    }
    return *this;
}

xmm::TrainingSet::~TrainingSet() {}

bool xmm::TrainingSet::ownMemory() const { return own_memory_; }

bool xmm::TrainingSet::bimodal() const { return bimodal_; }

bool xmm::TrainingSet::empty() const { return phrases_.empty(); }

unsigned int xmm::TrainingSet::size() const {
    return static_cast<unsigned int>(phrases_.size());
}

void xmm::TrainingSet::onAttributeChange(xmm::AttributeBase *attr_pointer) {
    if (attr_pointer == &dimension) {
        for (auto &phrase : phrases_) {
            phrase.second->dimension.set(dimension.get());
        }
        for (std::map<std::string, TrainingSet>::iterator it =
                 sub_training_sets_.begin();
             it != sub_training_sets_.end(); ++it)
            it->second.dimension.set(dimension.get(), true);

        column_names.resize(dimension.get());
        dimension_input.setLimitMax(dimension.get() - 1);
    }
    if (attr_pointer == &dimension_input) {
        for (auto &phrase : phrases_) {
            phrase.second->dimension_input.set(dimension_input.get());
        }
        for (std::map<std::string, TrainingSet>::iterator it =
                 sub_training_sets_.begin();
             it != sub_training_sets_.end(); ++it)
            it->second.dimension_input.set(dimension_input.get(), true);
    }
    if (attr_pointer == &column_names) {
        for (auto &phrase : phrases_) {
            phrase.second->column_names = column_names.get();
        }

        for (std::map<std::string, TrainingSet>::iterator it =
                 sub_training_sets_.begin();
             it != sub_training_sets_.end(); ++it)
            it->second.column_names.set(column_names.get(), true);
    }
    attr_pointer->changed = false;
}

std::map<int, std::shared_ptr<xmm::Phrase>>::iterator
xmm::TrainingSet::begin() {
    return phrases_.begin();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::iterator xmm::TrainingSet::end() {
    return phrases_.end();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::reverse_iterator
xmm::TrainingSet::rbegin() {
    return phrases_.rbegin();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::reverse_iterator
xmm::TrainingSet::rend() {
    return phrases_.rend();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::const_iterator
xmm::TrainingSet::cbegin() const {
    return phrases_.cbegin();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::const_iterator
xmm::TrainingSet::cend() const {
    return phrases_.cend();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::const_reverse_iterator
xmm::TrainingSet::crbegin() const {
    return phrases_.crbegin();
}

std::map<int, std::shared_ptr<xmm::Phrase>>::const_reverse_iterator
xmm::TrainingSet::crend() const {
    return phrases_.crend();
}

std::shared_ptr<xmm::Phrase> xmm::TrainingSet::getPhrase(int n) const {
    if (phrases_.count(n) == 0) return nullptr;
    return phrases_.at(n);
}

void xmm::TrainingSet::addPhrase(int phraseIndex, std::string label) {
    phrases_[phraseIndex] = std::make_shared<Phrase>(
        own_memory_ ? MemoryMode::OwnMemory : MemoryMode::SharedMemory,
        bimodal_ ? Multimodality::Bimodal : Multimodality::Unimodal);
    phrases_[phraseIndex]->events.addListener(this,
                                              &xmm::TrainingSet::onPhraseEvent);
    phrases_[phraseIndex]->dimension.set(dimension.get());
    phrases_[phraseIndex]->dimension_input.set(dimension_input.get());
    phrases_[phraseIndex]->column_names = this->column_names.get();
    phrases_[phraseIndex]->label.set(label, true);
    update();
}

void xmm::TrainingSet::addPhrase(int phraseIndex, Phrase const &phrase) {
    phrases_[phraseIndex] = std::make_shared<Phrase>(phrase);
    phrases_[phraseIndex]->events.removeListeners();
    phrases_[phraseIndex]->events.addListener(this,
                                              &xmm::TrainingSet::onPhraseEvent);
    update();
}

void xmm::TrainingSet::addPhrase(int phraseIndex,
                                 std::shared_ptr<Phrase> phrase) {
    phrases_[phraseIndex] = phrase;
    phrases_[phraseIndex]->events.addListener(this,
                                              &xmm::TrainingSet::onPhraseEvent);
    update();
}

void xmm::TrainingSet::removePhrase(int phraseIndex) {
    phrases_.erase(phraseIndex);
    update();
}

void xmm::TrainingSet::removePhrasesOfClass(std::string const &label) {
    bool contLoop(true);
    while (contLoop) {
        contLoop = false;
        for (auto &phrase : phrases_) {
            if (phrase.second->label.get() == label) {
                removePhrase(phrase.first);
                contLoop = true;
                break;
            }
        }
    }
    update();
}

void xmm::TrainingSet::clear() {
    sub_training_sets_.clear();
    phrases_.clear();
    labels_.clear();
}

xmm::TrainingSet *xmm::TrainingSet::getPhrasesOfClass(
    std::string const &label) {
    std::map<std::string, TrainingSet>::iterator it =
        sub_training_sets_.find(label);
    if (it == sub_training_sets_.end()) return nullptr;
    return &(it->second);
}

void xmm::TrainingSet::onPhraseEvent(PhraseEvent const &e) {
    if (e.type == PhraseEvent::Type::LabelChanged) {
        update();
    }
}

void xmm::TrainingSet::update() {
    labels_.clear();
    for (auto &phrase : phrases_) {
        labels_.insert(phrase.second->label.get());
    }
    sub_training_sets_.clear();
    for (std::string label : labels_) {
        sub_training_sets_.insert(std::pair<std::string, TrainingSet>(
            label,
            {own_memory_ ? MemoryMode::OwnMemory : MemoryMode::SharedMemory,
             bimodal_ ? Multimodality::Bimodal : Multimodality::Unimodal}));
        sub_training_sets_[label].dimension.set(dimension.get());
        sub_training_sets_[label].dimension_input.set(dimension_input.get());
        sub_training_sets_[label].column_names = this->column_names;
        // int newPhraseIndex(0);
        for (auto &phrase : phrases_) {
            if (phrase.second->label.get() == label) {
                sub_training_sets_[label].phrases_[phrase.first] =
                    this->phrases_[phrase.first];
                //                newPhraseIndex++;
            }
        }
    }
}

std::vector<float> xmm::TrainingSet::mean() const {
    std::vector<float> mean(dimension.get(), 0.0);
    unsigned int total_length(0);
    for (auto &phrase : phrases_) {
        for (unsigned int d = 0; d < dimension.get(); d++) {
            for (unsigned int t = 0; t < phrase.second->size(); t++) {
                mean[d] += phrase.second->getValue(t, d);
            }
        }
        total_length += phrase.second->size();
    }

    for (unsigned int d = 0; d < dimension.get(); d++)
        mean[d] /= float(total_length);

    return mean;
}

std::vector<float> xmm::TrainingSet::standardDeviation() const {
    std::vector<float> stddev(dimension.get());
    std::vector<float> _mean = mean();
    unsigned int total_length(0);
    for (auto &phrase : phrases_) {
        for (unsigned int d = 0; d < dimension.get(); d++) {
            for (unsigned int t = 0; t < phrase.second->size(); t++) {
                stddev[d] += (phrase.second->getValue(t, d) - _mean[d]) *
                             (phrase.second->getValue(t, d) - _mean[d]);
            }
        }
        total_length += phrase.second->size();
    }

    for (unsigned int d = 0; d < dimension.get(); d++) {
        stddev[d] /= float(total_length);
        stddev[d] = sqrtf(stddev[d]);
    }

    return stddev;
}

std::vector<std::pair<float, float>> xmm::TrainingSet::minmax() const {
    std::vector<std::pair<float, float>> minmax(
        dimension.get(), {std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::lowest()});
    if (bimodal_) {
        for (auto &phrase : phrases_) {
            for (unsigned int d = 0; d < dimension_input.get(); d++) {
                for (unsigned int t = 0; t < phrase.second->inputSize(); t++) {
                    minmax[d].first = std::min(phrase.second->getValue(t, d),
                                               minmax[d].first);
                    minmax[d].second = std::max(phrase.second->getValue(t, d),
                                                minmax[d].second);
                }
            }
            for (unsigned int d = dimension_input.get(); d < dimension.get();
                 d++) {
                for (unsigned int t = 0; t < phrase.second->outputSize(); t++) {
                    minmax[d].first = std::min(phrase.second->getValue(t, d),
                                               minmax[d].first);
                    minmax[d].second = std::max(phrase.second->getValue(t, d),
                                                minmax[d].second);
                }
            }
        }
    } else {
        for (auto &phrase : phrases_) {
            for (unsigned int d = 0; d < dimension.get(); d++) {
                for (unsigned int t = 0; t < phrase.second->size(); t++) {
                    minmax[d].first = std::min(phrase.second->getValue(t, d),
                                               minmax[d].first);
                    minmax[d].second = std::max(phrase.second->getValue(t, d),
                                                minmax[d].second);
                }
            }
        }
    }
    return minmax;
}

void xmm::TrainingSet::rescale(std::vector<float> offset,
                               std::vector<float> gain) {
    for (auto &phrase : phrases_) {
        phrase.second->rescale(offset, gain);
    }
}

void xmm::TrainingSet::normalize() {
    std::vector<float> offset = mean();
    std::vector<float> gain = standardDeviation();
    for (auto &v : gain) v = 1. / v;
    rescale(offset, gain);
}

Json::Value xmm::TrainingSet::toJson() const {
    Json::Value root;
    root["bimodal"] = bimodal_;
    root["dimension"] = static_cast<int>(dimension.get());
    root["dimension_input"] = static_cast<int>(dimension_input.get());
    for (int i = 0; i < dimension.get(); i++)
        root["column_names"][i] = column_names.at(i);

    // Add Phrases
    root["phrases"].resize(static_cast<Json::ArrayIndex>(size()));
    Json::ArrayIndex phraseIndex(0);
    for (auto &phrase : phrases_) {
        root["phrases"][phraseIndex] = phrase.second->toJson();
        root["phrases"][phraseIndex]["index"] = phrase.first;
        phraseIndex++;
    }

    return root;
}

void xmm::TrainingSet::fromJson(Json::Value const &root) {
    try {
        TrainingSet tmp(root);
        *this = tmp;
    } catch (JsonException &e) {
        throw e;
    }
}
