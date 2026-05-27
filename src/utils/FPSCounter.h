#pragma once
#include <deque>

class FPSCounter {
public:
    void update(double currentTime) {
        if (lastTime > 0) {
            double delta = currentTime - lastTime;
            if (delta > 0) {
                frameTimes.push_back(delta);
                if (frameTimes.size() > 60)
                    frameTimes.pop_front();
            }
        }
        lastTime = currentTime;
    }

    int getFPS() const {
        if (frameTimes.empty()) return 0;
        double sum = 0;
        for (double t : frameTimes) sum += t;
        double avg = sum / frameTimes.size();
        return avg > 0 ? (int)(1.0 / avg) : 999;
    }

    double getFrameTime() const {
        if (frameTimes.empty()) return 0;
        double sum = 0;
        for (double t : frameTimes) sum += t;
        return sum / frameTimes.size() * 1000.0;  // ms
    }

private:
    double lastTime = 0;
    std::deque<double> frameTimes;
};
