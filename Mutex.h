#pragma once
#include "windows.h"

class CMutex
{
public:
	CMutex(void);
	~CMutex(void);

public:
	bool Lock();
	bool Unlock();

private:
	CRITICAL_SECTION m_cs;
};

