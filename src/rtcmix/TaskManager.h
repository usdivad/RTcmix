//
// C++ Interface: TaskManager
//
// Description: 
//
//
// Author: Douglas Scott <netdscott@netscape.net>, (C) 2010
//
//

#ifndef _TASKMANAGER_H_
#define _TASKMANAGER_H_

#include <vector>

using namespace std;

class Task
{
public:
	virtual ~Task() {}
	virtual void run()=0;
};

template <typename Object, typename Ret, Ret (Object::*Method)()>
		class NoArgumentTask : public Task
{
public:
	NoArgumentTask(Object *inObject) : mObject(inObject), mReturned(0) {}
	NoArgumentTask(const NoArgumentTask &rhs) : mObject(rhs.mObject), mReturned(0) {}
	virtual ~NoArgumentTask() {}
	virtual void run() { mReturned = (mObject->*Method)(); }
private:
	Object *mObject;
	Ret		mReturned;
};

template <typename Object, typename Ret, typename Arg, Ret (Object::*Method)(Arg)>
		class OneArgumentTask : public Task
{
public:
	OneArgumentTask(Object *inObject, Arg inArg)
: mObject(inObject), mArg(inArg), mReturned(0) {}
	OneArgumentTask(const OneArgumentTask &rhs) : mObject(rhs.mObject), mArg(rhs.mArg), mReturned(0) {}
	virtual ~OneArgumentTask() {}
	virtual void run() { mReturned = (mObject->*Method)(mArg); }
private:
	Object *mObject;
	Arg		mArg;
	Ret		mReturned;
};

template <typename Object, typename Ret, typename Arg1, typename Arg2, Ret (Object::*Method)(Arg1, Arg2)>
		class TwoArgumentTask : public Task
{
public:
	TwoArgumentTask(Object *inObject, Arg1 inArg1, Arg2 inArg2)
: mObject(inObject), mArg1(inArg1), mArg2(inArg2), mReturned(0) {}
	TwoArgumentTask(const TwoArgumentTask &rhs)
: mObject(rhs.mObject), mArg1(rhs.mArg1), mArg2(rhs.mArg2), mReturned(0) {}
	virtual ~TwoArgumentTask() {}
	virtual void run() { mReturned = (mObject->*Method)(mArg1, mArg2); }
private:
	Object *mObject;
	Arg1	mArg1;
	Arg2	mArg2;
	Ret		mReturned;
};

class ThreadPool;

class TaskManagerImpl
{
public:
	TaskManagerImpl();
	virtual ~TaskManagerImpl();
	void	addTask(Task *inTask);
	void	setRequestCount(int count);
	void	wait();
private:
	ThreadPool *mThreadPool;
};

class TaskManager
{
public:
	TaskManager();
	~TaskManager();
	template <typename Object, typename Ret, Ret (Object::*Method)()>
	inline void addTasks(vector<Object *> inObjects);
	template <typename Object, typename Ret, typename Arg, Ret (Object::*Method)(Arg)>
	inline void addTasks(vector<Object *> inObjects, Arg inArg);
	template <typename Object, typename Ret, typename Arg1, typename Arg2, Ret (Object::*Method)(Arg1, Arg2)>
	inline void addTasks(vector<Object *> inObjects, Arg1 inArg1, Arg2 inArg2);

protected:
	void waitForCompletion();
private:
	TaskManagerImpl	*mImpl;
};

template <typename Object, typename Ret, Ret (Object::*Method)()>
inline void TaskManager::addTasks(vector<Object *> inObjects)
{
	bool needToWait = !inObjects.empty();
	mImpl->setRequestCount(inObjects.size());
	for (typename vector<Object *>::iterator i = inObjects.begin(); i != inObjects.end(); ++i) {
		mImpl->addTask(new NoArgumentTask<Object, Ret, Method>(*i));
	}
	if (needToWait) waitForCompletion();
}

template <typename Object, typename Ret, typename Arg, Ret (Object::*Method)(Arg)>
inline void TaskManager::addTasks(vector<Object *> inObjects, Arg inArg)
{
	bool needToWait = !inObjects.empty();
	mImpl->setRequestCount(inObjects.size());
	for (typename vector<Object *>::iterator i = inObjects.begin(); i != inObjects.end(); ++i) {
		mImpl->addTask(new OneArgumentTask<Object, Ret, Arg, Method>(*i, inArg));
	}
	if (needToWait) waitForCompletion();
}

template <typename Object, typename Ret, typename Arg1, typename Arg2, Ret (Object::*Method)(Arg1, Arg2)>
inline void TaskManager::addTasks(vector<Object *> inObjects, Arg1 inArg1, Arg2 inArg2)
{
	bool needToWait = !inObjects.empty();
	mImpl->setRequestCount(inObjects.size());
	for (typename vector<Object *>::iterator i = inObjects.begin(); i != inObjects.end(); ++i) {
		Object *o = *i;
		mImpl->addTask(new TwoArgumentTask<Object, Ret, Arg1, Arg2, Method>(o, inArg1, inArg2));
	}
	if (needToWait) waitForCompletion();
}

#endif	// _TASKMANAGER_H_
