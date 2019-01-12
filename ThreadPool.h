#pragma once

#include "ThreadPoolThread.h"
#include "Task.h"

#define DEFAULT_THREAD_COUNT 4

class ThreadPool
{
public:
	enum Priority
	{
		Normal,
		High
	};

	~ThreadPool();
	static ThreadPool* Instance();

public:
	//��ʼ���̳߳أ�����n���̵߳��̳߳ء�
	bool init(int threadCount = DEFAULT_THREAD_COUNT);
	//ֹͣ���е����񣬲��ҽ������߳��˳���
	bool waitForDone();

	//priorityΪ���ȼ��������ȼ������񽫱����뵽����
	bool addTask(TaskBase* t, Priority p);

	bool abortTask(int taskId);
	bool abortAllTask();

	//�߳��������֪ͨȡ����
	//���̴߳ӻ����ȡ������������߳�ջ�С�
	//��ȡ����֮ǰ�����ж���������Ƿ�������,��Ϊ��ʱ�Ź���,������������ȡ�������ִ��
	virtual bool onThreadFinished(ThreadPoolThread*);

public:
	class ThreadPoolCallBack
	{
	public:
		virtual void onTaskFinished(int task_id) = 0;
	};

	void setCallBack(ThreadPoolCallBack* pCallBack);
	void onTaskFinished(int taskId);

protected:
	ThreadPool();
	virtual ThreadPoolThread* popIdleThread();
	virtual TaskBase* getNextTask();

private:
	int m_nThreadNum;
	bool m_bInitialized;
	ThreadPoolCallBack* m_pCallBack;

	IdleThreadStack m_idleThreads;
	ActiveThreadList m_activeThreads;
	TaskQueue m_taskQueue;
};