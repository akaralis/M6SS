/**
 * Copyright 2020-2021 Apostolos Karalis
 * This file is part of Minimal 6TiSCH Synchronization Simulator (M6SS).
 *
 * M6SS is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * M6SS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with M6SS.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author Apostolos Karalis <akaralis@unipi.gr>
 */

#include <cmath>
#include <functional>
#include <numeric>
#include "model.h"
#include "timeinterval.h"

using std::chrono::duration, std::chrono::nanoseconds, std::floor, std::ceil, std::size_t, std::pow;
using namespace std::chrono_literals;

M6SS::Model::Results &
M6SS::Model::calculate(const SyncParameters &syncParams, Results &results) {

    const double &Peb = syncParams.getPeb();
    const int C = syncParams.getCHS().size();
    const std::vector<int> &chs = syncParams.getCHS();
    const std::map<int, double> &Psr = syncParams.getPsr();
    const nanoseconds &Tscan = syncParams.getTScan();
    const nanoseconds Tsf = SyncParameters::DEFAULT_SLOT_DURATION * syncParams.getS();
    const nanoseconds &Teb = syncParams.getTeb();
    duration<double> Tavg_sync = 0s;
    size_t max_step;

    std::function<double(size_t)> Psync;

    auto sumOfExpectedValueInCases1and2 = [&Psync, &Tsf, &Teb, &max_step] {
        /* calculates the sum of Psync(k) * [(k-1) * Tsf + Tsf/2 + Teb] from k=1 to infinity */

        duration<double> sum = 0s;
        size_t k = 1;
        double cumulativeProb = 0;
        while (cumulativeProb < 1 - std::pow(10, -9)) { // Runs until the cumulative probability reaches 0.999999999.
            cumulativeProb += Psync(k);
            sum += Psync(k) * ((k - 1) * Tsf + Tsf / 2.0 + Teb);
            k += 1;
        }

        max_step = k - 1;

        return sum;
    };

    std::vector<int> W;
    W.reserve(C);
    for (int i = 1; i <= C; i++) {
        // note that both the slotOffset and the channelOffset of the minimal cell are zero
        W.push_back(chs[((i - 1) * syncParams.getS()) % C]);
    }

    auto X = [&W, &C](int k, int y) {
        return W[(y + k - 1) % C];
    };

    auto Pstep = [&C, &Peb, &Psr, &X](int k, int y) {
        return 1.0 / C * Peb * Psr.at(X(k, y));
    };

    if (Tscan < Tsf) { // Case 1: The scan period is shorter than the duration of a step (or a slotframe)

        auto Psync_cond = [&Pstep](int k, int y) {
            double prod = 1;
            for (int i = 1; i <= k - 1; i++) {
                prod *= 1 - Pstep(i, y);
            }
            prod *= Pstep(k, y);
            return prod;
        };

        Psync = [Psync_cond, &C](int k) {
            double sum = 0;
            for (int y = 0; y <= C - 1; y++) {
                sum += 1.0 / C * Psync_cond(k, y);
            }
            return sum;
        };

        Tavg_sync = sumOfExpectedValueInCases1and2();


    } else if (Tscan % Tsf == 0ns) { // Case 2: The scan period is an integer multiple of the step (or the slotframe)
        int n = Tscan / Tsf;

        auto Pstep_sp = [n, &C, &Peb, &Psr, &X, &Pstep](int k, int y) {
            int k_f = ((k - 1) / n) * n + 1;
            int Nchp = (k - k_f) / C;
            return pow(1 - Peb * Psr.at(X(k, y)), Nchp) * Pstep(k, y);
        };

        auto Qsp = [n, Pstep_sp](int i, int y) {
            double sum = 0;
            for (int k = (i - 1) * n + 1; k <= i * n; k++) {
                sum += Pstep_sp(k, y);
            }

            return 1 - sum;
        };

        auto Psync_cond = [n, Qsp, Pstep_sp](int k, int y) {
            double prod = 1;
            for (int i = 1; i <= (k - 1) / n; i++) {
                prod *= Qsp(i, y);
            }
            return prod * Pstep_sp(k, y);
        };

        Psync = [Psync_cond, &C](int k) {
            double sum = 0;
            for (int y = 0; y <= C - 1; y++) {
                sum += 1.0 / C * Psync_cond(k, y);
            }
            return sum;
        };

        Tavg_sync = sumOfExpectedValueInCases1and2();


    } else { // Case 3: The scan period is greater than the step, but is not an integer multiple of the step

        const double n = Tscan * 1.0 / Tsf;
        std::vector<double> pSyncArray; // an array to store Psync for each step
        pSyncArray.push_back(0);
        auto updatePSync = [&pSyncArray](size_t k, double p) {
            if (k >= pSyncArray.size()) {
                pSyncArray.push_back(p);
            } else {
                pSyncArray[k] += p;
            }
        };

        auto B = [n](size_t i) { return (i - 1) * n != floor((i - 1) * n); };

        std::function<duration<double>(double, size_t, const TimeInterval &, int)> recursiveCalc;
        recursiveCalc = [&](double q, size_t i, const TimeInterval &I, int y) -> duration<double> {

            if (I.isEmpty() or q < std::pow(10, -9)) {
                return 0s;
            }

            duration<double> res;
            // declare here the variables needed for recursive calls
            double Q_C, Q_NC;
            TimeInterval Rl(
                    i * Tscan % Tsf, // equivalent to (i * n - floor(i*n)) * Tsf,
                    Tsf
            );
            TimeInterval Ll(
                    0ns,
                    i * Tscan % Tsf // equivalent to (i*n - floor(i*n)) * Tsf
            );
            TimeInterval Z = B(i + 1) ? TimeInterval::intersection(I, Ll) : I;

            {   // we define here an internal scope to release the variables that do not need to be retained during the
                // recursive calls so that to prevent from stack overflow
                size_t k_f = B(i) ? ceil((i - 1) * n) : (i - 1) * n + 1;
                size_t k_l = ceil(i * n);
                bool doesTheFirstStepOfScanPeriodCoverEBPoint = !B(i) or I.isSubsetOf(
                        TimeInterval( //Rf
                                (i - 1) * Tscan % Tsf, // equivalent to ((i - 1) * n - floor((i - 1) * n)) * Tsf
                                Tsf)
                );

                double Pstep_first = doesTheFirstStepOfScanPeriodCoverEBPoint ? Pstep(k_f, y) : 0;
                double Psync_first = q * Pstep_first;
                duration<double> E_first =
                        Psync_first * ((k_f - 1) * Tsf + I.getStart().value() + I.length() / 2.0 + Teb);


                auto M = [&](size_t k) { //for kf <= k < kl
                    return doesTheFirstStepOfScanPeriodCoverEBPoint ? k - k_f + 1 : k - k_f;
                };

                auto Pstep_inter = [&](size_t k) {
                    return pow(1 - Peb * Psr.at(X(k, y)), (M(k) - 1) / C) * Pstep(k, y);
                };

                auto Psync_inter = [&](size_t k) { //for kf < k < kl
                    return q * Pstep_inter(k);
                };

                auto Einter = [&](size_t k) { // for kf < k < kl
                    return Psync_inter(k) * ((k - 1) * Tsf + I.getStart().value() + I.length() / 2.0 + Teb);
                };


                //Plsc -> Plast_step_covered
                double Plsc = B(i + 1) ? TimeInterval::intersection(I, Ll).length() * 1.0 / I.length() : 1;
                double Pstep_last = pow(1 - Peb * Psr.at(X(k_l, y)), (M(k_l - 1)) / C) * Pstep(k_l, y);
                double Psync_last = q * Plsc * Pstep_last;

                duration<double> Elast = (!Z.isEmpty() ?
                                          Psync_last * ((k_l - 1) * Tsf + Z.getStart().value() + Z.length() / 2.0 + Teb)
                                                       : 0s);

                double sum_p_inter_step = 0;
                for (size_t k = k_f + 1; k <= k_l - 1; k++) {
                    sum_p_inter_step += Pstep_inter(k);
                }

                Q_C = q * Plsc * (1 - (Pstep_first + Pstep_last + sum_p_inter_step));

                Q_NC = q * (TimeInterval::intersection(I, Rl).length() * 1.0 / I.length()) *
                       (1 - (Pstep_first + sum_p_inter_step));

                res = E_first; // the variable 'res' holds the result
                updatePSync(k_f, 1.0 / C * Psync_first); // for the calculation of CDF

                size_t k = k_f + 1;
                while (k <= k_l - 1) {
                    res += Einter(k);
                    updatePSync(k, 1.0 / C * Psync_inter(k)); // for the calculation of CDF
                    k++;
                }

                res += Elast;

                updatePSync(k_l, 1.0 / C * Psync_last); // for the calculation of CDF
            }
            res += recursiveCalc(Q_C, i + 1, Z, y);

            if (B(i + 1)) {
                res += recursiveCalc(Q_NC, i + 1, TimeInterval::intersection(I, Rl), y);
            }

            return res;
        };

        for (int y = 0; y < C; y++) {
            Tavg_sync += 1.0 / C * recursiveCalc(1.0, 1, TimeInterval(0ns, Tsf), y);
        }

        max_step = pSyncArray.size() - 1;

        Psync = [pSyncArray](int k) {
            return pSyncArray.at(k);
        };

    }

    results.avgSyncTime_ = Tavg_sync;

    results.cdf_.push_back(0);
    size_t k = 1;
    do {
        results.cdf_.push_back(results.cdf_[k - 1] + Psync(k));
        k += 1;
    } while (k <= max_step);

    return results;
}

std::chrono::duration<double> M6SS::Model::Results::avgSyncTime() {
    return avgSyncTime_;
}

double M6SS::Model::Results::cdf(size_t steps) {
    if (steps < 1) {
        throw std::invalid_argument("steps must be greater than zero.");
    }

    if (steps >= cdf_.size())
        return 1;

    return cdf_[steps];
}