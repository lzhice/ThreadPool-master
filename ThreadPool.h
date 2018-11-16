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
	//��ʼ���̳߳أ�����n���̵߳��̳߳ء�
	bool init(int threadCount);
	//ֹͣ���е����񣬲��ҽ������߳��˳���
	bool waitForDone();

	//priorityΪ���ȼ��������ȼ������񽫱����뵽����
	bool addTask(TaskBase *t, PRIORITY priority);

	bool abortTask(int task_id);
	bool abortAllTask();

	//���̴߳ӻ����ȡ������������߳�ջ�С���ȡ֮ǰ�жϴ�ʱ��������Ƿ�������,���������Ϊ��ʱ�Ź���,������������ȡ�������ִ��
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