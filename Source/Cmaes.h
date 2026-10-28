#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

// CMA-ES (Hansen's covariance matrix adaptation evolution strategy), minimising
// a function of n values in [0, 1]: each generation samples lambda candidates
// around a mean, then moves the mean toward the best and reshapes the sampling
// distribution along the directions that paid off. Written from Hansen's
// "The CMA Evolution Strategy: A Tutorial" (2016) and his purecmaes reference;
// no code copied. Optional in Reference Match (useCmaes): at its budget (about 160
// renders over 21 settings) a pattern search did better (matchbench 6.5 dB vs
// 6.9); kept for when a learned starting point makes a longer search worthwhile.
class Cmaes
{
public:
    // lambdaOverride: a population size instead of the default 4 + 3 ln n (restarts grow it).
    Cmaes (std::vector<double> start, double sigma, unsigned seed, int lambdaOverride = 0)
        : n ((int) start.size()), mean (std::move (start)), sigma (sigma), rng (seed)
    {
        lambda = lambdaOverride > 0 ? lambdaOverride : 4 + (int) std::floor (3.0 * std::log ((double) n));
        mu = lambda / 2;
        for (int i = 0; i < mu; ++i)
            weights.push_back (std::log (mu + 0.5) - std::log (i + 1.0));
        const auto sum = std::accumulate (weights.begin(), weights.end(), 0.0);
        double sumSq = 0.0;
        for (auto& w : weights)
        {
            w /= sum;
            sumSq += w * w;
        }
        mueff = 1.0 / sumSq;

        cc = (4.0 + mueff / n) / (n + 4.0 + 2.0 * mueff / n);
        cs = (mueff + 2.0) / (n + mueff + 5.0);
        c1 = 2.0 / ((n + 1.3) * (n + 1.3) + mueff);
        cmu = std::min (1.0 - c1, 2.0 * (mueff - 2.0 + 1.0 / mueff) / ((n + 2.0) * (n + 2.0) + mueff));
        damps = 1.0 + 2.0 * std::max (0.0, std::sqrt ((mueff - 1.0) / (n + 1.0)) - 1.0) + cs;
        chiN = std::sqrt ((double) n) * (1.0 - 1.0 / (4.0 * n) + 1.0 / (21.0 * n * n));

        pc.assign ((size_t) n, 0.0);
        ps.assign ((size_t) n, 0.0);
        B.assign ((size_t) (n * n), 0.0);
        C.assign ((size_t) (n * n), 0.0);
        D.assign ((size_t) n, 1.0);
        for (int i = 0; i < n; ++i)
            B[(size_t) (i * n + i)] = C[(size_t) (i * n + i)] = 1.0;
    }

    int populationSize() const noexcept { return lambda; }

    // A generation of candidates, each clamped into [0, 1].
    const std::vector<std::vector<double>>& ask()
    {
        std::normal_distribution<double> normal;
        z.assign ((size_t) lambda, std::vector<double> ((size_t) n));
        y.assign ((size_t) lambda, std::vector<double> ((size_t) n));
        x.assign ((size_t) lambda, std::vector<double> ((size_t) n));
        for (int k = 0; k < lambda; ++k)
        {
            for (auto& v : z[(size_t) k])
                v = normal (rng);
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0; // y = B * (D .* z)
                for (int j = 0; j < n; ++j)
                    sum += B[(size_t) (i * n + j)] * D[(size_t) j] * z[(size_t) k][(size_t) j];
                y[(size_t) k][(size_t) i] = sum;
                x[(size_t) k][(size_t) i] = std::clamp (mean[(size_t) i] + sigma * sum, 0.0, 1.0);
            }
        }
        return x;
    }

    // Their costs, in the order ask() gave them.
    void tell (const std::vector<double>& cost)
    {
        std::vector<int> order ((size_t) lambda);
        std::iota (order.begin(), order.end(), 0);
        std::sort (order.begin(), order.end(), [&] (int a, int b) { return cost[(size_t) a] < cost[(size_t) b]; });

        // New mean from the best mu (by the clamped points, so it stays in range).
        std::vector<double> yw ((size_t) n, 0.0), zw ((size_t) n, 0.0), old = mean;
        std::fill (mean.begin(), mean.end(), 0.0);
        for (int r = 0; r < mu; ++r)
        {
            const auto k = (size_t) order[(size_t) r];
            for (int i = 0; i < n; ++i)
            {
                mean[(size_t) i] += weights[(size_t) r] * x[k][(size_t) i];
                zw[(size_t) i] += weights[(size_t) r] * z[k][(size_t) i];
            }
        }
        for (int i = 0; i < n; ++i)
            yw[(size_t) i] = (mean[(size_t) i] - old[(size_t) i]) / sigma;

        // Evolution paths.
        double psNorm = 0.0;
        for (int i = 0; i < n; ++i)
        {
            double bz = 0.0; // B * zw
            for (int j = 0; j < n; ++j)
                bz += B[(size_t) (i * n + j)] * zw[(size_t) j];
            ps[(size_t) i] = (1.0 - cs) * ps[(size_t) i] + std::sqrt (cs * (2.0 - cs) * mueff) * bz;
            psNorm += ps[(size_t) i] * ps[(size_t) i];
        }
        psNorm = std::sqrt (psNorm);
        ++generation;
        const auto hsig = psNorm / std::sqrt (1.0 - std::pow (1.0 - cs, 2.0 * generation)) / chiN < 1.4 + 2.0 / (n + 1.0);
        for (int i = 0; i < n; ++i)
            pc[(size_t) i] = (1.0 - cc) * pc[(size_t) i] + (hsig ? std::sqrt (cc * (2.0 - cc) * mueff) * yw[(size_t) i] : 0.0);

        // Covariance: rank-one (the path) plus rank-mu (the best steps).
        for (int i = 0; i < n; ++i)
            for (int j = 0; j <= i; ++j)
            {
                auto value = (1.0 - c1 - cmu) * C[(size_t) (i * n + j)]
                           + c1 * (pc[(size_t) i] * pc[(size_t) j] + (hsig ? 0.0 : cc * (2.0 - cc) * C[(size_t) (i * n + j)]));
                for (int r = 0; r < mu; ++r)
                {
                    const auto k = (size_t) order[(size_t) r];
                    value += cmu * weights[(size_t) r] * y[k][(size_t) i] * y[k][(size_t) j];
                }
                C[(size_t) (i * n + j)] = C[(size_t) (j * n + i)] = value;
            }

        sigma *= std::exp ((cs / damps) * (psNorm / chiN - 1.0));
        sigma = std::clamp (sigma, 1.0e-4, 0.5);
        decompose();
    }

    const std::vector<double>& getMean() const noexcept { return mean; }

private:
    // C = B diag(D^2) B^T by Jacobi rotations (n is small).
    void decompose()
    {
        std::vector<double> a = C;
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                B[(size_t) (i * n + j)] = i == j ? 1.0 : 0.0;

        for (int sweep = 0; sweep < 50; ++sweep)
        {
            double off = 0.0;
            for (int p = 0; p < n; ++p)
                for (int q = p + 1; q < n; ++q)
                    off += a[(size_t) (p * n + q)] * a[(size_t) (p * n + q)];
            if (off < 1.0e-20)
                break;

            for (int p = 0; p < n; ++p)
                for (int q = p + 1; q < n; ++q)
                {
                    const auto apq = a[(size_t) (p * n + q)];
                    if (std::abs (apq) < 1.0e-15)
                        continue;
                    const auto theta = (a[(size_t) (q * n + q)] - a[(size_t) (p * n + p)]) / (2.0 * apq);
                    const auto t = (theta >= 0.0 ? 1.0 : -1.0) / (std::abs (theta) + std::sqrt (theta * theta + 1.0));
                    const auto c = 1.0 / std::sqrt (t * t + 1.0), s = t * c;
                    for (int k = 0; k < n; ++k)
                    {
                        const auto akp = a[(size_t) (k * n + p)], akq = a[(size_t) (k * n + q)];
                        a[(size_t) (k * n + p)] = c * akp - s * akq;
                        a[(size_t) (k * n + q)] = s * akp + c * akq;
                    }
                    for (int k = 0; k < n; ++k)
                    {
                        const auto apk = a[(size_t) (p * n + k)], aqk = a[(size_t) (q * n + k)];
                        a[(size_t) (p * n + k)] = c * apk - s * aqk;
                        a[(size_t) (q * n + k)] = s * apk + c * aqk;
                    }
                    for (int k = 0; k < n; ++k)
                    {
                        const auto bkp = B[(size_t) (k * n + p)], bkq = B[(size_t) (k * n + q)];
                        B[(size_t) (k * n + p)] = c * bkp - s * bkq;
                        B[(size_t) (k * n + q)] = s * bkp + c * bkq;
                    }
                }
        }
        for (int i = 0; i < n; ++i)
            D[(size_t) i] = std::sqrt (std::max (1.0e-20, a[(size_t) (i * n + i)]));
    }

    int n, lambda = 0, mu = 0, generation = 0;
    std::vector<double> mean, weights;
    double sigma, mueff = 1.0, cc = 0.0, cs = 0.0, c1 = 0.0, cmu = 0.0, damps = 1.0, chiN = 1.0;
    std::vector<double> pc, ps, B, C, D;
    std::vector<std::vector<double>> z, y, x;
    std::mt19937 rng;
};
