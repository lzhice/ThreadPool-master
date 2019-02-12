//#include "stdafx.h"
#include "ScheduleThread.h"
#include <process.h>
#include <iostream>
#include "ThreadPool.h"
#include "log.h"
#include "ClassMemoryTracer.h"


ScheduleThread::ScheduleThread()
	: m_hThread(nullptr)
	, m_nThreadID(0)
	, m_bExit(false)
	, m_bRunning(false)
{
	TRACE_CLASS_CONSTRUCTOR(ScheduleThread);
	m_hEvent = CreateEvent(0, TRUE, TRUE, 0);
	if (nullptr == m_hEvent)
	{
		LOG_DEBUG("ScheduleThread CreateEvent error! [%ul]\n", GetLastError());
	}
}

ScheduleThread::~ScheduleThread()
{
	TRACE_CLASS_DESTRUCTOR(ScheduleThread);
	LOG_DEBUG("%s id[%d]\n", __FUNCTION__, m_nThreadID);

	quit();
	if (m_hEvent)
	{
		if (WaitForSingleObject(m_hEvent, 1000) == WAIT_TIMEOUT)
		{
			LOG_DEBUG("ScheduleThread WaitForEvent 1s TIMEOUT.\n");
		}

		CloseHandle(m_hEvent);
		m_hEvent = nullptr;
	}
}

bool ScheduleThread::start()
{
	m_bExit = false;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &ScheduleThread::ThreadFunc, this, NULL, &m_nThreadID);
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		LOG_DEBUG("%s error! [%ul]\n", __FUNCTION__, GetLastError());
		return false;
	}
	return true;
}

void ScheduleThread::quit()
{
	m_bExit = true;
	resume();

	if (m_hThread)
	{
		if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
		{
			LOG_DEBUG("ScheduleThread WaitForThread 5s TIMEOUT.\n");
			_endthreadex(1);
		}

		CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_nThreadID = 0;
	}
}

bool ScheduleThread::wait(unsigned long time)
{
	if (nullptr == m_hThread)
		return true;

	if (time == ULONG_MAX)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		return true;
	}

	if (WaitForSingleObject(m_hThread, time) == WAIT_TIMEOUT)
	{
		LOG_DEBUG("ScheduleThread::wait TIMEOUT. id[%d]\n", m_nThreadID);
		_endthreadex(1);
		return false;
	}
	return true;
}

bool ScheduleThread::suspend()
{
	if (m_hEvent)
	{
		ResetEvent(m_hEvent);
		return true;
	}
	return false;
}

bool ScheduleThread::resume()
{
	if (m_hEvent)
	{
		SetEvent(m_hEvent);
		return true;
	}
	return false;
}

bool ScheduleThread::isSuspend()
{
	if (nullptr == m_hThread || nullptr == m_hEvent)
		return false;

	if (WAIT_OBJECT_0 != WaitForSingleObject(m_hEvent, 0))
		return true;

	return false;
}

unsigned __stdcall ScheduleThread::ThreadFunc(LPVOID pParam)
{
	ScheduleThread *t = (ScheduleThread *)(pParam);
	if (t)
	{
		t->m_bRunning = true;
		t->onBeforeExec();

		MSG msg = { 0 };
		PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

		HANDLE h[1];
		h[0] = t->m_hEvent;

		DWORD ret = WAIT_FAILED;
		while (!t->m_bExit)
		{
			ret = MsgWaitForMultipleObjects(1, h, false, INFINITE, QS_ALLPOSTMESSAGE);
			switch (ret)
			{
			case WAIT_OBJECT_0:
				{
					t->run();
				}
				break;
			case WAIT_OBJECT_0 + 1:
				{
					msg = { 0 };
					if (TRUE == PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						switch (msg.message)
						{
						case WM_THREAD_TASK_FINISHED:
							t->switchToIdleThread((UINT)msg.wParam);
							break;
						case WM_QUIT:
							t->m_bExit = true;
							break;
						default:
							break;
						}
					}
				}
				break;
			case WAIT_FAILED:
				{
					LOG_DEBUG("ScheduleThread WaitForEvent error. [%ul]\n", GetLastError());
				}
				break;
			default:
				break;
			}
		}

		t->onBeforeExit();
		t->m_bRunning = false;
	}
	return 0;
}

void ScheduleThread::run()
{
	if (m_bExit)
		return;

	if (!ThreadPool::globalInstance()->hasTask())
	{
		suspend();
		return;
	}

	if (!ThreadPool::globalInstance()->hasIdleThread())
	{
		//有任务但是没有空闲线程。调用Sleep(0), 放弃当前cpu时间片，调度进程切换可调度线程
		Sleep(0);//SwitchToThread();
		return;
	}

	std::shared_ptr<TaskBase> pTask = ThreadPool::globalInstance()->takeTask();
	if (pTask.get())
	{
		ThreadPoolThread* t = ThreadPool::globalInstance()->popIdleThread();
		if (nullptr != t)
		{
			ThreadPool::globalInstance()->appendActiveThread(t);
			t->assignTask(pTask);
			t->resume();
			Sleep(1);
		}
		else
		{
			LOG_DEBUG("[ThreadPool] popIdleThread error!\n");
		}
	}
	else
	{
		LOG_DEBUG("[ThreadPool] takeTask error!\n");
	}
}

void ScheduleThread::switchToIdleThread(UINT threadId)
{
	ThreadPoolThread* t = ThreadPool::globalInstance()->takeActiveThread(threadId);
	if (nullptr != t)
	{
		ThreadPool::globalInstance()->pushIdleThread(t);
	}
	else
	{
		LOG_DEBUG("[ThreadPool] takeActiveThread error!\n");
	}
}