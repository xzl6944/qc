#include "pidcontroller.h"
#include <cmath>

/**
 * @brief PIDController 构造函数，初始化默认PID参数与内部状态
 * 默认参数 Kp=5.0，Ki=0.5，Kd=0.1；全部误差、输出清零
 */
PIDController::PIDController()
    : m_kp(5.0f), m_ki(0.5f), m_kd(0.1f),
    m_lastError(0), m_prevError(0), m_output(0)
{
}

void PIDController::setParams(float kp, float ki, float kd)
{
    m_kp = kp;
    m_ki = ki;
    m_kd = kd;
}

/**
 * @brief reset 清空PID历史状态
 * 场景：空调开关机、模式切换、目标温度大幅跳变时调用，防止历史误差造成异常输出
 */
void PIDController::reset()
{
    m_lastError = 0;
    m_prevError = 0;
    m_output = 0;
}

/**
 * @brief calculate 增量式PID计算核心
 * 算法：增量式 Δu(k) = Kp*(e(k)-e(k‑1)) + Ki*e(k)*dt + Kd*(e(k)-2e(k‑1)+e(k‑2))/dt
 * 输出 u(k) = u(k‑1) + Δu(k)，并限制输出0‑100
 *
 * 注意本控制器误差定义：error = actual − target
 *  > 制冷模式：实际温度高于目标 → error>0 → PID输出增大，开大制冷阀门
 *  > 制热模式：实际温度低于目标 → error<0 → PID输出增大，开大制热阀门
 *
 * @param target 目标温度
 * @param actual 当前实际温度
 * @param dt 仿真时间步
 * @return 阀门输出开度 0~100
 */
float PIDController::calculate(float target, float actual, float dt)
{
    // 计算当前误差：实际值 − 目标值
    float error = actual - target;

    // 积分项条件：只有误差绝对值 ≤10℃才启用积分，误差过大关闭积分，抑制积分饱和
    float ki_action = 0.0f;
    if (std::abs(error) <= 10.0f)
    {
        ki_action = m_ki * error * dt;
    }

    // 增量式PID公式：计算本次输出变化量 deltaU
    // 比例项：Kp*(e(k)-e(k‑1))
    // 积分项：Ki*e(k)*dt（条件启用）
    // 微分项：Kd*(e(k)-2e(k‑1)+e(k‑2)) / dt
    float deltaU = m_kp * (error - m_lastError)
                   + ki_action
                   + m_kd * (error - 2 * m_lastError + m_prevError) / dt;

    // 将增量叠加到历史输出上，得到新输出
    m_output += deltaU;

    // 输出限幅：阀门开度只能在0~100%之间
    if (m_output > 100.0f)
        m_output = 100.0f;
    if (m_output < 0.0f)
        m_output = 0.0f;

    // 更新误差历史记录，为下一次计算做准备
    m_prevError = m_lastError;
    m_lastError = error;

    return m_output;
}
