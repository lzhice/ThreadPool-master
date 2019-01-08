#include "ThreadPool.h"
#include <cassert>
#include <iostream>

ThreadPool::ThreadPool()
    : m_nThreadNum(1)
    , m_bInitialized(false)
    , m_pCallBack(nullptr)
{
}

ThreadPool::~ThreadPool()
{
    char ch[64];
    sprintf_s(ch, "%s(B)\n", __FUNCTION__);
    OutputDebugStringA(ch);

    waitForDone();

    sprintf_s(ch, "%s(E)\n", __FUNCTION__);
    OutputDebugStringA(ch);
}

ThreadPool* ThreadPool::Instance()
{
    static ThreadPool _instance;
    return &_instance;
}

bool ThreadPool::init(int threadCount)
{
    if (m_bInitialized)
    {
        return false;
    }

    if (threadCount < 1 || threadCount > 8)
    {
        char ch[128];
        sprintf_s(ch, "%s failed! thread range(1-8).\n", __FUNCTION__);
        OutputDebugStringA(ch);
        return false;
    }

    m_bInitialized = true;
    m_nThreadNum = threadCount;
    for (int i = 0; i < threadCount; i++)
    {
        ThreadPoolThread* p = new ThreadPoolThread(this);
        m_idleThreads.push(p);
        p->start();
    }
    return true;
}

bool ThreadPool::waitForDone()
{
    m_bInitialized = false;
	m_pCallBack = nullptr;

    abortAllTask();
    m_idleThreads.clear();
    m_activeThreads.clear();
    
    return true;
}

bool ThreadPool::addTask(TaskBase* t, Priority p)
{
    if (!t || !m_bInitialized)
    {
        return false;
    }

    if (p == Normal)
    {
        m_taskQueue.push(t);	//�����������
    }
    else if (p == High)
    {
        m_taskQueue.pushFront(t);	//�����ȼ�����
    }

    if (!m_idleThreads.isEmpty())
    {
        TaskBase* task = getNextTask();
        if (task == nullptr)
        {
            return false;
        }

        ThreadPoolThread* pThread = popIdleThread();
        if (pThread)
        {
            m_activeThreads.append(pThread);
            pThread->assignTask(task);
            pThread->startTask();
        }
    }
    return true;
}

bool ThreadPool::abortTask(int taskId)
{
    ThreadPoolThread* p = m_activeThreads.remove(taskId);
    if (p)
    {
        p->stopTask();
        return true;
    }
    return false;
}

bool ThreadPool::abortAllTask()
{
    m_taskQueue.clear();
    ThreadPoolThread* p = m_activeThreads.pop_back();
    while (p)
    {
        p->stopTask();
        p = m_activeThreads.pop_back();
    }
    return true;
}

TaskBase* ThreadPool::getNextTask()
{
    if (m_taskQueue.isEmpty())
    {
        return nullptr;
    }

    TaskBase* task = m_taskQueue.pop();
    if (task == nullptr)
    {
        OutputDebugStringA("null task!\n");
    }
    return task;
}

ThreadPoolThread* ThreadPool::popIdleThread()
{
    return m_idleThreads.pop();
}

bool ThreadPool::onThreadFinished(ThreadPoolThread* t)
{
    if (!m_bInitialized)
    {
        return false;
    }

    TaskBase* pTask = getNextTask();
    if (pTask)
    {
        t->assignTask(pTask);
        t->startTask();
    }
    else	//�������Ϊ�գ����̹߳���
    {
        m_activeThreads.remove(t);
        m_idleThreads.push(t);
    }
    return true;
}

void ThreadPool::setCallBack(ThreadPoolCallBack* pCallBack)
{
    if (pCallBack)
    {
        m_pCallBack = pCallBack;
    }
}

void ThreadPool::onTaskFinished(int taskId)
{
    if (m_pCallBack)
    {
        m_pCallBack->onTaskFinished(taskId);
    }
}