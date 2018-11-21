ThreadPool moudle
========================================
@version:		&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;1.0.1<br/>
@Author:		&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Vilas Wang<br/>
@Contact:		&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;QQ451930733<br/>
@Email:			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;vilas900420@gmail.com / 451930733@qq.com

Copyright © 2018 Vilas Wang. All rights reserved.

** ThreadPool is free library licensed under the term of LGPL v3.0. **



## Detailed Description


ThreadPool模块是对C++写的线程池库，可以动态添加任务。
- 多任务并发执行
- 所有任务异步调用
- 所有方法线程安全




## How to use

```cpp
yourtask.h:
class YourTask : public TaskBase
{
public:
	YourTask();
	~YourTask();

	virtual void taskProc() override; //线程执行入口函数，类似于QRunable的run()
	virtual void cancel() override; //取消任务接口

	...
};

main.cpp:

#include "Task.h"
#include "ThreadPool.h"

ThreadPool::Instance().init(4);;	//初始化线程池，创建n个线程的线程池。
for (int i = 0; i < 10; ++i)
{
	YourTask *t = new YourTask();
	ThreadPool::Instance().addTask(t);
}

ThreadPool::Instance().waitForDone();	//停止所有的任务，并且将所有线程退出。
```
