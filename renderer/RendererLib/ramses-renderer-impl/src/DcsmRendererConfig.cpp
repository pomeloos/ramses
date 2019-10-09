//  -------------------------------------------------------------------------
//  Copyright (C) 2019 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-renderer-api/DcsmRendererConfig.h"
#include "DcsmRendererConfigImpl.h"
#include "APILoggingMacros.h"

namespace ramses
{
    DcsmRendererConfig::DcsmRendererConfig()
        : DcsmRendererConfig({})
    {
        LOG_HL_RENDERER_API_NOARG(LOG_API_VOID);
    }

    DcsmRendererConfig::DcsmRendererConfig(std::initializer_list<std::pair<Category, CategoryInfo>> categories)
        : StatusObject(*new DcsmRendererConfigImpl(categories))
        , m_impl(static_cast<DcsmRendererConfigImpl&>(StatusObject::impl))
    {
        for (const auto& c : categories)
        {
            LOG_HL_RENDERER_API4(LOG_API_VOID, c.first.getValue(), c.second.size.width, c.second.size.height, c.second.display);
        }
    }

    status_t DcsmRendererConfig::addCategory(Category categoryId, const CategoryInfo& categoryInfo)
    {
        const auto status = m_impl.addCategory(categoryId, categoryInfo);
        LOG_HL_RENDERER_API4(status, categoryId.getValue(), categoryInfo.size.width, categoryInfo.size.height, categoryInfo.display);
        return status;
    }

    const DcsmRendererConfig::CategoryInfo* DcsmRendererConfig::findCategoryInfo(Category categoryId) const
    {
        return m_impl.findCategoryInfo(categoryId);
    }
}
