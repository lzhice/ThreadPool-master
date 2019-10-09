ThreadPool
====================================================================
@version: 1.0.1  
@Author: Vilas Wang  
@Contact: QQ451930733  
@Email: vilas900420@gmail.com / 451930733@qq.com

Copyright © 2018-2019 Vilas Wang. All rights reserved.



## Detailed Description


ThreadPool模块，Windows平台下C++实现的线程池库。

ThreadPool线程池功能特性如下：
- 动态修改最大线程数
- 动态添加任务到线程池执行
- 多个任务并发执行
- 动态结束某个任务或者所有任务
- 任务执行的结果异步返回
- 所有方法线程安全




## How to use

yourtask.h:

```cpp
#include "Task.h"
class YourTask : public TaskBase
{
public:
	YourTask();
	~YourTask();

	virtual void exec() override;	//线程执行入口函数，类似于QRunable的run()
	virtual void cancel() override; //取消任务接口

	...
};
```


main.cpp:

```cpp
#include "yourtask.h"
#include "ThreadPool.h"

ThreadPool::globalInstance()->init(4);;	//初始化线程池，创建n个线程的线程池。
for (int i = 0; i < 10; ++i)
{
	ThreadPool::globalInstance()->addTask(std::make_shared<YourTask>());
}

ThreadPool::globalInstance()->waitForDone();	//尝试停止所有的任务，并且等待所有线程退出
```
