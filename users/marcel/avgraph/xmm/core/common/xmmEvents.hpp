/*
 * xmmEvents.hpp
 *
 * Template classes for Event generators
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

#ifndef xmmEvents_h
#define xmmEvents_h

#include <functional>
#include <map>
#include <set>

namespace xmm {
/**
 @ingroup Common
 @brief Generator class for a specific type of events
 @tparam EventType Type of Events that can be sent
 */
template <typename EventType>
class EventGenerator {
  public:
    typedef std::function<void(EventType&)> EventCallBack;

    /**
     @brief Default constructor
     */
    EventGenerator() {}

    /**
     @brief Destructor
     @details Also notifies all listeners that this generators is deleted
     */
    virtual ~EventGenerator() {}

    /**
     @brief Adds a listener object to be notified when events are sent
     @param owner Pointer to the listener object
     @param listenerMethod Method to be called in the target class
     @tparam U type of the target object
     @tparam args callback arguments
     @tparam ListenerClass Listener Class
     */
    template <typename U, typename args, class ListenerClass>
    void addListener(U* owner, void (ListenerClass::*listenerMethod)(args)) {
        callbacks_.insert(std::pair<void*, EventCallBack>(
            static_cast<void*>(owner),
            std::bind(listenerMethod, owner, std::placeholders::_1)));
    }

    /**
     @brief Removes a listener object
     @param owner Pointer to the listener object
     @param listenerMethod Method to be called in the target class
     @tparam U type of the target object
     @tparam args callback arguments
     @tparam ListenerClass Listener Class
     */
    template <typename U, typename args, class ListenerClass>
    void removeListener(U* owner, void (ListenerClass::*listenerMethod)(args)) {
        callbacks_.erase(static_cast<void*>(owner));
    }

    /**
     @brief Removes all listeners
     */
    void removeListeners() { callbacks_.clear(); }

    /**
     @brief Propagates the event to all listeners
     @param e event
     */
    void notifyListeners(EventType& e) const {
        for (auto& callback : callbacks_) {
            callback.second(e);
        }
    }

  private:
    /**
     @brief Set of listener objects
     */
    std::map<void*, EventCallBack> callbacks_;
};
}

#endif
