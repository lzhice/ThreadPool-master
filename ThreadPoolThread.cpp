//#include "stdafx.h"
#include "ThreadPoolThread.h"
#include "ThreadPool.h"
#include "Task.h"
#include <process.h>
#include <cassert>
#include <iostream>
#ifdef TRACE_CLASS_MEMORY_ENABLED
#include "ClassMemoryTracer.h"
#endif

ThreadPoolThread::ThreadPoolThread(ThreadPool* threadPool)
	: m_pThreadPool(threadPool)
	, m_pTask(nullptr)
	, m_hThread(INVALID_HANDLE_VALUE)
	, m_hEvent(nullptr)
	, m_nThreadID(0)
	, m_bExit(false)
{
#ifdef TRACE_CLASS_MEMORY_ENABLED
	TRACE_CLASS_CONSTRUCTOR(ThreadPoolThread);
#endif
	m_hEvent = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == m_hEvent)
	{
		std::cout << GetLastError() << std::endl;
	}
}

ThreadPoolThread::~ThreadPoolThread()
{
#ifdef TRACE_CLASS_MEMORY_ENABLED
	TRACE_CLASS_DESTRUCTOR(ThreadPoolThread);
#endif
	std::cout << __FUNCTION__ << " id:" << m_nThreadID << std::endl;

	quit();
	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = nullptr;
	}
}

bool ThreadPoolThread::start()
{
	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, &ThreadPoolThread::threadFunc, this, 0, &m_nThreadID);
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		std::cout << GetLastError() << std::endl;
		return false;
	}
	return true;
}

void ThreadPoolThread::quit()
{
	m_bExit = true;
	waitForDone();

	if (m_hThread)
	{
		if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
		{
			std::cout << "ThreadPoolThread WaitForSingleObject 5s TIMEOUT. id:" << m_nThreadID << std::endl;
			_endthreadex(1);
		}

		CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_nThreadID = 0;
	}
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

void ThreadPoolThread::waitForDone()
{
	stopTask();
}

UINT WINAPI ThreadPoolThread::threadFunc(LPVOID pParam)
{
	ThreadPoolThread* t = (ThreadPoolThread*)pParam;
	if (t)
	{
		while (!t->m_bExit)
		{
			DWORD ret = WaitForSingleObject(t->m_hEvent, INFINITE);
			switch (ret)
			{
			case WAIT_OBJECT_0:
				{
					t->exec();
				}
				break;
			case WAIT_FAILED:
				{
					std::cout << GetLastError() << std::endl;
				}
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

bool ThreadPoolThread::assignTask(std::shared_ptr<TaskBase> pTask)
{
	if (!pTask)
	{
		return false;
	}
	m_pTask = pTask;
	return true;
}

const int ThreadPoolThread::taskId()
{
	if (m_pTask.get())
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
	if (m_pTask.get())
	{
		m_pTask->cancel();
	}
	resume();
	return true;
}

void ThreadPoolThread::exec()
{
	if (m_bExit)
		return;

	if (m_pTask.get())
	{
		int id = m_pTask->id();
		m_pTask->exec();
		m_pTask.reset();

		if (m_pThreadPool && !m_bExit)
		{
			m_pThreadPool->onTaskFinished(id, m_nThreadID);
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

	m_lock.lock();
	m_list.push_back(t);
	m_lock.unLock();
	return true;
}

bool ActiveThreadList::remove(ThreadPoolThread* t)
{
	if (!t)
	{
		return false;
	}

	m_lock.lock();
	m_list.remove(t);
	m_lock.unLock();
	return true;
}

ThreadPoolThread* ActiveThreadList::take(int task_id)
{
	ThreadPoolThread* t = nullptr;
	m_lock.lock();
	auto iter = m_list.begin();
	for (; iter != m_list.end();)
	{
		if ((*iter)->taskId() == task_id)
		{
			t = (*iter);
			iter = m_list.erase(iter);
			break;
		}
		else
		{
			++iter;
		}
	}
	m_lock.unLock();
	return t;
}

ThreadPoolThread* ActiveThreadList::take(UINT thread_id)
{
	ThreadPoolThread* t = nullptr;
	m_lock.lock();
	auto iter = m_list.begin();
	for (; iter != m_list.end();)
	{
		if ((*iter)->threadId() == thread_id)
		{
			t = (*iter);
			iter = m_list.erase(iter);
			break;
		}
		else
		{
			++iter;
		}
	}
	m_lock.unLock();
	return t;
}

ThreadPoolThread* ActiveThreadList::pop_back()
{
	ThreadPoolThread* t = nullptr;
	m_lock.lock();
	if (!m_list.empty())
	{
		t = m_list.back();
		m_list.remove(t);
	}
	m_lock.unLock();
	return t;
}

int ActiveThreadList::size()
{
	m_lock.lock();
	int size = m_list.size();
	m_lock.unLock();
	return size;
}

bool ActiveThreadList::isEmpty()
{
	m_lock.lock();
	bool ret = m_list.empty();
	m_lock.unLock();
	return ret;
}

bool ActiveThreadList::clear()
{
	m_lock.lock();
	auto iter = m_list.begin();
	for (; iter != m_list.end(); iter++)
	{
		if (nullptr != (*iter))
		{
			delete (*iter);
		}
	}
	m_list.clear();
	m_lock.unLock();
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
	m_lock.lock();
	if (!m_stack.empty())
	{
		ThreadPoolThread* t = m_stack.top();
		m_stack.pop();
		m_lock.unLock();
		return t;
	}
	m_lock.unLock();
	return nullptr;
}

bool IdleThreadStack::push(ThreadPoolThread* t)
{
	assert(t);
	if (!t)
	{
		return false;
	}
	m_lock.lock();
	t->suspend();
	m_stack.push(t);
	m_lock.unLock();
	return true;
}

int IdleThreadStack::size()
{
	m_lock.lock();
	int size = m_stack.size();
	m_lock.unLock();
	return size;
}

bool IdleThreadStack::isEmpty()
{
	m_lock.lock();
	bool ret = m_stack.empty();
	m_lock.unLock();
	return ret;
}

bool IdleThreadStack::clear()
{
	m_lock.lock();
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
	m_lock.unLock();
	return true;
}