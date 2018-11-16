#include "ThreadPoolThread.h"
#include "ThreadPool.h"
#include "Task.h"
#include <process.h>
#include <cassert>
#define WAIT_TIME 20

ThreadPoolThread::ThreadPoolThread(ThreadPool* threadPool)
	: m_pThreadPool(threadPool)
	, m_pTask(nullptr)
	, m_hThread(nullptr)
	, m_hEvent(nullptr)
	, m_threadId(0)
	, m_bExit(false)
{
	m_hEvent = CreateEvent(nullptr, false, false, nullptr);
}

ThreadPoolThread::~ThreadPoolThread()
{
	char ch[64];
	sprintf(ch, "%s id:%d\n", __FUNCTION__, m_threadId); 
	OutputDebugStringA(ch);

	quit();

	if (m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = nullptr;
	}

	if(m_hThread)
	{
		if(WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT) 
		{
			char ch[64];
			sprintf(ch, "TerminateThread5 id:%d\n", m_threadId); 
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
	if(m_hThread == INVALID_HANDLE_VALUE)
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

	if(WaitForSingleObject(m_hThread, 1000) == WAIT_TIMEOUT)
	{
		char ch[64];
		sprintf(ch, "TerminateThread1 id:%d\n", m_threadId); 
		OutputDebugStringA(ch);
	}
}

void ThreadPoolThread::waitForDone()
{
	detachTask();
}

UINT WINAPI ThreadPoolThread::threadProc( LPVOID pParam )
{
	ThreadPoolThread *pThread = (ThreadPoolThread*)pParam;
	while(!pThread->m_bExit)
	{
		DWORD ret = WaitForSingleObject(pThread->m_hEvent, INFINITE);
		if(ret == WAIT_OBJECT_0)
		{
			pThread->exec();
		}
	}
	return 0;
}

bool ThreadPoolThread::assignTask(TaskBase *pTask)
{
	if(!pTask)
		return false;

	m_pTask = pTask;
	return true;
}

void ThreadPoolThread::detachTask()
{
	stopTask();
}

int ThreadPoolThread::taskId()
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
	if(m_pTask)
	{
		m_pTask->taskProc();
		delete m_pTask;
		m_pTask = nullptr;
	}

	if (!m_bExit)
	{
		m_pThreadPool->switchActiveThread(this);
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

bool ActiveThreadList::append(ThreadPoolThread *t)
{
	if(!t)
		return false;

	m_mutex.Lock();
	m_list.push_back(t);
	m_mutex.Unlock();
	return true;
}

bool ActiveThreadList::remove(ThreadPoolThread *t)
{
	if(!t)
		return false;

	m_mutex.Lock();
	m_list.remove(t);
	m_mutex.Unlock();
	return true;
}

ThreadPoolThread* ActiveThreadList::remove(int task_id)
{
	ThreadPoolThread* thread = nullptr;
	m_mutex.Lock();
	std::list<ThreadPoolThread*>::iterator iter = m_list.begin();
	for(; iter != m_list.end();)
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
	m_mutex.Unlock();
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
	m_mutex.Unlock();
	return thread;
}

int ActiveThreadList::size()
{
	m_mutex.Lock();
	int size = m_list.size();
	m_mutex.Unlock();
	return size;
}

bool ActiveThreadList::isEmpty()
{
	m_mutex.Lock();
	bool ret = m_list.empty();
	m_mutex.Unlock();
	return ret;
}

bool ActiveThreadList::clear()
{
	m_mutex.Lock();
	std::list<ThreadPoolThread*>::iterator iter = m_list.begin();
	for(; iter != m_list.end(); iter++)
	{
		delete (*iter);
	}
	m_list.clear();
	m_mutex.Unlock();
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
	if(!m_stack.empty())
	{
		ThreadPoolThread *t = m_stack.top();
		m_stack.pop();
		m_mutex.Unlock();
		return t;
	}
	m_mutex.Unlock();
	return nullptr;
}

bool IdleThreadStack::push( ThreadPoolThread* t)
{
	assert(t);
	if(!t)
		return false;
	m_mutex.Lock();
	t->suspend();
	m_stack.push(t);
	m_mutex.Unlock();
	return true;
}

int IdleThreadStack::getSize()
{
	m_mutex.Lock();
	int size = m_stack.size();
	m_mutex.Unlock();
	return size;
}

bool IdleThreadStack::isEmpty()
{
	m_mutex.Lock();
	bool ret = m_stack.empty();
	m_mutex.Unlock();
	return ret;
}

bool IdleThreadStack::clear()
{
	m_mutex.Lock();
	ThreadPoolThread *pThread = nullptr;
	while(!m_stack.empty())
	{
		pThread = m_stack.top();
		m_stack.pop();
		delete pThread;
	}
	m_mutex.Unlock();
	return true;
}