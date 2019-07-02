﻿#ifndef SCHEDULETHREAD_H
#define SCHEDULETHREAD_H
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#if _MSC_VER >= 1700
#include <atomic>
#else
#include "Lock.h"
#endif

//class ScheduleThread - 线程池调度线程
class ScheduleThread
{
public:
    ScheduleThread();
    virtual ~ScheduleThread();
#if _MSC_VER >= 1700
    ScheduleThread(const ScheduleThread &) = delete;
    ScheduleThread &operator=(const ScheduleThread &) = delete;
#endif

    bool start();
    void quit();
    bool wait(unsigned long time = ULONG_MAX); //dwMilliseconds
    bool isSuspend();
    bool suspend();
    bool resume();

    bool isRunning() const;

    const UINT threadId() const { return m_nThreadID; }

protected:
    virtual void run();
    virtual void onBeforeRun() {}
    virtual void onBeforeExit() {}

private:
#if _MSC_VER < 1700
    ScheduleThread(const ScheduleThread &);
    ScheduleThread &operator=(const ScheduleThread &);
#endif

    static unsigned __stdcall ThreadFunc(LPVOID pParam);
    void switchToIdleThread(UINT threadId);

    bool isExit() const;
    void setExit(bool bExit);

private:
    HANDLE m_hThread;
    unsigned m_nThreadID;
#if _MSC_VER >= 1700
    std::atomic<bool> m_bExit;
    std::atomic<bool> m_bRunning;
#else
    mutable VCUtil::CSLock m_lock;
    bool m_bExit;
    bool m_bRunning;
#endif
    HANDLE m_hEvent;
};

#endif //SCHEDULETHREAD_H