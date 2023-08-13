#pragma once

static long long timerFrequency;

void InitTimer();


typedef struct
{
	long long start;
	long long end;
} Timer;

void StartTimer(Timer* t);
double EndTimer(Timer* t);
