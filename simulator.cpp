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
#include <algorithm>
#include <set>
#include "simulator.h"

using std::vector, std::chrono::nanoseconds, std::set, std::random_device, std::mt19937, std::uniform_int_distribution,
std::uniform_real_distribution;
using namespace std::chrono_literals;

M6SS::Simulator::Results &
M6SS::Simulator::run(const SyncParameters &syncParams, long numRuns, Results &results) {

    if (numRuns <= 0) {
        throw std::invalid_argument("The parameter numRuns must be greater than 0");
    }

    const vector<int> &chs = syncParams.getCHS(); /* the channel hopping sequence to be used. */
    const vector<int> &availableChannels = syncParams.getCHS();
    const int C = chs.size(); // the number of available channels in the network
    nanoseconds slotframeDuration = SyncParameters::DEFAULT_SLOT_DURATION * syncParams.getS();
    nanoseconds channelRotationCycle = C * slotframeDuration;
    thread_local random_device randomDevice;
    thread_local mt19937 randomGenerator1(randomDevice()), randomGenerator2(randomDevice()), randomGenerator3(
            randomDevice());

    uniform_int_distribution<int> uniformIntDistributionRC(0, availableChannels.size() - 1);
    uniform_real_distribution<long double> uniformRealDistribution0_1(0, 1);
    uniform_int_distribution<long long> startTimeDistribution(0, channelRotationCycle.count());

    // this map stores the number of synchronization attempts that finish in a specific (time) step
    std::map<long, long> counter;

    auto randomChannel = [&uniformIntDistributionRC, &availableChannels]() {
        return availableChannels[uniformIntDistributionRC(randomGenerator1)];
    };

    auto random = [&uniformRealDistribution0_1]() {
        // Returns a random floating point number in the range [0.0, 1.0)
        return uniformRealDistribution0_1(randomGenerator2);
    };

    auto randomScanStartTime = [&startTimeDistribution]() {
        // Returns a random time within the first channel rotation period
        return nanoseconds(startTimeDistribution(randomGenerator3));
    };

    std::chrono::duration<double> avg_st = 0s;


    for (long run = 0; run < numRuns; run++) {

        nanoseconds scanStartTime = randomScanStartTime();
        long long scanStartASN =
                scanStartTime / SyncParameters::DEFAULT_SLOT_DURATION; // = floor(scanStartTime / DEFAULT_SLOT_DURATION)

        /* Check if the scan starts within a minimal cell and after the transmission start time of frames. If not,
           set the absolute serial number (asn) to point to the first minimal cell after the scan start time */
        long long asn = scanStartASN % syncParams.getS() == 0 and scanStartTime <=
                                                                  scanStartASN * SyncParameters::DEFAULT_SLOT_DURATION +
                                                                  SyncParameters::DEFAULT_TX_OFFSET
                        ? scanStartASN : scanStartASN + syncParams.getS() - scanStartASN % syncParams.getS();

        // Select a random channel for the first scan period
        int lastSelectedChannel = randomChannel();
        nanoseconds lastSelectionTime = scanStartTime;
        nanoseconds nextSelectionTime = lastSelectionTime + syncParams.getTSwitch() + syncParams.getTScan();

        /* this flag indicates if the node switched to a new channel (i.e, the current channel is not the same with the
         * previous selected channel)*/
        bool channel_switch_flag = true;

        while (true) { // repeat for each minimal cell after the scan start time, until an EB is received successfully

            // tx is the time when a transmission start
            nanoseconds txTime = asn * SyncParameters::DEFAULT_SLOT_DURATION + SyncParameters::DEFAULT_TX_OFFSET;
            int minimalCellChannel = chs[asn % C]; // the channel used by the minimal cell
            int scannedChannel;

            if (availableChannels.size() > 1 and txTime >= nextSelectionTime) {
                /* The following loop simplistically simulates the next channel selections of the node up to the scan
                 * period covering txTime. An optimized version of the loop may be provided in the future.
                 */
                do {
                    scannedChannel = randomChannel();
                    channel_switch_flag = scannedChannel != lastSelectedChannel;
                    lastSelectedChannel = scannedChannel;
                    lastSelectionTime = nextSelectionTime;

                    if (channel_switch_flag) {
                        nextSelectionTime += syncParams.getTSwitch() + syncParams.getTScan();
                    } else {
                        nextSelectionTime += syncParams.getTScan();
                    }
                } while (nextSelectionTime <= txTime); // Until the scan period that covers the txTime

            } else {
                scannedChannel = lastSelectedChannel;
            }

            // check if the scan has started
            if (not channel_switch_flag or txTime >= lastSelectionTime + syncParams.getTSwitch()) {

                // check if an EB is received
                if (
                        minimalCellChannel == scannedChannel and
                        random() < syncParams.getPeb() * syncParams.getPsr().at(scannedChannel)

                        ) {

                    avg_st += (txTime - scanStartTime + syncParams.getTeb()) / (double) numRuns;
                    // calculate the current (time) step; that is, the step where the EB was found.
                    long long current_step = ceil((txTime - scanStartTime) * 1.0 / slotframeDuration);

                    if (counter.find(current_step) == counter.end()) {
                        counter[current_step] = 1;
                    } else {
                        counter[current_step] += 1;
                    }

                    break;

                }
            }

            asn += syncParams.getS(); // go to the next minimal cell

        }
    }

    // Set the avgSyncTime_ in the results
    results.avgSyncTime_ = avg_st;

    // Create CDF
    results.cdf_.assign(counter.rbegin()->first + 1, 0);

    long long sumCounters = 0;
    for (size_t i = 1; i < results.cdf_.size(); i++) {
        if (counter.find(i) != counter.end()) {
            sumCounters += counter[i];
        }
        results.cdf_[i] = static_cast<double>(sumCounters) / numRuns;
    }

    return results;
}

std::chrono::duration<double> M6SS::Simulator::Results::avgSyncTime() {
    return avgSyncTime_;
}

double M6SS::Simulator::Results::cdf(size_t steps) {
    if (steps < 1) {
        throw std::invalid_argument("steps must be greater than zero.");
    }

    if (steps >= cdf_.size()) {
        return 1.0;
    }

    return cdf_[steps];
}