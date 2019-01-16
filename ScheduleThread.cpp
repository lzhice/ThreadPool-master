#include "ScheduleThread.h"
#include <process.h>
#include <iostream>
#include "ThreadPool.h"


ScheduleThread::ScheduleThread()
	: m_hThread(nullptr)
	, m_nThreadID(0)
	, m_bExit(false)
	, m_bRunning(false)
{
}

ScheduleThread::~ScheduleThread()
{
	quit();
}

unsigned __stdcall ScheduleThread::ThreadFunc(LPVOID pParam)
{
	ScheduleThread *thread = (ScheduleThread *)(pParam);
	if (thread)
	{
		thread->m_bRunning = true;
		thread->onBeforeExec();

		MSG msg = { 0 };
		while (!thread->m_bExit)
		{
			msg = { 0 };
			PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			switch (msg.message)
			{
			case WM_THREAD_TASK_FINISHED:
				thread->switchThread((UINT)msg.wParam);
				break;
			case WM_QUIT:
				thread->m_bExit = true;
				break;
			default: 
				break;
			}

			thread->run();
		}
		thread->onBeforeExit();
		thread->m_bRunning = false;
	}
	return 0;
}

bool ScheduleThread::start()
{
	m_bExit = false;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &ScheduleThread::ThreadFunc, this, NULL, &m_nThreadID);
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	return true;
}

void ScheduleThread::quit()
{
	m_bExit = true;
	m_task_cv.notify_all();

	if (m_hThread)
	{
		if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
		{
			std::cout << "WaitForSingleObject 5s TIMEOUT. id:" << m_nThreadID << std::endl;
			_endthreadex(1);
		}

		CloseHandle(m_hThread);
		m_hThread = NULL;
		m_nThreadID = 0;
	}
}

bool ScheduleThread::wait(unsigned long time)
{
	if (time == ULONG_MAX)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		return true;
	}

	if (WaitForSingleObject(m_hThread, time) == WAIT_TIMEOUT)
	{
		std::cout << "ScheduleThread::wait TIMEOUT. id:" << m_nThreadID << std::endl;
		_endthreadex(1);
		return false;
	}
	return true;
}

void ScheduleThread::wakeAll()
{
	m_task_cv.notify_all();
}

void ScheduleThread::wakeONe()
{
	m_task_cv.notify_one();
}

void ScheduleThread::run()
{
	/*std::unique_lock<std::mutex> lock(m_mutex);
	m_task_cv.wait(lock, [this] {
		return (m_bExit || (ThreadPool::globalInstance()->hasIdleThread() && ThreadPool::globalInstance()->hasTask()));
	});*/

	if (m_bExit)
		return;

	if (!ThreadPool::globalInstance()->hasIdleThread() || !ThreadPool::globalInstance()->hasTask())
	{
		Sleep(30);
		return;
	}

	std::shared_ptr<TaskBase> pTask = ThreadPool::globalInstance()->takeTask();
	if (pTask.get())
	{
		ThreadPoolThread* thread = ThreadPool::globalInstance()->popIdleThread();
		if (nullptr != thread)
		{
			ThreadPool::globalInstance()->appendActiveThread(thread);
			thread->assignTask(pTask);
			thread->resume();
			Sleep(1);
		}
		else
		{
			OutputDebugString(L"error1\n");
		}
	}
	else
	{
		OutputDebugString(L"error2\n");
	}
}

void ScheduleThread::switchThread(UINT threadId)
{
	ThreadPoolThread* thread = ThreadPool::globalInstance()->takeActiveThread(threadId);
	if (nullptr != thread)
	{
		ThreadPool::globalInstance()->pushIdleThread(thread);
		m_task_cv.notify_one();
	}
}