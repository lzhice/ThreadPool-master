/*////////////////////////////////////////////////////////////////////
@Brief:		׷��C++����ڴ������ͷ�
@Author:	vilas wang
@Contact:	QQ451930733|451930733@qq.com

���÷���

1��Ԥ�����TRACE_CLASS_MEMORY_ENABLED

2������Ҫ׷�ٵ���Ĺ��캯����������������
����
Class A
{
public:
A() { TRACE_CLASS_CONSTRUCTOR(A); }
~A() { TRACE_CLASS_DESTRUCTOR(A); }
}

3: ������Ҫ֪�����ڴ������ͷ������ʱ��(��������˳�ǰ)��ӡ��Ϣ
TRACE_CLASS_PRINT();
///////////////////////////////////////////////////////////////////////////////////////*/

#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <map>

typedef std::map<std::string, int> TClassRefCount;

class Lock;
class ClassMemoryTracer
{
public:
	template <class T>
	static void addRef()
	{
		const char *name = typeid(T).name();
		std::string str(name);
		if (str.empty())
		{
			return;
		}
		m_lock->lock();
		auto iter = s_mapRefConstructor.find(str);
		if (iter == s_mapRefConstructor.end())
		{
			s_mapRefConstructor[str] = 1;
		}
		else
		{
			s_mapRefConstructor[str] = ++iter->second;
		}
		m_lock->unLock();
	}

	template <class T>
	static void decRef()
	{
		const char *name = typeid(T).name();
		std::string str(name);
		if (str.empty())
		{
			return;
		}
		m_lock->lock();
		auto iter = s_mapRefDestructor.find(str);
		if (iter == s_mapRefDestructor.end())
		{
			s_mapRefDestructor[str] = 1;
		}
		else
		{
			s_mapRefDestructor[str] = ++iter->second;
		}
		m_lock->unLock();
	}

	static void printInfo();

private:
	ClassMemoryTracer() {}
	ClassMemoryTracer(const ClassMemoryTracer &) {}
	ClassMemoryTracer &operator=(const ClassMemoryTracer &) {}

private:
#if _MSC_VER >= 1700
	static std::unique_ptr<Lock> m_lock;
#else
	static std::shared_ptr<Lock> m_lock;
#endif
	static TClassRefCount s_mapRefConstructor;
	static TClassRefCount s_mapRefDestructor;
};

class Lock
{
public:
	Lock(void);
	~Lock(void);

public:
	bool lock();
	bool unLock();

private:
	CRITICAL_SECTION m_cs;
};


#ifndef TRACE_CLASS_CONSTRUCTOR
#ifdef TRACE_CLASS_MEMORY_ENABLED
#define TRACE_CLASS_CONSTRUCTOR(T) ClassMemoryTracer::addRef<T>()
#else
#define TRACE_CLASS_CONSTRUCTOR(T) __noop
#endif
#endif

#ifndef TRACE_CLASS_DESTRUCTOR
#ifdef TRACE_CLASS_MEMORY_ENABLED
#define TRACE_CLASS_DESTRUCTOR(T) ClassMemoryTracer::decRef<T>()
#else
#define TRACE_CLASS_DESTRUCTOR(T) __noop
#endif
#endif

#ifndef TRACE_CLASS_PRINT
#ifdef TRACE_CLASS_MEMORY_ENABLED
#define TRACE_CLASS_PRINT() ClassMemoryTracer::printInfo()
#else
#define TRACE_CLASS_PRINT __noop
#endif
#endif
