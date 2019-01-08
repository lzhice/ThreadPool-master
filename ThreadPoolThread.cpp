#include "ThreadPoolThread.h"
#include "ThreadPool.h"
#include "Task.h"
#include <process.h>
#include <cassert>
#define WAIT_TIME 20

ThreadPoolThread::ThreadPoolThread(ThreadPool* threadPool)
    : m_pThreadPool(threadPool)
    , m_pTask(nullptr)
    , m_hThread(INVALID_HANDLE_VALUE)
    , m_hEvent(nullptr)
    , m_threadId(0)
    , m_bExit(false)
{
    m_hEvent = CreateEvent(nullptr, false, false, nullptr);
}

ThreadPoolThread::~ThreadPoolThread()
{
    char ch[64];
    sprintf_s(ch, "%s id:%d\n", __FUNCTION__, m_threadId);
    OutputDebugStringA(ch);

    quit();

    if (m_hEvent)
    {
        CloseHandle(m_hEvent);
        m_hEvent = nullptr;
    }

    if (m_hThread)
    {
        if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
        {
            char ch[64];
            sprintf_s(ch, "TerminateThread5 id:%d\n", m_threadId);
            OutputDebugStringA(ch);
            TerminateThread(m_hThread, -1);
        }
        CloseHandle(m_hThread);
        m_hThread = nullptr;
        m_threadId = 0;
    }
}

bool ThreadPoolThread::start()
{
    m_hThread = (HANDLE)_beginthreadex(nullptr, 0, &ThreadPoolThread::threadProc, this, 0, &m_threadId);
    if (m_hThread == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    return true;
}

bool ThreadPoolThread::suspend()
{
    ResetEvent(m_hEvent);
    return true;
}

bool ThreadPoolThread::resume()
{
    SetEvent(m_hEvent);
    return true;
}

void ThreadPoolThread::quit()
{
    m_bExit = true;
    waitForDone();

    if (WaitForSingleObject(m_hThread, 1000) == WAIT_TIMEOUT)
    {
        char ch[64];
        sprintf_s(ch, "TerminateThread TIMEOUT id:%d\n", m_threadId);
        OutputDebugStringA(ch);
    }
}

void ThreadPoolThread::waitForDone()
{
    stopTask();
}

UINT WINAPI ThreadPoolThread::threadProc(LPVOID pParam)
{
    // Ϊ�߳�׼����Ϣ����
    MSG msg = {0};
    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

    ThreadPoolThread* pThread = (ThreadPoolThread*)pParam;
    while (pThread && !pThread->m_bExit)
    {
        DWORD ret = WaitForSingleObject(pThread->m_hEvent, INFINITE);
        switch (ret)
        {
        case WAIT_OBJECT_0 + 1:
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                switch (msg.message)
                {
                case WM_QUIT:
                    pThread->m_bExit = TRUE;
                    break;
                default:
                    break;
                }
            }
        }
        break;
        case WAIT_OBJECT_0:
        {
            pThread->exec();
        }
        break;
        default:
            break;
        }
    }
    return 0;
}

bool ThreadPoolThread::assignTask(TaskBase* pTask)
{
    if (!pTask)
    {
        return false;
    }

    m_pTask = pTask;
    return true;
}

void ThreadPoolThread::detachTask()
{
    stopTask();
}

const int ThreadPoolThread::taskId()
{
    if (m_pTask)
    {
        return m_pTask->id();
    }
    return 0;
}

bool ThreadPoolThread::startTask()
{
    resume();
    return true;
}

bool ThreadPoolThread::stopTask()
{
    if (m_pTask)
    {
        m_pTask->cancel();
    }
    resume();
    return true;
}

void ThreadPoolThread::exec()
{
    if (m_pTask)
    {
        m_pTask->exec();
        if (m_pTask->isAutoDelete())
        {
            delete m_pTask;
        }
        else
        {
            if (m_pThreadPool)
            {
                m_pThreadPool->onTaskFinished(m_pTask->id());
            }
        }
        m_pTask = nullptr;
    }

    if (!m_bExit)
    {
        Sleep(1);
        if (m_pThreadPool)
        {
            m_pThreadPool->onThreadFinished(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
ActiveThreadList::ActiveThreadList()
{
}

ActiveThreadList::~ActiveThreadList()
{
    clear();
}

bool ActiveThreadList::append(ThreadPoolThread* t)
{
    if (!t)
    {
        return false;
    }

    m_mutex.Lock();
    m_list.push_back(t);
    m_mutex.UnLock();
    return true;
}

bool ActiveThreadList::remove(ThreadPoolThread* t)
{
    if (!t)
    {
        return false;
    }

    m_mutex.Lock();
    m_list.remove(t);
    m_mutex.UnLock();
    return true;
}

ThreadPoolThread* ActiveThreadList::remove(int task_id)
{
    ThreadPoolThread* thread = nullptr;
    m_mutex.Lock();
    auto iter = m_list.begin();
    for (; iter != m_list.end();)
    {
        if ((*iter)->taskId() == task_id)
        {
            thread = (*iter);
            iter = m_list.erase(iter);
            break;
        }
        else
        {
            ++iter;
        }
    }
    m_mutex.UnLock();
    return thread;
}

ThreadPoolThread* ActiveThreadList::pop_back()
{
    ThreadPoolThread* thread = nullptr;
    m_mutex.Lock();
    if (!m_list.empty())
    {
        thread = m_list.back();
        m_list.remove(thread);
    }
    m_mutex.UnLock();
    return thread;
}

int ActiveThreadList::size()
{
    m_mutex.Lock();
    int size = m_list.size();
    m_mutex.UnLock();
    return size;
}

bool ActiveThreadList::isEmpty()
{
    m_mutex.Lock();
    bool ret = m_list.empty();
    m_mutex.UnLock();
    return ret;
}

bool ActiveThreadList::clear()
{
    m_mutex.Lock();
    auto iter = m_list.begin();
    for (; iter != m_list.end(); iter++)
    {
        if (nullptr != (*iter))
        {
            delete (*iter);
        }
    }
    m_list.clear();
    m_mutex.UnLock();
    return true;
}

//////////////////////////////////////////////////////////////////////////
IdleThreadStack::IdleThreadStack()
{
}

IdleThreadStack::~IdleThreadStack()
{
    clear();
}

ThreadPoolThread* IdleThreadStack::pop()
{
    m_mutex.Lock();
    if (!m_stack.empty())
    {
        ThreadPoolThread* t = m_stack.top();
        m_stack.pop();
        m_mutex.UnLock();
        return t;
    }
    m_mutex.UnLock();
    return nullptr;
}

bool IdleThreadStack::push(ThreadPoolThread* t)
{
    assert(t);
    if (!t)
    {
        return false;
    }
    m_mutex.Lock();
    t->suspend();
    m_stack.push(t);
    m_mutex.UnLock();
    return true;
}

int IdleThreadStack::size()
{
    m_mutex.Lock();
    int size = m_stack.size();
    m_mutex.UnLock();
    return size;
}

bool IdleThreadStack::isEmpty()
{
    m_mutex.Lock();
    bool ret = m_stack.empty();
    m_mutex.UnLock();
    return ret;
}

bool IdleThreadStack::clear()
{
    m_mutex.Lock();
    ThreadPoolThread* pThread = nullptr;
    while (!m_stack.empty())
    {
        pThread = m_stack.top();
        m_stack.pop();

        if (pThread)
        {
            delete pThread;
        }
    }
    m_mutex.UnLock();
    return true;
}