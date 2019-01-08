#pragma once
#include <windows.h>
#include <list>
#include <stack>
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

    const UINT threadId() const
    {
        return m_threadId;
    }
    const int taskId();

    //������������߳���
    bool assignTask(TaskBase* pTask);
    void detachTask();
    bool startTask();
    bool stopTask();

protected:
    virtual void exec();
    //����ֹͣ����ִ�е����񣬷���ȴ��������
    virtual void waitForDone();

private:
    static UINT WINAPI threadProc(LPVOID pParam);

private:
    HANDLE m_hThread;
    UINT m_threadId;
    HANDLE m_hEvent;
    bool m_bExit;
    CMutex m_mutex;

    TaskBase* m_pTask;
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
    ThreadPoolThread* remove(int task_id);
    ThreadPoolThread* pop_back();
    int size();
    bool isEmpty();
    bool clear();

private:
    std::list<ThreadPoolThread*>m_list;
    CMutex m_mutex;
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
    CMutex m_mutex;
};
