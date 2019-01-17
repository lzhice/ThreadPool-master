#include "Task.h"
#include "Mutex.h"

std::atomic<int> TaskBase::s_id = 0;
TaskBase::TaskBase(bool bAutoDelete)
	: m_id(++s_id)
	, m_bAutoDelete(bAutoDelete)
{
}

TaskBase::~TaskBase()
{
}

const int TaskBase::id() const
{
	return m_id;
}

bool TaskBase::isAutoDelete() const
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

std::shared_ptr<TaskBase> TaskQueue::pop()
{
	std::shared_ptr<TaskBase> t;
	m_lock.lock();
	if (!m_TaskQueue.empty())
	{
		t = m_TaskQueue.front();
		m_TaskQueue.pop_front();
	}
	m_lock.unLock();
	return t;
}

bool TaskQueue::push(std::shared_ptr<TaskBase> t)
{
	if (!t.get())
	{
		return false;
	}

	m_lock.lock();
	m_TaskQueue.push_back(t);
	m_lock.unLock();
	return true;
}

bool TaskQueue::pushFront(std::shared_ptr<TaskBase> t)
{
	if (!t.get())
	{
		return false;
	}

	m_lock.lock();
	m_TaskQueue.push_front(t);
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

bool TaskQueue::clear()
{
	m_lock.lock();
	m_TaskQueue.clear();
	m_lock.unLock();
	return true;
}