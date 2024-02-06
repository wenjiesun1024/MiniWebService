#ifndef MINI_WEB_SERVICE_TIMER_H
#define MINI_WEB_SERVICE_TIMER_H

#include <vector>

class TimerNode
{
public:
    int interval;
    int times;
    int threadNum;
};

class TimerHeap
{
public:
    TimerHeap() = default;
    ~TimerHeap() = default;

    void addTimer(TimerNode *timerNode);
    void delTimer(int i);

    void tick();
    void pop();
    void adjust(int i);
    void clear();
    void setThreadNum(int threadNum);

    void popTimer();

    int GetNextTick();

private:
    std::vector<TimerNode *> heap;
};

#endif // MINI_WEB_SERVICE_TIMER_H