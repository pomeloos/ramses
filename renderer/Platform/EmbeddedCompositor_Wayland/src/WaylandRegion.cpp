//  -------------------------------------------------------------------------
//  Copyright (C) 2017 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "EmbeddedCompositor_Wayland/WaylandRegion.h"
#include "EmbeddedCompositor_Wayland/WaylandClient.h"
#include "EmbeddedCompositor_Wayland/IWaylandResource.h"
#include "EmbeddedCompositor_Wayland/IEmbeddedCompositor_Wayland.h"
#include "Utils/LogMacros.h"

namespace ramses_internal
{
    WaylandRegion::WaylandRegion(IEmbeddedCompositor_Wayland& compositor,
                                 IWaylandClient&              client,
                                 uint32_t                     version,
                                 uint32_t                     id)
        : m_compositor(compositor)
    {
        LOG_TRACE(CONTEXT_RENDERER, "WaylandRegion::WaylandRegion");

        m_resource = client.resourceCreate(&wl_region_interface, version, id);
        if (nullptr != m_resource)
        {
            m_resource->setImplementation(&m_regionInterface, this, ResourceDestroyedCallback);
        }
        else
        {
            LOG_ERROR(CONTEXT_RENDERER, "WaylandRegion::WaylandRegion Could not create wayland region!");
            client.postNoMemory();
        }

        m_compositor.addWaylandRegion(*this);
    }

    WaylandRegion::~WaylandRegion()
    {
        LOG_TRACE(CONTEXT_RENDERER, "WaylandRegion::~WaylandRegion");

        m_compositor.removeWaylandRegion(*this);
        if (nullptr != m_resource)
        {
            // Remove ResourceDestroyedCallback
            m_resource->setImplementation(&wl_region_interface, this, nullptr);
            delete m_resource;
        }
    }

    void WaylandRegion::resourceDestroyed()
    {
        LOG_TRACE(CONTEXT_RENDERER, "WaylandRegion::resourceDestroyed");

        // wl_resource is destroyed outside by the Wayland library, so m_resource loses the ownership of the
        // Wayland resource, so that we don't call wl_resource_destroy.
        m_resource->disownWaylandResource();
    }

    void WaylandRegion::regionAdd(IWaylandClient& client, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        LOG_TRACE(CONTEXT_RENDERER, "WaylandRegion::regionAdd x: " << x << " y: " << y << " width: " << width << " height: " << height);

        UNUSED(client)
    }

    void WaylandRegion::regionSubtract(IWaylandClient& client, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        LOG_TRACE(CONTEXT_RENDERER,
                  "WaylandRegion::regionSubtract x: " << x << " y: " << y << " width: " << width << " height: " << height);

        UNUSED(client)
    }

    void WaylandRegion::RegionDestroyCallback(wl_client* client, wl_resource* regionResource)
    {
        UNUSED(client)
        WaylandRegion* region = static_cast<WaylandRegion*>(wl_resource_get_user_data(regionResource));
        delete region;
    }

    void WaylandRegion::RegionAddCallback(
        wl_client* client, wl_resource* regionResource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        WaylandRegion* region = static_cast<WaylandRegion*>(wl_resource_get_user_data(regionResource));
        WaylandClient  waylandClient(client);
        region->regionAdd(waylandClient, x, y, width, height);
    }

    void WaylandRegion::RegionSubtractCallback(
        wl_client* client, wl_resource* regionResource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        WaylandRegion* region = static_cast<WaylandRegion*>(wl_resource_get_user_data(regionResource));
        WaylandClient  waylandClient(client);
        region->regionSubtract(waylandClient, x, y, width, height);
    }

    void WaylandRegion::ResourceDestroyedCallback(wl_resource* regionResource)
    {
        WaylandRegion* region = static_cast<WaylandRegion*>(wl_resource_get_user_data(regionResource));
        region->resourceDestroyed();
        delete region;
    }
}
