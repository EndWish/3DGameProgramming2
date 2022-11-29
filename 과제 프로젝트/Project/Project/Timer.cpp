#include "stdafx.h"
#include "Timer.h"


Timer::Timer() {
	timeElapsed = 0;		// ����ð�

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
	timeElapsed = (totalFrequency - lastFrequency) * period; // ƽ �� ������ ������ * �ֱ� = ��� �ð�

	// �ִ� FPS ���ѽ�
	if (_lockFPS > 0.0f) {
		while (timeElapsed < (1.0 / _lockFPS)) {	// ���� fps���� ���� �ð��� �������� ��� ��� ���
			QueryPerformanceCounter((LARGE_INTEGER*)&totalFrequency);
			timeElapsed = (totalFrequency - lastFrequency) * period;
		}
	}

	lastFrequency = totalFrequency;
	// FPS ���
	frameTimes.push(timeElapsed);
	sumFrameTime += timeElapsed;

	while (sumFrameTime > 1.0f) {
		sumFrameTime -= frameTimes.front();
		frameTimes.pop();
	}
}

void Timer::Reset() {
	QueryPerformanceCounter((LARGE_INTEGER*)&lastFrequency);		// ���� ���� ������ ����
	totalFrequency = lastFrequency;

	paused = false;
}
