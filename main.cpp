/**
 * Copyright 2020-2021 Apostolos Karalis
 * This file is part of Minimal 6TiSCH Synchronization Simulator (M6SS).
 *
 * M6SS is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * M6SS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY o FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with M6SS.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author Apostolos Karalis <akaralis@unipi.gr>
 */
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <fstream>
#include "simulator.h"
#include "model.h"

using namespace M6SS;
using namespace std::chrono_literals;

void generateSimStatsFig8();

int main() {
    std::cout << "<----------------------------M6SS project---------------------------->" << std::endl;
    std::cout << "This is an example program of calculating the average initial-synchronization time using (a) the "
                 "simulator and (b) the model." << std::endl;

    std::vector<int> chs = SyncParameters::DEFAULT_CHANNEL_HOPPING_SEQUENCES.at(4);
    std::map<int, double> pSR = {
            {11, 0.1},
            {13, 0.9},
            {14, 0.5},
            {12, 1}};

    SyncParameters settings(chs, 101, 0.9375, pSR, 5250ms, 0s, 4256us); // Tswitch is practically negligible
    std::cout << settings << std::endl;

    Simulator::Results simResults;
    Simulator::run(settings, 1000000, simResults);
    Model::Results modelResults;
    Model::calculate(settings, modelResults);

    std::cout << "<-----Average Synchronization Time----->" << std::endl;
    std::cout << "Simulator: " << simResults.avgSyncTime().count() << "s" << std::endl;
    std::cout << "Model: " << modelResults.avgSyncTime().count() << "s" << std::endl;
    std::cout << "<-------------------------------------->" << std::endl;

    return 0;

}

void generateSimStatsFig8() {
    auto calculateCI = [](std::vector<double> &values, double confLevel) {
        std::sort(values.begin(), values.end());
        double pl = (1 - confLevel) / 2.0;
        double pu = 1 - pl;
        long idxL = std::floor(values.size() * pl);
        long idxU = std::floor(values.size() * pu);
        return std::make_pair(values[idxL], values[idxU]);
    };

    constexpr int NUM_SLOTS = 101;
    constexpr long NUM_SAMPLES = 100; // for each scan period
    constexpr long SAMPLE_POINTS_PER_SAMPLE = 1000000;

    std::ofstream simStatsFig8CSV;
    simStatsFig8CSV.open("simStatsFig8.csv");
    if (!simStatsFig8CSV.is_open()) {
        std::cerr << "Error to open simStatsFig8.csv" << std::endl;
        return;
    }

    simStatsFig8CSV << "SD,c,s,b_avg,n,avgSyncTime,avgSyncTimeCIL,avgSyncTimeCIU" << std::endl;

    enum AveragePsrType {
        ZeroStdDev, MaxStdDev
    };

    auto generatePsr = [](const std::vector<int> &chs, double averagePsr, AveragePsrType averagePsrType) {
        std::map<int, double> pSR;
        if (averagePsrType == ZeroStdDev) {
            for (int ch: chs) { pSR[ch] = averagePsr; }
        } else {
            double targetSum = averagePsr * chs.size();
            int numOf1 = (int) targetSum;
            double rem = targetSum - numOf1;
            std::vector<double> temp;
            for (int i = 0; i < numOf1; i++) {
                temp.push_back(1);
            };
            if (temp.size() < chs.size()) { temp.push_back(rem); }
            while (temp.size() < chs.size()) { temp.push_back(0); }
            for (int i = 0; i < chs.size(); i++) { pSR[chs[i]] = temp[i]; }
        }
        return pSR;
    };

    double pEB = 1;
    for (int c: {4, 8, 12, 16}) {
        auto &chs = M6SS::SyncParameters::DEFAULT_CHANNEL_HOPPING_SEQUENCES.at(c);
        for (double averagePsr : {0.25, 0.5, 0.75, 1.0})
            for (auto averagePsrType: {ZeroStdDev, MaxStdDev}) {
                auto pSR = generatePsr(chs, averagePsr, averagePsrType);
                for (int i = 0; i <= 20; i++) {
                    for (int j = 1; j <= 4; j++) {
                        double n = i + j * 0.25;

                        std::chrono::nanoseconds tScan = std::chrono::round<std::chrono::nanoseconds>(
                                n * NUM_SLOTS * SyncParameters::DEFAULT_SLOT_DURATION
                        );
                        SyncParameters syncParameters(
                                chs, NUM_SLOTS, pEB, pSR, tScan, 0ns, 4256us
                        ); // 4256us -> we assume the max length EB

                        Simulator::Results simResults;
                        std::vector<double> averages;
                        averages.reserve(NUM_SAMPLES);
                        while (averages.size() < NUM_SAMPLES) {
                            Simulator::run(syncParameters, SAMPLE_POINTS_PER_SAMPLE, simResults);
                            averages.push_back(simResults.avgSyncTime().count());
                        }
                        auto ci = calculateCI(averages, 0.95);
                        double avg = std::accumulate(averages.begin(), averages.end(), 0.0) / averages.size();
                        simStatsFig8CSV << (averagePsrType == ZeroStdDev ? "0" : "max") << ","
                                               << c << "," << NUM_SLOTS << "," << pEB * averagePsr << "," << n
                                               << "," << avg << ","
                                               << std::get<0>(ci)
                                               << "," << std::get<1>(ci) << std::endl;
                    }
                }
            }
    }

    simStatsFig8CSV.close();
}
