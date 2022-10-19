#pragma once
class Timer	{
private:
	double timeElapsed;			// ����ð�

	UINT64 frequencyPerSec;	// CPU ������(Hz����)
	double period;				// �ֱ�(������ ����ð�)

	UINT64 totalFrequency;	// ���� ������
	UINT64 lastFrequency;	// ���� �����ӿ����� ���� ������
	UINT64 pauseFrequency;

	queue<double> frameTimes;	// ť �� ������ Ÿ�ӵ��� ���� 1�ʰ� �ǵ��� �Ͽ� FPS ���
	double sumFrameTime;		// ť �� ������ Ÿ�ӵ��� ���� ����

	bool paused;				// ���� ����

public:
	Timer();
	~Timer();

	double GetTimeElapsed() const;
	int GetFPS() const;
	bool IsPaused() const;
	
	void Tick(double lockFPS = 0.0f);	// �� ������ ����
	void Reset();


};

