#include <gtest/gtest.h>

#include "NonlinSolver.h"
#include <logog/include/logog.hpp>


class Ode1 final
        : public IFirstOrderImplicitOdeNewton
{
public:
    void assemble(const double t, Vector const& x,
                  Matrix& M, Matrix& K, Vector& b) override
    {
        (void) t; (void) x;

        Eigen::Matrix2d m;
        m << 1.0, 0.0,
             0.0, 1.0;
        M = m.sparseView();

        Eigen::Matrix2d k;
        k << 0.0, 1.0,
            -1.0, 0.0;
        K = k.sparseView();

        Eigen::Vector2d b_;
        b_ << 0.0, 0.0;
        b = b_;
    }

    void assembleJacobian(const double t, const Vector &x,
                          const double dxdot_dx, Matrix &Jac)
    {
        (void) t; (void) x;

        Eigen::Matrix2d m;
        m << 1.0, 0.0,
             0.0, 1.0;

        // TODO: \dot x contribution
        Eigen::Matrix2d k;
        k << 0.0, 1.0,
            -1.0, 0.0;

        Jac = (m*dxdot_dx + k).sparseView();
    }
};

Eigen::Vector2d ode1_solution(const double t) {
    Eigen::Vector2d v;
    v << cos(t), sin(t);
    return v;
}


class BackwardEuler : ITimeDiscretization
{
public:
    void pushState(const double t, Vector const& x) override {
        (void) t;
        _x = x;
    }

    void pushMatrices() override {}

    void setCurrentTime(const double t, const double delta_t) override {
        _t = t; _delta_t = delta_t;
    }

    double getCurrentTime() const override {
        return _t;
    }

    double getCurrentXWeight() override {
        return 1.0/_delta_t;
    }

    Vector getWeightedOldX() override {
        return _x / _delta_t;
    }

private:
    double _t = 9999.9999;
    double _delta_t = 8888.8888;
    Vector _x;
};



TEST(NumLibODEInt, PicardBwdEuler)
{
    Ode1 ode;
    TimeDiscretizedODESystemPicard<BackwardEuler> ode_sys(ode);

    const double tol = 1e-4;
    NonlinearSolverPicard picard(tol, 5);

    const double t_end = 0.1;
    const double delta_t = t_end/10.0;

    // initial condition
    const double t0 = 0.0;
    Eigen::Vector2d x0;
    x0 << 1.0, 0.0;

    Vector x(x0); // solution vector

    ode_sys.pushState(t0, x0); // push IC

    for (double t=t0+delta_t; t<=t_end; t+=delta_t)
    {
        INFO("time: %e, delta_t: %e", t, delta_t);
        ode_sys.setCurrentTime(t, delta_t);

        picard.solve(ode_sys, x);

        ode_sys.pushState(t, x);
        ode_sys.pushMatrices();

        INFO("x[0] = %10g, x[1] = %10g", x[0], x[1]);
        auto const v = ode1_solution(t);
        INFO("x[0] = %10g, x[1] = %10g (analytical solution)", v[0], v[1]);
    }
}


TEST(NumLibODEInt, NewtonBwdEuler)
{
    Ode1 ode;
    TimeDiscretizedODESystemNewton<BackwardEuler> ode_sys(ode);

    const double tol = 1e-4;
    NonlinearSolverNewton newton(tol, 5);

    const double t_end = 0.1;
    const double delta_t = t_end/10.0;

    // initial condition
    const double t0 = 0.0;
    Eigen::Vector2d x0;
    x0 << 1.0, 0.0;

    Vector x(x0); // solution vector

    ode_sys.pushState(t0, x0); // push IC

    for (double t=t0+delta_t; t<=t_end; t+=delta_t)
    {
        INFO("time: %e, delta_t: %e", t, delta_t);
        ode_sys.setCurrentTime(t, delta_t);

        newton.solve(ode_sys, x);

        ode_sys.pushState(t, x);
        ode_sys.pushMatrices();

        INFO("x[0] = %10g, x[1] = %10g", x[0], x[1]);
        auto const v = ode1_solution(t);
        INFO("x[0] = %10g, x[1] = %10g (analytical solution)", v[0], v[1]);
    }
}

