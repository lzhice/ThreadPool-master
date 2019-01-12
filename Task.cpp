#include "Task.h"
#include "Mutex.h"

int TaskBase::s_id = 0;
TaskBase::TaskBase(bool bAutoDelete)
	: m_id(++s_id)
	, m_bAutoDelete(bAutoDelete)
{
}

TaskBase::~TaskBase()
{
}

const int TaskBase::id()
{
	return m_id;
}

bool TaskBase::isAutoDelete()
{
	return m_bAutoDelete;
}

//////////////////////////////////////////////////////////////////////////
TaskQueue::TaskQueue(void)
{
}

TaskQueue::~TaskQueue(void)
{
}

TaskBase* TaskQueue::pop()
{
	TaskBase* p = nullptr;
	m_lock.lock();
	if (!m_TaskQueue.empty())
	{
		p = m_TaskQueue.front();
		m_TaskQueue.pop_front();
	}
	m_lock.unLock();
	return p;
}

bool TaskQueue::push(TaskBase* t)
{
	if (!t)
	{
		return false;
	}

	m_lock.lock();
	m_TaskQueue.push_back(t);
	m_lock.unLock();
	return true;
}

bool TaskQueue::isEmpty()
{
	bool ret = false;
	m_lock.lock();
	ret = m_TaskQueue.empty();
	m_lock.unLock();
	return ret;
}

bool TaskQueue::pushFront(TaskBase* t)
{
	if (!t)
	{
		return false;
	}

	m_lock.lock();
	m_TaskQueue.push_front(t);
	m_lock.unLock();
	return true;
}

bool TaskQueue::clear()
{
	m_lock.lock();
	std::deque<TaskBase*>::iterator iter = m_TaskQueue.begin();
	for (; iter != m_TaskQueue.end(); ++iter)
	{
		if (nullptr != (*iter) && (*iter)->isAutoDelete())
		{
			delete (*iter);
		}
	}
	m_TaskQueue.clear();
	m_lock.unLock();
	return true;
}