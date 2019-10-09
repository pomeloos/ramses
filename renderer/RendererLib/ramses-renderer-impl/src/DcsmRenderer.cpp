//  -------------------------------------------------------------------------
//  Copyright (C) 2019 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-renderer-api/DcsmRenderer.h"
#include "DcsmRendererImpl.h"
#include "ramses-framework-api/RamsesFramework.h"
#include "ramses-framework-api/DcsmConsumer.h"
#include "DcsmConsumerImpl.h"
#include "DisplayManager/DisplayManager.h"
#include "APILoggingMacros.h"

namespace ramses
{
    DcsmRenderer::DcsmRenderer(RamsesRenderer& renderer, RamsesFramework& framework, const DcsmRendererConfig& config)
        : StatusObject(*new DcsmRendererImpl(config, framework.createDcsmConsumer()->impl, std::make_unique<ramses_display_manager::DisplayManager>(renderer, framework, false)))
        , m_impl(static_cast<DcsmRendererImpl&>(StatusObject::impl))
    {
        LOG_HL_RENDERER_API3(LOG_API_VOID, LOG_API_GENERIC_OBJECT_STRING(renderer), LOG_API_GENERIC_OBJECT_STRING(framework), LOG_API_GENERIC_OBJECT_STRING(config));
    }

    status_t DcsmRenderer::requestContentReady(ContentID contentID, uint64_t timeOut)
    {
        const auto status = m_impl.requestContentReady(contentID, timeOut);
        LOG_HL_RENDERER_API2(status, contentID.getValue(), timeOut);
        return status;
    }

    status_t DcsmRenderer::requestContentReadyAndLinkedViaOffscreenBuffer(ContentID contentID, uint32_t width, uint32_t height, ContentID consumerContentID, dataConsumerId_t consumerDataId, uint64_t timeOut)
    {
        const auto status = m_impl.requestContentReadyAndLinkedViaOffscreenBuffer(contentID, width, height, consumerContentID, consumerDataId, timeOut);
        LOG_HL_RENDERER_API6(status, contentID.getValue(), width, height, consumerContentID.getValue(), consumerDataId, timeOut);
        return status;
    }

    status_t DcsmRenderer::showContent(ContentID contentID, AnimationInformation timingInfo)
    {
        const auto status = m_impl.showContent(contentID, timingInfo);
        LOG_HL_RENDERER_API3(status, contentID.getValue(), timingInfo.startTime, timingInfo.finishTime);
        return status;
    }

    status_t DcsmRenderer::hideContent(ContentID contentID, AnimationInformation timingInfo)
    {
        const auto status = m_impl.hideContent(contentID, timingInfo);
        LOG_HL_RENDERER_API3(status, contentID.getValue(), timingInfo.startTime, timingInfo.finishTime);
        return status;
    }

    status_t DcsmRenderer::releaseContent(ContentID contentID, AnimationInformation timingInfo)
    {
        const auto status = m_impl.releaseContent(contentID, timingInfo);
        LOG_HL_RENDERER_API3(status, contentID.getValue(), timingInfo.startTime, timingInfo.finishTime);
        return status;
    }

    status_t DcsmRenderer::setCategorySize(Category categoryId, SizeInfo size, AnimationInformation timingInfo)
    {
        const auto status = m_impl.setCategorySize(categoryId, size, timingInfo);
        LOG_HL_RENDERER_API5(status, categoryId.getValue(), size.width, size.height, timingInfo.startTime, timingInfo.finishTime);
        return status;
    }

    status_t DcsmRenderer::acceptStopOffer(ContentID contentID, AnimationInformation timingInfo)
    {
        const auto status = m_impl.acceptStopOffer(contentID, timingInfo);
        LOG_HL_RENDERER_API3(status, contentID.getValue(), timingInfo.startTime, timingInfo.finishTime);
        return status;
    }

    status_t DcsmRenderer::update(uint64_t timeStampNow, IDcsmRendererEventHandler& eventHandler, IRendererEventHandler* customRendererEventHandler)
    {
        const auto status = m_impl.update(timeStampNow, eventHandler, customRendererEventHandler);
        LOG_HL_RENDERER_API3(status, timeStampNow, LOG_API_GENERIC_OBJECT_STRING(eventHandler), LOG_API_GENERIC_PTR_STRING(customRendererEventHandler));
        return status;
    }
}
