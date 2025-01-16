#pragma once

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void	Launch(std::function<void(void)> callback);
	void	Join();

	static void InitTLS();
	static void DestroyTLS();

private:
	std::mutex					_lock;
	std::vector<std::thread>	_threads;
};


================================================================================
// ThreadManager.cpp 파일 내용
================================================================================

#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"

ThreadManager::ThreadManager()
{
	// Main Thread
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(std::function<void(void)> callback)
{
	std::lock_guard<std::mutex> guard(_lock);
	_threads.push_back(std::thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (std::thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	static std::atomic<__int32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}