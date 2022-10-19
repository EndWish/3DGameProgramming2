#pragma once
class Timer	{
private:
	double timeElapsed;			// 경과시간

	UINT64 frequencyPerSec;	// CPU 진동수(Hz단위)
	double period;				// 주기(진동당 경과시간)

	UINT64 totalFrequency;	// 누적 진동수
	UINT64 lastFrequency;	// 지난 프레임에서의 누적 진동수
	UINT64 pauseFrequency;

	queue<double> frameTimes;	// 큐 내 프레임 타임들의 합이 1초가 되도록 하여 FPS 계산
	double sumFrameTime;		// 큐 내 프레임 타임들의 합을 저장

	bool paused;				// 중지 여부

public:
	Timer();
	~Timer();

	double GetTimeElapsed() const;
	int GetFPS() const;
	bool IsPaused() const;
	
	void Tick(double lockFPS = 0.0f);	// 한 프레임 진행
	void Reset();


};

