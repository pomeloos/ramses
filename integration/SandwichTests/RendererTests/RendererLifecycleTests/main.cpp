//  -------------------------------------------------------------------------
//  Copyright (C) 2014 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "RendererTestUtils.h"
#include "Utils/CommandLineParser.h"
#include "Utils/Argument.h"
#include "gmock/gmock.h"

int main(int argc, char *argv[])
{
    ramses_internal::CommandLineParser parser(argc, argv);
    ramses_internal::ArgumentUInt32 waylandIviLayerId(parser, "lid", "waylandIviLayerId", 3);
    RendererTestUtils::SetWaylandIviLayerID(waylandIviLayerId);

    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
