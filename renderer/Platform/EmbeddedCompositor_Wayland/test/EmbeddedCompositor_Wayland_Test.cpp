//  -------------------------------------------------------------------------
//  Copyright (C) 2017 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"
#include "TestWithWaylandEnvironment.h"
#include "RendererLib/RendererConfig.h"
#include "EmbeddedCompositor_Wayland/EmbeddedCompositor_Wayland.h"
#include "PlatformFactoryMock.h"
#include "Platform_Base/PlatformFactory_Base.h"
#include "Platform_Wayland_IVI_EGL_ES_3_0/Platform_Wayland_IVI_EGL_ES_3_0.h"
#include "WaylandUtilities/UnixDomainSocket.h"
#include "WaylandUtilities/WaylandEnvironmentUtils.h"
#include "PlatformAbstraction/PlatformThread.h"
#include "Collections/StringOutputStream.h"
#include "WaylandSurfaceMock.h"
#include "WaylandBufferMock.h"
#include "WaylandCompositorConnectionMock.h"
#include "WaylandRegionMock.h"
#include "WaylandBufferResourceMock.h"
#include "EmbeddedCompositor_Wayland/WaylandBuffer.h"
#include "wayland-client.h"
#include <pwd.h>
#include <grp.h>
#include <atomic>

namespace ramses_internal
{
    using namespace testing;

    namespace
    {
        Bool canDisplayConnectToCompositor(wl_display* display)
        {
            if(display == nullptr)
            {
                return false;
            }

            return (wl_display_roundtrip(display) >= 0);
        }


        Bool isSocket(int fd)
        {
            if (fd < 0)
            {
                return false;
            }

            struct stat buf;
            if (fstat(fd, &buf) < 0)
            {
                return false;
            }

            return S_ISSOCK(buf.st_mode);
        }

        ramses_internal::String getUserGroupName()
        {
            passwd* pws = getpwuid(geteuid());
            if (pws)
            {
                group* group = getgrgid(pws->pw_gid);
                if (group)
                {
                    return group->gr_name;
                }
            }
            return "";
        }


        class ConnectToDisplayRunnable : public Runnable
        {
        public:
            ConnectToDisplayRunnable(int clientFileDescriptor)
            : m_clientSocketFileDescriptor(clientFileDescriptor)
            , m_result(false)
            , m_started(false)
            , m_ended(false)
            {
            }

            ConnectToDisplayRunnable(const String& clientFileName)
            : m_clientSocketFileName(clientFileName)
            , m_result(false)
            , m_started(false)
            , m_ended(false)
            {
            }

            virtual void run() override
            {
                m_started = true;
                wl_display* display = (m_clientSocketFileDescriptor>=0) ?
                    wl_display_connect_to_fd(m_clientSocketFileDescriptor) :
                    wl_display_connect(m_clientSocketFileName.c_str());

                m_result = canDisplayConnectToCompositor( display);

                // cleanup display and free resources
                if (display)
                {
                    wl_display_disconnect(display);
                }

                m_ended = true;
            }

            Bool couldConnectToEmbeddedCompositor() const
            {
                return m_result;
            }

            Bool hasStarted() const
            {
                return m_started;
            }

            Bool hasEnded() const
            {
                return m_ended;
            }

        private:
            int          m_clientSocketFileDescriptor = -1;
            String       m_clientSocketFileName;
            std::atomic<Bool> m_result;
            std::atomic<Bool> m_started;
            std::atomic<Bool> m_ended;
        };
    }





    IPlatformFactory* PlatformFactory_Base::CreatePlatformFactory(const RendererConfig&)
    {
        return new ::testing::NiceMock<PlatformFactoryNiceMock>();
    }

    class AEmbeddedCompositor_Wayland : public TestWithWaylandEnvironment
    {
    public:
        AEmbeddedCompositor_Wayland()
        {
            embeddedCompositor = new EmbeddedCompositor_Wayland(rendererConfig);
        }

        void init()
        {
            WaylandEnvironmentUtils::SetVariable(WaylandEnvironmentVariable::XDGRuntimeDir, m_initialValueOfXdgRuntimeDir);
            const String socketName("wayland-10");
            rendererConfig.setWaylandSocketEmbedded(socketName);
            EXPECT_TRUE(embeddedCompositor->init());
        }

        virtual ~AEmbeddedCompositor_Wayland()
        {
            delete(embeddedCompositor);
        }


        Bool clientCanConnectViaSocket(const String& socketName)
        {
            ConnectToDisplayRunnable client(socketName);
            runClientAndWaitForThreadJoining(client);
            return client.couldConnectToEmbeddedCompositor();
        }

        Bool clientCanConnectViaSocket(int socketFD)
        {
            ConnectToDisplayRunnable client(socketFD);
            runClientAndWaitForThreadJoining(client);
            return client.couldConnectToEmbeddedCompositor();
        }


    protected:

        RendererConfig              rendererConfig     = RendererConfig();
        EmbeddedCompositor_Wayland* embeddedCompositor = nullptr;
        UnixDomainSocket            socket             = UnixDomainSocket("testingSocket", m_initialValueOfXdgRuntimeDir);

    private:

        void runClientAndWaitForThreadJoining(ConnectToDisplayRunnable& client)
        {
            PlatformThread clientThread("ClientApp");
            clientThread.start(client);

            while(!client.hasStarted())
            {
                PlatformThread::Sleep(10);
            }
            while(!client.hasEnded())
            {
                embeddedCompositor->handleRequestsFromClients();
                embeddedCompositor->endFrame(true);
                PlatformThread::Sleep(10);
            }
        }
    };

    TEST_F(AEmbeddedCompositor_Wayland, CanBeCreatedAndDestroyed)
    {
        EXPECT_NE(nullptr, embeddedCompositor);
    }

    TEST_F(AEmbeddedCompositor_Wayland, DefaultRenderConfigCanNotInitialize)
    {
        EXPECT_FALSE(embeddedCompositor->init());
        EXPECT_FALSE(clientCanConnectViaSocket("wayland-10"));
    }

    TEST_F(AEmbeddedCompositor_Wayland, InitializeWorksWithSocketNameSet_ClientConnectionTest)
    {
        init();
        EXPECT_TRUE(clientCanConnectViaSocket("wayland-10"));
    }

    TEST_F(AEmbeddedCompositor_Wayland, InitializeWorksWithSocketNameAndGroupSet_ClientConnectionTest)
    {
        WaylandEnvironmentUtils::SetVariable(WaylandEnvironmentVariable::XDGRuntimeDir, m_initialValueOfXdgRuntimeDir);

        const String socketName("wayland-10");
        const String groupName = getUserGroupName();
        LOG_INFO(CONTEXT_RENDERER, "InitializeWorksWithSocketNameAndGroupSet groupName: " << groupName);
        rendererConfig.setWaylandSocketEmbedded(socketName);
        rendererConfig.setWaylandSocketEmbeddedGroup(groupName);

        EXPECT_TRUE(embeddedCompositor->init());

        EXPECT_TRUE(clientCanConnectViaSocket(socketName));
    }

    TEST_F(AEmbeddedCompositor_Wayland, InitializeFailsWithSocketNameAndWrongGroupSet)
    {
        const String socketName("wayland-10");
        rendererConfig.setWaylandSocketEmbedded(socketName);
        rendererConfig.setWaylandSocketEmbeddedGroup("notExistingGroupName");
        EXPECT_FALSE(embeddedCompositor->init());
    }

    TEST_F(AEmbeddedCompositor_Wayland, InitializeWorksWithSocketFDSet_ClientConnectionTest)
    {
        const int socketFD = socket.createBoundFileDescriptor();
        const int clientFD = socket.createConnectedFileDescriptor(false);
        ASSERT_TRUE(isSocket(socketFD));
        ASSERT_TRUE(isSocket(clientFD));

        rendererConfig.setWaylandSocketEmbeddedFD(socketFD);
        EXPECT_TRUE(embeddedCompositor->init());

        EXPECT_TRUE(clientCanConnectViaSocket(clientFD));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanNotInitializeWithWrongSocketFD)
    {
        const int nonExistentSocketFD = socket.createBoundFileDescriptor() + 3;
        ASSERT_FALSE(isSocket(nonExistentSocketFD));
        rendererConfig.setWaylandSocketEmbeddedFD(nonExistentSocketFD);

        EXPECT_FALSE(embeddedCompositor->init());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanNotInitializeWithBothSocketsConfigured)
    {
        const String socketName("wayland-10");
        const int socketFD = socket.createBoundFileDescriptor();
        ASSERT_TRUE(isSocket(socketFD));

        rendererConfig.setWaylandSocketEmbedded(socketName);
        rendererConfig.setWaylandSocketEmbeddedFD(socketFD);
        EXPECT_FALSE(embeddedCompositor->init());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanNotInitializeWithSocketNameSetButXDGRuntimeDirNotSet)
    {
        const String socketName("wayland-10");
        rendererConfig.setWaylandSocketEmbedded(socketName);

        WaylandEnvironmentUtils::UnsetVariable(WaylandEnvironmentVariable::XDGRuntimeDir);

        EXPECT_FALSE(embeddedCompositor->init());
        EXPECT_FALSE(clientCanConnectViaSocket(socketName));
    }

    TEST_F(AEmbeddedCompositor_Wayland, InitializeWorksWithSocketFDSetEvenWithoutXDGRuntimeDirNotSet_ClientConnectionTest)
    {
        const int socketFD = socket.createBoundFileDescriptor();
        const int clientFD = socket.createConnectedFileDescriptor(false);
        ASSERT_TRUE(isSocket(socketFD));
        ASSERT_TRUE(isSocket(clientFD));

        // the SocketEmbeddedFD is the socket the EC is using for incomming
        // connections from different clients
        rendererConfig.setWaylandSocketEmbeddedFD(socketFD);

        UnixDomainSocket systemCompositorSocket("wayland-0", m_initialValueOfXdgRuntimeDir);
        StringOutputStream systemCompositorSocketFD;
        systemCompositorSocketFD << systemCompositorSocket.createConnectedFileDescriptor(false);

        // The EC needs to connect to the system compositor (it is no real server just acting as proxy)
        // So we need to configure the socket information to Wayland
        WaylandEnvironmentUtils::SetVariable(WaylandEnvironmentVariable::WaylandSocket, systemCompositorSocketFD.c_str());
        WaylandEnvironmentUtils::UnsetVariable(WaylandEnvironmentVariable::XDGRuntimeDir);
        EXPECT_TRUE(embeddedCompositor->init());

        EXPECT_TRUE(clientCanConnectViaSocket(clientFD));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanAddWaylandSurfaceWithCheckHasSurfaceForStreamTexture)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_TRUE(embeddedCompositor->hasSurfaceForStreamTexture(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanRemoveWaylandSurfaceWithCheckHasSurfaceForStreamTexture)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        embeddedCompositor->removeWaylandSurface(surface);
        EXPECT_FALSE(embeddedCompositor->hasSurfaceForStreamTexture(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanGetGetTitleOfWaylandIviSurface)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);
        const String title = "someTitle";

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, getSurfaceTitle()).WillOnce(ReturnRef(title));

        EXPECT_EQ(title, embeddedCompositor->getTitleOfWaylandIviSurface(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CallsSendFrameCallbacksInEndFrameWhenNotifyClientsFlagSet)
    {
        init();

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, sendFrameCallbacks(_));
        EXPECT_CALL(surface, resetNumberOfCommitedFrames());
        embeddedCompositor->endFrame(true);
    }

    TEST_F(AEmbeddedCompositor_Wayland, DoesNotCallSendFrameCallbacksInEndFrameWhenNotifyClientsFlagNotSet)
    {
        init();


        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);
        embeddedCompositor->endFrame(false);
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanAddIdToUpdatedStreamTextureSourceIds)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        EXPECT_FALSE(embeddedCompositor->hasUpdatedStreamTextureSources());
        embeddedCompositor->addToUpdatedStreamTextureSourceIds(surfaceIVIId);
        EXPECT_TRUE(embeddedCompositor->hasUpdatedStreamTextureSources());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanRemoveIdFromUpdatedStreamTextureSourceIds)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        embeddedCompositor->addToUpdatedStreamTextureSourceIds(surfaceIVIId);
        embeddedCompositor->removeFromUpdatedStreamTextureSourceIds(surfaceIVIId);

        EXPECT_FALSE(embeddedCompositor->hasUpdatedStreamTextureSources());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanDispatchUpdatedStreamTextureSourceIds)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        embeddedCompositor->addToUpdatedStreamTextureSourceIds(surfaceIVIId);

        StreamTextureSourceIdSet streamTextureSourceIds =  embeddedCompositor->dispatchUpdatedStreamTextureSourceIds();
        EXPECT_EQ(1u, streamTextureSourceIds.count());
        EXPECT_TRUE(streamTextureSourceIds.hasElement(surfaceIVIId));
        EXPECT_FALSE(embeddedCompositor->hasUpdatedStreamTextureSources());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanGetTotalNumberOfCommitedFramesForWaylandIviSurface)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);
        const UInt64              numberOfCommitedFrames = 456;

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, getNumberOfCommitedFramesSinceBeginningOfTime()).WillOnce(Return(numberOfCommitedFrames));

        EXPECT_EQ(numberOfCommitedFrames, embeddedCompositor->getNumberOfCommitedFramesForWaylandIviSurfaceSinceBeginningOfTime(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, ReturnsZeroForGetTotalNumberOfCommitedFramesForWaylandIviSurfaceWhenSurfaceDoesNotExist)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);
        const WaylandIviSurfaceId secondSurfaceIVIId(124);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_EQ(0u, embeddedCompositor->getNumberOfCommitedFramesForWaylandIviSurfaceSinceBeginningOfTime(secondSurfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, IsBufferAttachedToWaylandIviSurfaceReturnsCorrectValueWhenSurfaceExists)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, hasPendingBuffer()).WillOnce(Return(true));
        EXPECT_TRUE(embeddedCompositor->isBufferAttachedToWaylandIviSurface(surfaceIVIId));

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, hasPendingBuffer()).WillOnce(Return(false));
        EXPECT_FALSE(embeddedCompositor->isBufferAttachedToWaylandIviSurface(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, IsBufferAttachedToWaylandIviSurfaceReturnsCorrectValueWhenSurfaceDoesNotExist)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);
        const WaylandIviSurfaceId secondSurfaceIVIId(124);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_FALSE(embeddedCompositor->isBufferAttachedToWaylandIviSurface(secondSurfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, IsContentAvailableForStreamTextureReturnsCorrectValueWhenSurfaceExists)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);

        StrictMock<WaylandSurfaceMock> surface;
        StrictMock<WaylandBufferMock> waylandBuffer;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, getWaylandBuffer()).WillOnce(Return(&waylandBuffer));
        EXPECT_TRUE(embeddedCompositor->isContentAvailableForStreamTexture(surfaceIVIId));

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_CALL(surface, getWaylandBuffer()).WillOnce(Return(nullptr));
        EXPECT_FALSE(embeddedCompositor->isContentAvailableForStreamTexture(surfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, IsContentAvailableForStreamTextureReturnsCorrectValueWhenSurfaceDoesNotExist)
    {
        init();

        const WaylandIviSurfaceId surfaceIVIId(123);
        const WaylandIviSurfaceId secondSurfaceIVIId(124);

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(surface, getIviSurfaceId()).WillOnce(Return(surfaceIVIId));
        EXPECT_FALSE(embeddedCompositor->isContentAvailableForStreamTexture(secondSurfaceIVIId));
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanAddWaylandCompositorConnection)
    {
        init();

        StrictMock<WaylandCompositorConnectionMock> compositorConnection;

        EXPECT_EQ(0u, embeddedCompositor->getNumberOfCompositorConnections());
        embeddedCompositor->addWaylandCompositorConnection(compositorConnection);
        EXPECT_EQ(1u, embeddedCompositor->getNumberOfCompositorConnections());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanRemoveWaylandCompositorConnection)
    {
        init();

        StrictMock<WaylandCompositorConnectionMock> compositorConnection;
        embeddedCompositor->addWaylandCompositorConnection(compositorConnection);

        embeddedCompositor->removeWaylandCompositorConnection(compositorConnection);
        EXPECT_EQ(0u, embeddedCompositor->getNumberOfCompositorConnections());
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanAddAndRemoveWaylandRegion)
    {
        init();

        StrictMock<WaylandRegionMock> region;
        embeddedCompositor->addWaylandRegion(region);
        embeddedCompositor->removeWaylandRegion(region);
    }

    ACTION(LogSomeSurfaceLogToContext)
    {
        arg0 << "SomeSurfaceLog";
    }

    TEST_F(AEmbeddedCompositor_Wayland, CanLogInfos)
    {
        init();

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);


        RendererLogContext logContext(ERendererLogLevelFlag_Details);

        EXPECT_CALL(surface, logInfos(Ref(logContext))).WillOnce(LogSomeSurfaceLogToContext());
        embeddedCompositor->logInfos(logContext);

        EXPECT_STREQ(logContext.getStream().c_str(), "1 connected wayland client(s)\n  SomeSurfaceLog");
    }

    TEST_F(AEmbeddedCompositor_Wayland, getOrCreateBufferCreatesNewBuffer)
    {
        init();

        StrictMock<WaylandBufferResourceMock> bufferResource;
        StrictMock<WaylandBufferResourceMock>* bufferResourceCloned = new StrictMock<WaylandBufferResourceMock>;

        EXPECT_CALL(bufferResource, clone()).WillOnce(Return(bufferResourceCloned));
        EXPECT_CALL(*bufferResourceCloned, addDestroyListener(_));
        IWaylandBuffer& waylandBuffer = embeddedCompositor->getOrCreateBuffer(bufferResource);
        EXPECT_EQ(bufferResourceCloned, &waylandBuffer.getResource());

        delete &waylandBuffer;
    }

    TEST_F(AEmbeddedCompositor_Wayland, getOrCreateBufferReturnsExistingBuffer)
    {
        init();

        StrictMock<WaylandBufferResourceMock>  bufferResource;
        StrictMock<WaylandBufferResourceMock>* bufferResourceCloned = new StrictMock<WaylandBufferResourceMock>;

        WaylandNativeResource waylandNativeBufferResource(reinterpret_cast<WaylandNativeResource>(123));

        EXPECT_CALL(bufferResource, clone()).WillOnce(Return(bufferResourceCloned));
        EXPECT_CALL(*bufferResourceCloned, addDestroyListener(_));
        IWaylandBuffer& waylandBuffer = embeddedCompositor->getOrCreateBuffer(bufferResource);
        EXPECT_CALL(*bufferResourceCloned, getWaylandNativeResource()).WillOnce(Return(waylandNativeBufferResource));
        EXPECT_CALL(bufferResource, getWaylandNativeResource()).WillOnce(Return(waylandNativeBufferResource));
        IWaylandBuffer& waylandBuffer2 = embeddedCompositor->getOrCreateBuffer(bufferResource);

        EXPECT_EQ(&waylandBuffer, &waylandBuffer2);

        delete &waylandBuffer;
    }

    TEST_F(AEmbeddedCompositor_Wayland, HandleBufferDestroyedCallsSurfaceBufferDestroyed)
    {
        init();

        StrictMock<WaylandBufferResourceMock>  bufferResource;
        StrictMock<WaylandBufferResourceMock>* bufferResourceCloned = new StrictMock<WaylandBufferResourceMock>;

        StrictMock<WaylandSurfaceMock> surface;
        embeddedCompositor->addWaylandSurface(surface);

        EXPECT_CALL(bufferResource, clone()).WillOnce(Return(bufferResourceCloned));
        EXPECT_CALL(*bufferResourceCloned, addDestroyListener(_));
        IWaylandBuffer& waylandBuffer = embeddedCompositor->getOrCreateBuffer(bufferResource);

        EXPECT_CALL(surface, bufferDestroyed(Ref(waylandBuffer)));
        embeddedCompositor->handleBufferDestroyed(waylandBuffer);

        delete &waylandBuffer;
    }
}
