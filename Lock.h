﻿/*
@Brief:		windows锁
@Author:	vilas wang
@Contact:	QQ451930733
*/

#ifndef __WINDOWS_LOCK_H__
#define __WINDOWS_LOCK_H__

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>

namespace VCUtil {

    //Class CSLock - 关键段锁
    class CSLock
    {
    public:
        CSLock();
        virtual ~CSLock();

        virtual void lock();
        virtual bool tryLock();
        virtual void unlock();

    private:
        CSLock(const CSLock &);
        CSLock &operator=(const CSLock &);

    private:
        CRITICAL_SECTION m_cs;
    };

    //Class SRWLock - slim 读写锁
    class SRWLock
    {
    public:
        SRWLock();
        virtual ~SRWLock();

        virtual void lock(bool bShared = false);
        virtual bool tryLock(bool bShared = false);
        virtual void unlock();

    private:
        SRWLock(const SRWLock &);
        SRWLock &operator=(const SRWLock &);

    private:
        SRWLOCK m_lock;
        long m_bSharedLocked;
        long m_bExclusiveLocked;
    };

    template<typename _Lock = typename std::enable_if<
        std::is_convertible<_Lock, SRWLock>::value ||
        std::is_convertible<_Lock, CSLock>::value>::type>
    class Locker
    {
    public:
        explicit Locker(_Lock& lock)
            : m_lock(lock)
        {
            m_lock.lock();
        }

        Locker(_Lock& lock, bool bShared)
            : m_lock(lock)
        {
            m_lock.lock(bShared);
        }

#if _MSC_VER >= 1700
        ~Locker() _NOEXCEPT
#else
        ~Locker()
#endif
        {
            m_lock.unlock();
        }

    private:
        Locker(const Locker&);
        Locker& operator=(const Locker&);

    private:
        _Lock& m_lock;
    };
}

#endif