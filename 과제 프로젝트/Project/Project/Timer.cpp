#include "stdafx.h"
#include "Timer.h"


Timer::Timer() {
	timeElapsed = 0;		// 경과시간

	QueryPerformanceFrequency((LARGE_INTEGER*)&frequencyPerSec);	
	period = 1.0 / frequencyPerSec;

	Reset();
	pauseFrequency = 0;

	sumFrameTime = 0;

}

Timer::~Timer() {

}

double Timer::GetTimeElapsed() const {
	return timeElapsed;
}

float Timer::GetTotalTime() const {
	return (float)totalFrequency;
}

int Timer::GetFPS() const {
	return (int)frameTimes.size() + 1;

}

bool Timer::IsPaused() const {
	return paused;
}

void Timer::Tick(double _lockFPS) {
	QueryPerformanceCounter((LARGE_INTEGER*)&totalFrequency);
	timeElapsed = (totalFrequency - lastFrequency) * period; // 틱 당 진행한 진동수 * 주기 = 경과 시간

	// 최대 FPS 제한시
	if (_lockFPS > 0.0f) {
		while (timeElapsed < (1.0 / _lockFPS)) {	// 아직 fps제한 보다 시간이 덜지났을 경우 계속 대기
			QueryPerformanceCounter((LARGE_INTEGER*)&totalFrequency);
			timeElapsed = (totalFrequency - lastFrequency) * period;
		}
	}

	lastFrequency = totalFrequency;
	// FPS 계산
	frameTimes.push(timeElapsed);
	sumFrameTime += timeElapsed;

	while (sumFrameTime > 1.0f) {
		sumFrameTime -= frameTimes.front();
		frameTimes.pop();
	}
}

void Timer::Reset() {
	QueryPerformanceCounter((LARGE_INTEGER*)&lastFrequency);		// 현재 누적 진동수 저장
	totalFrequency = lastFrequency;

	paused = false;
}
