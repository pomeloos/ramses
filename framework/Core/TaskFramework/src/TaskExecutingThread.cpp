//  -------------------------------------------------------------------------
//  Copyright (C) 2013 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "TaskFramework/TaskExecutingThread.h"

#include "TaskFramework/ITask.h"
#include "PlatformAbstraction/PlatformGuard.h"

namespace ramses_internal
{

    TaskExecutingThread::TaskExecutingThread(UInt16 workerIndex, IThreadAliveNotifier& aliveHandler)
            : m_pBlockingTaskQueue(nullptr)
            , m_thread("R_Taskpool_Thrd")
            , m_workerIndex(workerIndex)
            , m_aliveHandler(aliveHandler)
            , m_bThreadStarted(false)
    {
    }

    TaskExecutingThread::~TaskExecutingThread()
    {
        stop();
    }


    void TaskExecutingThread::start(IBlockingTaskQueue& blockingTaskQueue)
    {
        PlatformLightweightGuard g(m_startStopLock);
        if (!m_bThreadStarted)
        {
            m_bThreadStarted = true;

            m_pBlockingTaskQueue = &blockingTaskQueue;

            resetCancel();

            m_thread.start(*this);
        }
    }

    void TaskExecutingThread::stop()
    {
        PlatformLightweightGuard g(m_startStopLock);
        if (m_bThreadStarted)
        {
            // Signal the runnable the cancel request.
            m_thread.cancel();
            m_pBlockingTaskQueue->addTask(nullptr);

            m_thread.join();
            m_pBlockingTaskQueue = nullptr;
            m_bThreadStarted = false;
        }
    }

    void TaskExecutingThread::cancelThread()
    {
        PlatformLightweightGuard g(m_startStopLock);
        if (m_bThreadStarted)
        {
            // Signal the runnable the cancel request.
            m_thread.cancel();
        }
    }

    void TaskExecutingThread::unlockThread()
    {
        m_pBlockingTaskQueue->addTask(nullptr);
    }

    void TaskExecutingThread::joinThread()
    {
        PlatformLightweightGuard g(m_startStopLock);
        if (m_bThreadStarted && isCancelRequested())
        {
            m_thread.join();
            m_pBlockingTaskQueue = nullptr;
            m_bThreadStarted = false;
        }
    }

    void TaskExecutingThread::run()
    {
        if (nullptr != m_pBlockingTaskQueue)
        {
            m_aliveHandler.notifyAlive(m_workerIndex);
            while (!isCancelRequested())
            {
                ITask* const pTaskToExecute = m_pBlockingTaskQueue->popTask(m_aliveHandler.calculateTimeout());
                m_aliveHandler.notifyAlive(m_workerIndex);
                if (nullptr != pTaskToExecute)
                {
                    pTaskToExecute->execute();
                    pTaskToExecute->release();
                }
            }
        }
        else
        {
            LOG_WARN(CONTEXT_FRAMEWORK, "TaskExecutingThread::run() pointer to blocking task queue is null, leaving thread.");
        }

        LOG_TRACE(CONTEXT_FRAMEWORK, "TaskExecutingThread::run() leaving thread.");
    }
}
