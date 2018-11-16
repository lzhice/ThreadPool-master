#pragma once

#include <deque>
#include "Mutex.h"

class TaskBase
{
public:
	TaskBase();
	virtual ~TaskBase();

	virtual void taskProc() = 0;
	virtual void cancel() = 0;
	
	int id();

protected:
	int m_id;

private:
	static int s_id;
};

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

public:
	TaskBase *pop();
	bool push(TaskBase *t);
	bool pushFront(TaskBase *t);//²åµ½¶ÓÊ×¡£
	bool isEmpty();
	bool clear();

private:
	std::deque<TaskBase *>m_TaskQueue;
	CMutex m_mutex;
};