#pragma once

#include "ThreadPoolThread.h"
#include "Task.h"

enum PRIORITY
{
	NORMAL,
	HIGH
};

class ThreadPool
{
public:
	~ThreadPool();
	static ThreadPool& Instance();

public:
	//初始化线程池，创建n个线程的线程池。
	bool init(int threadCount);
	//停止所有的任务，并且将所有线程退出。
	bool waitForDone();

	//priority为优先级。高优先级的任务将被插入到队首
	bool addTask(TaskBase *t, PRIORITY priority);

	bool abortTask(int task_id);
	bool abortAllTask();

	//将线程从活动队列取出，放入空闲线程栈中。在取之前判断此时任务队列是否有任务,如任务队列为空时才挂起,否则从任务队列取任务继续执行
	virtual bool switchActiveThread(ThreadPoolThread *);

protected:
	ThreadPool();
	virtual ThreadPoolThread *popIdleThread();
	virtual TaskBase *getNextTask();

private:
	int m_nThreadNum;
	bool m_bInitialized;

	IdleThreadStack m_idleThreads;
	ActiveThreadList m_activeThreads;
	TaskQueue m_taskQueue;
	TaskQueue m_taskFinishQueue;
};