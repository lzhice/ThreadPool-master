#pragma once
#include <windows.h>
#include <list>
#include <stack>
#include <atomic>
#include "Mutex.h"

class TaskBase;
class ThreadPool;

class ThreadPoolThread
{
public:
	ThreadPoolThread(ThreadPool* threadPool);
	~ThreadPoolThread();

public:
	bool start();
	void quit();
	//�̹߳���
	bool suspend();
	//�̹߳���ָ�
	bool resume();

	const UINT threadId() const { return m_nThreadID; }
	const int taskId();

	//������������߳���
	bool assignTask(std::shared_ptr<TaskBase> pTask);
	void detachTask();
	bool startTask();
	bool stopTask();

protected:
	virtual void exec();
	//����ֹͣ����ִ�е����񣬷���ȴ��������
	virtual void waitForDone();

private:
	static UINT WINAPI threadFunc(LPVOID pParam);

private:
	HANDLE m_hThread;
	UINT m_nThreadID;
	HANDLE m_hEvent;
	std::atomic<bool> m_bExit;
	TPLock m_mutex;

	std::shared_ptr<TaskBase> m_pTask;
	ThreadPool* m_pThreadPool;
};

class ActiveThreadList
{
public:
	ActiveThreadList();
	~ActiveThreadList();

public:
	bool append(ThreadPoolThread* t);
	bool remove(ThreadPoolThread* t);
	ThreadPoolThread* take(int task_id);
	ThreadPoolThread* take(UINT thread_id);
	ThreadPoolThread* pop_back();
	int size();
	bool isEmpty();
	bool clear();

private:
	std::list<ThreadPoolThread*>m_list;
	TPLock m_lock;
};

class IdleThreadStack
{
public:
	IdleThreadStack();
	~IdleThreadStack();

public:
	ThreadPoolThread* pop();
	bool push(ThreadPoolThread*);
	int size();
	bool isEmpty();
	bool clear();

private:
	std::stack<ThreadPoolThread*> m_stack;
	TPLock m_lock;
};
