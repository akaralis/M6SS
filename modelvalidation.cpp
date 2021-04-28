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
#include <random>
#include <cmath>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include <sstream>
#include "syncparameters.h"
#include "simulator.h"
#include "model.h"
#include "modelvalidation.h"

using std::chrono::nanoseconds, std::uniform_int_distribution, std::uniform_real_distribution, std::mt19937,
std::random_device, std::vector, std::thread, std::map;
using namespace std::chrono_literals;


int M6SS::ModelValidator::makeValidation(int numThreads) {

    if (numThreads < 1) {
        throw std::invalid_argument("numThreads must be greater than zero.");
    }

    std::mutex _mutex;
    prepareDBSession();

    auto comparisonWithSimulator = [&](auto tScanDistribution) {
        bool validationFailed = false;
        bool optimalScanPeriodFlag = true;

        auto worker = [&](int numCases) -> void {
            random_device randomDevice;
            mt19937 randomGenerator1(randomDevice()), randomGenerator2(randomDevice()),
                    randomGenerator3(randomDevice()), randomGenerator4(randomDevice()),
                    randomGenerator5(randomDevice()), randomGenerator6(randomDevice()),
                    randomGenerator7(randomDevice()), randomGenerator8(randomDevice());


            uniform_int_distribution<int> numChannelsDistribution(1, 16);
            uniform_int_distribution<int> channelsDistribution(11, 26);
            uniform_int_distribution<int> slotsDistribution(1, 10000);
            uniform_real_distribution<double> pEBDistribution(0.1,
                                                              std::nextafter(1, std::numeric_limits<double>::max()));
            uniform_real_distribution<double> avgPsrDistribution(0.1,
                                                                 std::nextafter(1, std::numeric_limits<double>::max()));
            uniform_int_distribution<int> tEBDistribution(1504, 4256);

            int c, s;
            double pEB;
            nanoseconds tEB;
            int i = 0;

            while (i < numCases) {
                c = numChannelsDistribution(randomGenerator1); // a random number of channels

                // select a random number of slots that is relatively prime to the number of channels that are in used
                do {
                    s = slotsDistribution(randomGenerator2);
                } while (std::gcd(s, c) != 1);

                std::vector<int> chs;
                chs.reserve(c);
                while (chs.size() < c) {
                    int newChannel;
                    while (std::find(chs.begin(), chs.end(), (newChannel = channelsDistribution(randomGenerator8))) !=
                           chs.end());
                    chs.push_back(newChannel);
                }

                // select randomly the probabilities pEB and pSR
                pEB = pEBDistribution(randomGenerator3);

                map<int, double> pSR;
                double targetAveragePsr = avgPsrDistribution(randomGenerator4);

                // Desiring to uniformly distribute the average Psr, instead of creating the Psr values of channels by
                // randomly selecting the values in the interval (0, 1], we select the Psr values in a random way that
                // achieves a desired average Psr.
                double targetSum = targetAveragePsr * c;
                double sum = 0;
                vector<double> temp;
                temp.reserve(c);

                for (int j = 0; j < c; j++) {
                    double minP = targetSum - sum - c + (j + 1), maxP = targetSum - sum - (c - (j + 1)) * 0.1;

                    if (minP < 0.1) {
                        minP = 0.1;
                    }
                    if (maxP > 1) {
                        maxP = 1;
                    }

                    uniform_real_distribution<double> pSRDistribution(minP, std::nextafter(maxP,
                                                                                           std::numeric_limits<double>::max()));

                    temp.push_back(j == c - 1 ? targetSum - sum : pSRDistribution(randomGenerator7));
                    sum += temp.back();
                }

                std::shuffle(temp.begin(), temp.end(), randomGenerator7);
                for (int j = 0; j < c; j++) {
                    pSR[chs[j]] = temp[j];
                }

                // select randomly a value for the ratio of tScan to slotframe
                auto n = tScanDistribution(randomGenerator5);

                // the rounding has effect only when n is not integer
                nanoseconds tScan = std::chrono::round<nanoseconds>(
                        n * s * SyncParameters::DEFAULT_SLOT_DURATION
                );

                tEB = nanoseconds(tEBDistribution(randomGenerator6));

                SyncParameters syncParameters(chs, s, pEB, pSR, tScan, 0ns, tEB);

                Simulator::Results sim_results;
                Simulator::run(syncParameters, NUM_SIM_SAMPLES_PER_CASE, sim_results);

                Model::Results model_results;
                Model::calculate(syncParameters, model_results);

                double relativeErrorInAVG =
                        std::chrono::abs(model_results.avgSyncTime() - sim_results.avgSyncTime()) /
                        sim_results.avgSyncTime();

                double maxAbsoluteErrorInCDF = -1;

                for (int k = 1; model_results.cdf(k) < 1 or sim_results.cdf(k) < 1; k++) {
                    double absoluteErrorInCDF = std::abs(model_results.cdf(k) - sim_results.cdf(k));

                    if (absoluteErrorInCDF > maxAbsoluteErrorInCDF) {
                        maxAbsoluteErrorInCDF = absoluteErrorInCDF;
                    }
                }

                nanoseconds optimalTscan = c * s * SyncParameters::DEFAULT_SLOT_DURATION;
                // compare with the optimal value of scan period (i.e., c slotframes)
                SyncParameters syncParametersWithOptimalScanPeriod(chs, s, pEB, pSR, optimalTscan, 0ns, tEB);
                Model::Results modelResultsWithOptimalScanPeriod;
                Model::calculate(syncParametersWithOptimalScanPeriod, modelResultsWithOptimalScanPeriod);

                auto isOptimalScanPeriodValid = [&model_results, &modelResultsWithOptimalScanPeriod]() {
                    return model_results.avgSyncTime() >= modelResultsWithOptimalScanPeriod.avgSyncTime() or
                           // due to the possible precision error we also check if the two values are equal with the precision
                           // of six decimal places (i.e., with microsecond accuracy)
                           ((long long) model_results.avgSyncTime().count() * 1000000) ==
                           ((long long) modelResultsWithOptimalScanPeriod.avgSyncTime().count() * 1000000);
                };

                // save statistics
                if (numThreads > 1) {
                    std::lock_guard<std::mutex> guard(_mutex);
                    save(syncParameters, relativeErrorInAVG, maxAbsoluteErrorInCDF);

                    if (validationFailed) { // check if another thread set validationFailed = true
                        return;
                    }

                    if (relativeErrorInAVG > MAX_ALLOWED_ERROR or maxAbsoluteErrorInCDF > MAX_ALLOWED_ERROR) {
                        validationFailed = true;
                        return;
                    }

                    if (!isOptimalScanPeriodValid()) {
                        optimalScanPeriodFlag = false;
                    }

                } else {
                    save(syncParameters, relativeErrorInAVG, maxAbsoluteErrorInCDF);

                    if (relativeErrorInAVG > MAX_ALLOWED_ERROR or maxAbsoluteErrorInCDF > MAX_ALLOWED_ERROR) {
                        validationFailed = true;
                        return;
                    }

                    if (!isOptimalScanPeriodValid()) {
                        optimalScanPeriodFlag = false;
                    }
                }

                i++;
            }
        };

        if (numThreads == 1) {
            worker(NUM_RANDOM_CASES);
        } else {
            vector<thread> threads;
            for (int i = 1; i <= numThreads; i++) {
                if (NUM_RANDOM_CASES % numThreads >= i) {
                    threads.push_back(thread(worker, NUM_RANDOM_CASES / numThreads + 1));
                } else {
                    threads.push_back(thread(worker, NUM_RANDOM_CASES / numThreads));
                }
            }

            for (auto &t : threads) {
                t.join();
            }
        }

        if (validationFailed) {
            return -1;
        } else if (!optimalScanPeriodFlag) {
            return 0;
        }
        return 1;

    };

    int comparisonRes1 = -1, comparisonRes2 = -1, comparisonRes3 = -1;
    int finalRes;
    if (

            (comparisonRes1 = comparisonWithSimulator(uniform_real_distribution<>(0.1, 1))) == -1 // for n in (0,1)
            or
            (comparisonRes2 = comparisonWithSimulator(uniform_int_distribution<>(1, 100))) == -1 // for n in N*
            or
            // for n real greater than 1 and not integer
            (comparisonRes3 = comparisonWithSimulator(custom_real_n_distribution<>(1, 100))) == -1

            ) {
        finalRes = -1;
    } else if (comparisonRes1 == 0 or comparisonRes2 == 0 or comparisonRes3 == 0) {
        finalRes = 0;
    } else {
        finalRes = 1;
    }

    closeDBSession();
    return finalRes;

}

void M6SS::ModelValidator::prepareDBSession() {
    stmt = nullptr;
    insertCounter = 0;

    if (sqlite3_open("modelvalidation.db", &db)) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    std::string createTableStatement = "CREATE TABLE IF NOT EXISTS statistics ("
                                       "c INTEGER,"
                                       "chs TEXT,"
                                       "s INTEGER,"
                                       "pEB REAL,"
                                       "averagePsr REAL,"
                                       "Psr TEXT,"
                                       "tSCAN INTEGER,"
                                       "relativeErrorInAVG REAL,"
                                       "maxAbsoluteErrorInCDF REAL"
                                       ")";

    char *err_msg = nullptr;
    if (
            sqlite3_exec(db, createTableStatement.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK or
            sqlite3_exec(db, "PRAGMA cache_size=10000", nullptr, nullptr, &err_msg) != SQLITE_OK
            ) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        closeDBSession();
        throw std::runtime_error(error);
    }


    if (sqlite3_prepare_v2(db, "INSERT INTO statistics VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, nullptr) !=
        SQLITE_OK) {
        closeDBSession();
        throw std::runtime_error("Fail to prepare statement.");
    }
}


void M6SS::ModelValidator::save(const SyncParameters &syncParameters,
                                double relativeErrorInAVG,
                                double maxAbsoluteErrorInCDF) {

    std::string stringCHS, stringPsr;
    std::stringstream ss, ss2;
    ss << "[";
    for (auto it = syncParameters.getCHS().begin(); it != syncParameters.getCHS().end(); ++it) {
        ss << *it;
        if (it + 1 != syncParameters.getCHS().end()) {
            ss << ",";
        }
    }
    ss << "]";
    ss >> stringCHS;

    ss2 << "{";
    for (auto it = syncParameters.getPsr().begin(); it != syncParameters.getPsr().end();) {
        ss2 << it->first <<":" << it->second;
        if (++it != syncParameters.getPsr().end()) {
            ss2 << ",";
        }
    }
    ss2 << "}";
    ss2 >> stringPsr;


    if (insertCounter % NUM_INSERTIONS_TO_CACHE == 0) {
        char *err_msg = nullptr;
        if (sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string error = err_msg;
            sqlite3_free(err_msg);
            closeDBSession();
            throw std::runtime_error(error);
        }
    }

    if (
            sqlite3_bind_int(stmt, 1, syncParameters.getCHS().size()) != SQLITE_OK or
            sqlite3_bind_text(stmt, 2, stringCHS.c_str(), -1, nullptr) != SQLITE_OK or
            sqlite3_bind_int(stmt, 3, syncParameters.getS()) != SQLITE_OK or
            sqlite3_bind_double(stmt, 4, syncParameters.getPeb()) != SQLITE_OK or
            sqlite3_bind_double(stmt, 5, [&]() {
                double sum = 0;
                for (auto &element: syncParameters.getPsr()) { sum += element.second; }
                return sum / syncParameters.getPsr().size();
            }()) != SQLITE_OK or

            sqlite3_bind_text(stmt, 6, stringPsr.c_str(), -1, nullptr) != SQLITE_OK or
            sqlite3_bind_int64(stmt, 7, syncParameters.getTScan().count()) != SQLITE_OK or
            sqlite3_bind_double(stmt, 8, relativeErrorInAVG) != SQLITE_OK or
            sqlite3_bind_double(stmt, 9, maxAbsoluteErrorInCDF) != SQLITE_OK or
            sqlite3_step(stmt) != SQLITE_DONE
            ) {
        throw std::runtime_error("Fail to bind arguments.");

    }

    insertCounter++;

    if (insertCounter % NUM_INSERTIONS_TO_CACHE == 0) {
        char *err_msg = nullptr;
        if (sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string error = err_msg;
            sqlite3_free(err_msg);
            closeDBSession();
            throw std::runtime_error(error);
        }
    }

    sqlite3_reset(stmt);
}

void M6SS::ModelValidator::closeDBSession() {
    char *err_msg = nullptr;

    if (insertCounter % NUM_INSERTIONS_TO_CACHE != 0) { // check if a transaction is open
        sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, &err_msg);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (err_msg) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error(err_msg);
    }

}

template<class Realtype>
M6SS::ModelValidator::custom_real_n_distribution<Realtype>::custom_real_n_distribution(Realtype a, Realtype b) : a_(a),
                                                                                                                 b_(b) {}

template<class Realtype>
template<class Generator>
double M6SS::ModelValidator::custom_real_n_distribution<Realtype>::operator()(Generator &g) {
    std::bernoulli_distribution d(0.5);
    if (d(g)) {
        // Create a n that can lead to scan periods that will not finish in a switch step.
        // For example if n=3.5, the second scan period in the scan process will not finish in a switch step.
        // Note that, assuming the use of the IEEE 754 floating point standard, only decimal parts that are power of
        // 2 can be exactly represented.
        return uniform_int_distribution<long long>(static_cast<long long>(std::ceil(a_)),
                                                   static_cast<long long>(std::floor(b_)))(g)
               + std::pow(2, -uniform_int_distribution(1, 4)(g));

    } else {
        return uniform_real_distribution<Realtype>(a_, b_)(g);
    }

}