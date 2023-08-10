#pragma once
#include <testutil/base_fixture.hpp>
#include <testutil/data_util.hpp>
#include <testutil/thread.hpp>
#include <glmnetpp_bits/elnet_driver/standardize.hpp>    // separately unit-tested

namespace glmnetpp {

struct LognetBasePack
{
    const double thr = 1e-20;
    const int maxit, nx, ne, nlam, kopt, isd, intr;
    const double alpha, flmin;

    // will be modified
    Eigen::VectorXd a0;
    Eigen::MatrixXd a;
    Eigen::VectorXi m;
    Eigen::VectorXi kin;
    Eigen::VectorXd dev; 
    Eigen::VectorXd alm;
    double dev0 = 0.0;
    int nlp = 0, jerr = 0, lmu = 0;
    
    const Eigen::VectorXd& w;
    const Eigen::VectorXd& ulam;
    const Eigen::VectorXd& vp;
    const Eigen::MatrixXd& cl;
    const Eigen::VectorXi& ju;

    LognetBasePack(
            int _maxit,
            int _nx,
            int _ne,
            int _nlam,
            int _kopt,
            int _isd,
            int _intr,
            double _alpha,
            double _flmin,
            const Eigen::VectorXd& _w,
            const Eigen::VectorXd& _ulam,
            const Eigen::VectorXd& _vp,
            const Eigen::MatrixXd& _cl,
            const Eigen::VectorXi& _ju)
        : maxit(_maxit), nx(_nx), ne(_ne), nlam(_nlam), kopt(_kopt), isd(_isd), intr(_intr)
        , alpha(_alpha), flmin(_flmin), w(_w), ulam(_ulam)
        , vp(_vp), cl(_cl), ju(_ju)
    {
        int p = _ju.size();
        a0.setZero(nlam);
        a.setZero(nx, nlam);
        m.setZero(p);
        kin.setZero(nlam);
        dev.setZero(nlam); 
        alm.setZero(nlam);
    }

    virtual void fit() =0;
    virtual void fit_old() =0;
};

struct lognet_base_fixture: 
    base_fixture,
    testing::WithParamInterface<
        std::tuple<size_t, size_t, size_t, size_t, size_t, double, double, bool, bool, int> >
{
    void SetUp() override
    {
        size_t seed, n, p;
        std::tie(seed, n, p, maxit, nlam, alpha, flmin, isd, intr, kopt) = GetParam();
        DataGen dgen(seed); // hehe
        X = dgen.make_X(n, p);
        auto beta = dgen.make_beta(p);
        w = dgen.make_w(n);
        Eigen::VectorXd prob = X * beta;
        prob = 1./ (1. + (-prob).array().exp());
        std::mt19937 gen(seed);
        y = y.NullaryExpr(n, [&](auto i) { 
                    auto x_cnt = std::uniform_int_distribution<int>(1, 10)(gen);
                    return std::binomial_distribution<int>(x_cnt, prob[i])(gen);
                });
        ju = dgen.make_ju(p);
        vp = dgen.make_vp(p);
        cl = dgen.make_cl(p);
        nx = dgen.make_nx(p);
        ne = dgen.make_ne(p);
        ulam = dgen.make_ulam(nlam);

        g = g.NullaryExpr(n, [&](auto) { 
                return 0.5+std::normal_distribution<double>()(gen);
                });

        Eigen::VectorXd xm(p), xs(p);
        LStandardize1::eval(X, w, ju, isd, intr, xm, xs);
    }

protected:
    Eigen::MatrixXd X, cl;
    Eigen::VectorXd y, g, xv, ulam, vp, w;
    Eigen::VectorXi ju;
    int nx, ne, maxit, nlam, isd, intr, kopt;
    double alpha, flmin;

    void check_pack(const LognetBasePack& actual,
                    const LognetBasePack& expected) const
    {
        EXPECT_EQ(actual.lmu, expected.lmu);
        EXPECT_EQ(actual.nlp, expected.nlp);
        EXPECT_EQ(actual.jerr, expected.jerr);
        EXPECT_FLOAT_EQ(actual.dev0, expected.dev0);

        expect_eq_vec(actual.kin, expected.kin);
        expect_eq_vec(actual.m, expected.m);

        expect_float_eq_vec(actual.a0, expected.a0);
        expect_near_mat(actual.a, expected.a, 1e-7);
        expect_near_vec(actual.dev, expected.dev, 1e-7);
        expect_float_eq_vec(actual.alm, expected.alm);
    }
};

struct splognet_base_fixture
    : lognet_base_fixture
{
    void SetUp() override
    {
        size_t seed, n, p;
        std::tie(seed, n, p, maxit, nlam, alpha, flmin, isd, intr, kopt) = GetParam();
        DataGen dgen(seed); // hehe
        X = dgen.make_X_sparse(n, p);
        auto beta = dgen.make_beta(p);
        y = dgen.make_y(X, beta);
        y.array() =  (y.array() > 0.).template cast<double>();
        w = dgen.make_w(n);
        ju = dgen.make_ju(p);
        vp = dgen.make_vp(p);
        cl = dgen.make_cl(p);
        nx = dgen.make_nx(p);
        ne = dgen.make_ne(p);
        ulam = dgen.make_ulam(nlam);

        xm.setZero(p); xs.setZero(p);
        SpLStandardize2::eval(X, w, ju, isd, intr, xm, xs);
    }

protected: 
    Eigen::SparseMatrix<double> X;
    Eigen::VectorXd xm, xs, w;
};

} // namespace glmnetpp
