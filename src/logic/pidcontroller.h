#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H

class PIDController {
public:
    PIDController();
    void setParams(float kp, float ki, float kd);
    float calculate(float target, float actual, float dt);
    void reset();

private:
    float m_kp;         // 比例系数 P
    float m_ki;         // 积分系数 I
    float m_kd;         // 微分系数 D

    float m_lastError;  // 上一帧误差 e(k‑1)
    float m_prevError;  // 上上帧误差 e(k‑2)
    float m_output;     // PID累计输出（阀门开度，0‑100）
};
#endif // PIDCONTROLLER_H