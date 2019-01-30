#include "stdafx.h"
#include "ThreadPool.h"
#include <cassert>
#include <iostream>
#include "ClassMemoryTracer.h"
#include "log.h"

ThreadPool::ThreadPool()
	: m_nThreadNum(1)
	, m_bInitialized(false)
	, m_pCallBack(nullptr)
{
	TRACE_CLASS_CONSTRUCTOR(ThreadPool);
	m_pThread = new ScheduleThread;
}

ThreadPool::~ThreadPool()
{
	LOG_DEBUG("%s (B)\n", __FUNCTION__);
	TRACE_CLASS_DESTRUCTOR(ThreadPool);

	waitForDone();
	if (m_pThread)
	{
		m_pThread->quit();
		m_pThread->wait();
		delete m_pThread;
		m_pThread = nullptr;
	}

	TRACE_CLASS_PRINT();
	LOG_DEBUG("%s (E)\n", __FUNCTION__);
}

ThreadPool* ThreadPool::globalInstance()
{
	static ThreadPool _instance;
	return &_instance;
}

bool ThreadPool::init(int threadCount)
{
	if (m_bInitialized)
		return false;

	if (threadCount < 1 || threadCount > 16)
	{
		LOG_DEBUG("%s failed! thread range(1-16).\n", __FUNCTION__);
		return false;
	}

	m_nThreadNum = threadCount;
	for (int i = 0; i < threadCount; i++)
	{
		ThreadPoolThread* p = new ThreadPoolThread(this);
		m_idleThreads.push(p);
		p->start();
	}
	m_pThread->start();
	m_bInitialized = true;
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

bool ThreadPool::addTask(std::shared_ptr<TaskBase> t, Priority p)
{
	if (!t.get() || !m_bInitialized)
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

	if (m_pThread->isSuspend())
	{
		m_pThread->resume();
	}

	return true;
}

bool ThreadPool::abortTask(int taskId)
{
	ThreadPoolThread* p = m_activeThreads.take(taskId);
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

std::shared_ptr<TaskBase> ThreadPool::takeTask()
{
	if (m_taskQueue.isEmpty())
	{
		return nullptr;
	}

	std::shared_ptr<TaskBase> task = m_taskQueue.pop();
	if (!task.get())
	{
		LOG_DEBUG("Error pop task!\n");
	}
	/*else
	{
		LOG_DEBUG("Pop task, id:%d\n", task->id());
	}*/
	return task;
}

void ThreadPool::pushIdleThread(ThreadPoolThread* t)
{
	m_idleThreads.push(t);
}

ThreadPoolThread* ThreadPool::popIdleThread()
{
	return m_idleThreads.pop();
}

void ThreadPool::appendActiveThread(ThreadPoolThread* t)
{
	m_activeThreads.append(t);
}

ThreadPoolThread* ThreadPool::takeActiveThread(UINT threadId)
{
	return m_activeThreads.take(threadId);
}

void ThreadPool::setCallBack(ThreadPoolCallBack* pCallBack)
{
	if (pCallBack)
	{
		m_pCallBack = pCallBack;
	}
}

void ThreadPool::onTaskFinished(int taskId, UINT threadId)
{
	if (!m_bInitialized)
		return;

	if (m_pThread)
	{
		::PostThreadMessage(m_pThread->threadId(), WM_THREAD_TASK_FINISHED, (WPARAM)threadId, 0);
	}

	if (m_pCallBack && taskId > 0)
	{
		m_pCallBack->onTaskFinished(taskId);
	}
}