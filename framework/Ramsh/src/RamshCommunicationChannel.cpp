//  -------------------------------------------------------------------------
//  Copyright (C) 2014 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "Ramsh/RamshCommunicationChannel.h"

namespace ramses_internal
{
    RamshCommunicationChannel::~RamshCommunicationChannel()
    {
    }

    RamshCommunicationChannel::RamshCommunicationChannel()
        : m_ramsh(nullptr)
    {

    }

    void RamshCommunicationChannel::registerRamsh(Ramsh& ramsh)
    {
        m_ramsh = &ramsh;
    }
}
