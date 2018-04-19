#pragma once

#include <cstdint>
#include <SDL_timer.h>
#define FPS_DATA_FRAME_HISTORY_SIZE 1024

struct TimeSpan
{
	double start;
	double end;
};

void startTiming(TimeSpan& span);
double endTiming(TimeSpan& span);

struct FPSData
{
	TimeSpan curFrame;

	double totalTime;
	uint32_t numSamples;

	void(*logCallback)(double);

};

void startTiming(TimeSpan& span)
{
//	span.start = OS::getMilliseconds();
//	span.end = OS::getMilliseconds();
  span.start = SDL_GetTicks();
  span.end = SDL_GetTicks();
}

double endTiming(TimeSpan& span)
{
//	span.end = OS::getMilliseconds();
	span.end = SDL_GetTicks();
	return span.end - span.start;
}

void startTimingFrame(FPSData& span)
{
	startTiming(span.curFrame);
}

double endTimingFrame(FPSData& span)
{
	double frametime = endTiming(span.curFrame);

	span.numSamples++;
	span.totalTime += frametime;

	if (span.numSamples == FPS_DATA_FRAME_HISTORY_SIZE)
	{
		if (span.logCallback != nullptr)
		{
			span.logCallback(span.totalTime / (double)span.numSamples);
		}

		span.numSamples = 0;
		span.totalTime = 0;
	}

	return frametime;
}
