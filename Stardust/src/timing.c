#include "timing.h"
#include <Windows.h>

void InitTimer()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	timerFrequency = freq.QuadPart;
}

void StartTimer(Timer* t)
{
	LARGE_INTEGER s;
	QueryPerformanceCounter(&s);

	t->start = s.QuadPart;
}

double EndTimer(Timer* t)
{
	LARGE_INTEGER e;
	QueryPerformanceCounter(&e);

	t->end = e.QuadPart;

	long long delta = (t->end - t->start) * 1000000;
	return (double)(delta / timerFrequency);
}