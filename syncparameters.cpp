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
#include <stdexcept>
#include <numeric>
#include "syncparameters.h"

using namespace std::chrono_literals;

M6SS::SyncParameters::SyncParameters(const std::vector<int> &chs, int s, double pEB, const std::map<int, double> &pSR,
                                     const std::chrono::nanoseconds &tScan,
                                     const std::chrono::nanoseconds &tSwitch,
                                     const std::chrono::nanoseconds &tEB) {

    for (int channel: chs) {
        if (channel < 11 or channel > 26) {
            throw std::invalid_argument("chs can not contain channel numbers less than 11 and greater than 26.");
        }
    }

    for (auto it1 = chs.begin(); it1 != chs.end(); ++it1) {
        for (auto it2 = it1 + 1; it2 != chs.end(); ++it2) {
            if (*it1 == *it2) {
                throw std::invalid_argument("chs must contain unique elements.");
            }
        }
    }

    if (s <= 0) {
        throw std::invalid_argument("s must be greater than 0.");
    }

    if (std::gcd(chs.size(), s) != 1) {
        throw std::invalid_argument(
                "The number of channels and the number of slots (s) must be co-primes."
        );
    }

    if (pEB < 0 or pEB > 1) {
        throw std::invalid_argument("pEB is not a valid probability.");
    }

    for (int channel: chs) {
        auto it = pSR.find(channel);
        if (it == pSR.end()) {
            throw std::invalid_argument("pSR does not contain all the channels included in chs.");
        }

        if (it->second < 0 or it->second > 1) {
            throw std::invalid_argument("pSR contains an invalid probability.");
        }
    }

    if (pSR.size() > chs.size()) {
        throw std::invalid_argument("pSR contains channels that are not included in chs.");
    }

    if (tScan <= 0ns) {
        throw std::invalid_argument("tScan must be greater than 0.");
    }

    if (tSwitch < 0ns) {
        throw std::invalid_argument("tSwitch must be greater than or equal to 0.");
    }

    if (tEB < 0ns) {
        throw std::invalid_argument("tEB cannot be negative.");
    }

    chs_ = chs;
    s_ = s;
    pEB_ = pEB;
    pSR_ = pSR;
    tScan_ = tScan;
    tSwitch_ = tSwitch;
    tEB_ = tEB;

}

const std::vector<int> &M6SS::SyncParameters::getCHS() const {
    return chs_;
}

int M6SS::SyncParameters::getS() const {
    return s_;
}


double M6SS::SyncParameters::getPeb() const {
    return pEB_;
}

const std::map<int, double> &M6SS::SyncParameters::getPsr() const {
    return pSR_;
}

const std::chrono::nanoseconds &M6SS::SyncParameters::getTScan() const {
    return tScan_;
}


const std::chrono::nanoseconds &M6SS::SyncParameters::getTSwitch() const {
    return tSwitch_;
}

const std::chrono::nanoseconds &M6SS::SyncParameters::getTeb() const {
    return tEB_;
}
