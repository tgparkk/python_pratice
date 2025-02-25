#pragma once

extern class ThreadManager* GThreadManager;
extern class SendBufferManager* GSendBufferManager;

class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
};



================================================================================
// CoreGlobal.cpp file content
================================================================================

#include "pch.h"
#include "CoreGlobal.h"
#include "SendBuffer.h"
#include "ThreadManager.h"

ThreadManager* GThreadManager = nullptr;
SendBufferManager* GSendBufferManager = nullptr;

CoreGlobal::CoreGlobal()
{
	GThreadManager = new ThreadManager();
	GSendBufferManager = new SendBufferManager();
}

CoreGlobal::~CoreGlobal()
{
	delete GThreadManager;
	delete GSendBufferManager;
}