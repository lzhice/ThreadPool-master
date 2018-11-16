#include "ThreadPool.h"
#include <cassert>
#include <iostream>

ThreadPool::ThreadPool() 
	: m_nThreadNum(1)
	, m_bInitialized(false)
{
}

ThreadPool::~ThreadPool()
{
	char ch[64];
	sprintf(ch, "%s\n", __FUNCTION__); 
	OutputDebugStringA(ch);

	waitForDone();
}

ThreadPool& ThreadPool::Instance()
{
	static ThreadPool _instance;
	return _instance;
}

bool ThreadPool::init(int threadCount)
{
	if (m_bInitialized)
		return false;

	if (threadCount < 1 || threadCount > 8)
		return false;

	m_bInitialized = true;
	m_nThreadNum = threadCount;
	for(int i = 0; i < threadCount; i++)
	{
		ThreadPoolThread *p = new ThreadPoolThread(this);
		m_idleThreads.push(p);
		p->start();
	}
	return true;
}

bool ThreadPool::addTask(TaskBase *t, PRIORITY priority)
{
	if(!t || !m_bInitialized)
		return false;

	if(priority == PRIORITY::NORMAL)
	{
		m_taskQueue.push(t);//进入任务队列
	}
	else if(priority == PRIORITY::HIGH)
	{
		m_taskQueue.pushFront(t);//高优先级任务
	}

	if(!m_idleThreads.isEmpty())
	{
		TaskBase *task = getNextTask();
		if(task == nullptr)
			return false;

		ThreadPoolThread *pThread = popIdleThread();
		if (pThread)
		{
			m_activeThreads.append(pThread);
			pThread->assignTask(task);
			pThread->startTask();
		}
	}
	return true;
}

bool ThreadPool::abortTask(int task_id)
{
	ThreadPoolThread *p = m_activeThreads.remove(task_id);
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
	ThreadPoolThread *p = m_activeThreads.pop_back();
	while (p)
	{
		p->stopTask();
		p = m_activeThreads.pop_back();
	}
	return true;
}

TaskBase* ThreadPool::getNextTask()
{
	if(m_taskQueue.isEmpty())
		return nullptr;

	TaskBase *task = m_taskQueue.pop();
	if(task == nullptr)
	{
		OutputDebugStringA("get task error!\n");
	}
	return task;
}

ThreadPoolThread *ThreadPool::popIdleThread()
{
	return m_idleThreads.pop();
}

bool ThreadPool::switchActiveThread(ThreadPoolThread *t)
{
	if (!m_bInitialized)
		return false;

	TaskBase *pTask = getNextTask();
	if (pTask)
	{
		t->assignTask(pTask);
		t->startTask();
	}
	else//任务队列为空，该线程挂起
	{
		m_activeThreads.remove(t);
		m_idleThreads.push(t);
	}
	return true;
}

bool ThreadPool::waitForDone()
{
	m_bInitialized = false;
	m_taskQueue.clear();
	m_idleThreads.clear();
	m_activeThreads.clear();
	return true;
}