//  -------------------------------------------------------------------------
//  Copyright (C) 2012 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "RendererLib/DisplayEventHandlerManager.h"
#include "RendererLib/DisplayEventHandler.h"
#include "RendererLib/Renderer.h"

namespace ramses_internal
{
    DisplayEventHandlerManager::DisplayEventHandlerManager(RendererEventCollector& eventCollector)
        : m_eventCollector(eventCollector)
    {
    }

    DisplayEventHandlerManager::~DisplayEventHandlerManager()
    {
        // destroy all handlers
        for(const auto& displayHandler : m_displayHandlers)
        {
            delete displayHandler.value;
        }
    }

    DisplayEventHandler& DisplayEventHandlerManager::createHandler(DisplayHandle display)
    {
        assert(!m_displayHandlers.contains(display));

        DisplayEventHandler* handler = new DisplayEventHandler(display, m_eventCollector);
        m_displayHandlers.put(display, handler);

        return *handler;
    }

    void DisplayEventHandlerManager::destroyHandler(DisplayHandle display)
    {
        assert(m_displayHandlers.contains(display));

        DisplayEventHandler* oldHandler = nullptr;
        m_displayHandlers.remove(display, &oldHandler);
        delete oldHandler;
    }

    DisplayEventHandler& DisplayEventHandlerManager::getHandler(DisplayHandle display)
    {
        assert(m_displayHandlers.contains(display));
        return **m_displayHandlers.get(display);
    }
}
