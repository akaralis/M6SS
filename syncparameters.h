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
#ifndef M6SS_SYNCPARAMETERS_H
#define M6SS_SYNCPARAMETERS_H

#include <chrono>
#include <iostream>
#include <vector>
#include <map>

namespace M6SS {

    /***
     * This class represents the parameters of the synchronization procedure.
     */
    class SyncParameters {
    public:
        /***
         * Initializes a new instance of the SyncParameters class.
         * @param chs the channel hopping sequence.
         * @param s the number of slots in the slotframe.
         * @param pEB the probability of an EB transmission (in the minimal cell).
         * @param pSR a map that contains for each channel in chs the probability of the successful reception of an EB
         * (in the context of the minimal cell).
         * @param tScan the time that a selected channel is scanned for an EB.
         * @param tSwitch the channel switch delay.
         * @param tEB the time required for the transmission of an EB. Note that we allow Teb to be set to zero.
         * @throw std::invalid_argument if:
         * (a) chs contains channels that are not in the set {11, 12, ..., 26}, or,
         * (b) chs contains a channel multiple times, or,
         * (c) s is not a positive integer, or,
         * (d) s and the size of chs (i.e., the number of channels) are not co-primes, or,
         * (e) pEB is an invalid probability (i.e, less than 0 or greater than 1), or,
         * (f) pSR does not contain all the channels included in chs, or,
         * (g) pSR contains channels that are not included in chs, or,
         * (h) pSR contains an invalid probability (i.e, less than 0 or greater than 1), or,
         * (i) tScan is not a positive time, or,
         * (k) tSwitch is a negative time, or,
         * (l) tEB is a negative time.
         */
        SyncParameters(const std::vector<int> &chs, int s, double pEB, const std::map<int, double> &pSR,
                       const std::chrono::nanoseconds &tScan,
                       const std::chrono::nanoseconds &tSwitch,
                       const std::chrono::nanoseconds &tEB);

        /**
         * Returns the channel hopping sequence of the network.
         * @return the channel hopping sequence of the network.
         */
        [[nodiscard]] const std::vector<int> &getCHS() const;

        /**
         * Returns the number of slots in the slotframe.
         * @return the number of slots in the slotframe.
         */
        [[nodiscard]] int getS() const;

        /**
         * Returns the transmission probability of an EB (in the minimal cell).
         * @return the probability of an EB transmission (in the minimal cell).
         */
        [[nodiscard]] double getPeb() const;

        /**
         * Returns a map that contains for each channel the probability of the successful reception of an EB (in the
         * context of the minimal cell).
         * @return returns a map that contains for each channel the probability of the successful reception of an EB
         */
        [[nodiscard]] const std::map<int, double> &getPsr() const;

        /**
         * Returns the time that a selected channel is scanned for an EB.
         * @return the time that a selected channel is scanned for an EB.
         */
        [[nodiscard]] const std::chrono::nanoseconds &getTScan() const;

        /**
         * Returns the channel switch delay.
         * @return the channel switch delay.
         */
        [[nodiscard]] const std::chrono::nanoseconds &getTSwitch() const;

        /**
         * Returns the time required for the transmission of an EB.
         * @return the time required for the transmission of an EB.
         */
        [[nodiscard]] const std::chrono::nanoseconds &getTeb() const;

        /**
         * The default duration of a timeslot in the 2.4Ghz band.
         */
        static constexpr auto DEFAULT_SLOT_DURATION = std::chrono::milliseconds(10);

        /**
         * The default channel hopping sequences as defined by the standard, depending on the number of channels
         * used by the network (maximum 16 in the 2.4 Ghz band).
         */
        inline static const std::map<int, std::vector<int> > DEFAULT_CHANNEL_HOPPING_SEQUENCES{
                {1,  {11}},
                {2,  {11, 12}},
                {3,  {11, 13, 12}},
                {4,  {11, 13, 14, 12}},
                {5,  {11, 13, 14, 15, 12}},
                {6,  {16, 12, 15, 11, 13, 14}},
                {7,  {14, 13, 15, 11, 16, 12, 17}},
                {8,  {16, 12, 15, 11, 14, 13, 17, 18}},
                {9,  {11, 13, 12, 16, 17, 18, 19, 14, 15}},
                {10, {16, 12, 19, 13, 17, 14, 20, 18, 15, 11}},
                {11, {16, 12, 11, 20, 17, 18, 14, 13, 19, 15, 21}},
                {12, {16, 19, 15, 20, 13, 12, 21, 18, 22, 11, 14, 17}},
                {13, {15, 13, 20, 19, 17, 23, 16, 12, 21, 22, 14, 11, 18}},
                {14, {14, 11, 21, 18, 16, 19, 17, 20, 22, 24, 15, 23, 12, 13}},
                {15, {17, 22, 24, 18, 12, 11, 25, 13, 19, 16, 14, 15, 20, 23, 21}},
                {16, {16, 17, 23, 18, 26, 15, 25, 22, 19, 11, 12, 13, 24, 14, 20, 21}}
        };

        /**
         * The time between the beginning of a timeslot and the start of frame transmission, considering the default
         * timeslot template of the 2.4Ghz band.
         */
        static constexpr auto DEFAULT_TX_OFFSET = std::chrono::microseconds(2120);

        friend std::ostream &operator<<(std::ostream &out, const SyncParameters &syncParameters) {
            out << "SyncParameters{" << std::endl
                << "CHS: [";
            for (auto it = syncParameters.getCHS().begin(); it != syncParameters.getCHS().end(); ++it) {
                out << *it;
                if (it + 1 != syncParameters.getCHS().end()) {
                    out << ", ";
                }
            }
            out << "]" << std::endl
                << "S: " << syncParameters.getS() << std::endl
                << "Peb: " << syncParameters.getPeb() << std::endl
                << "Psr: {";

            for (auto it = syncParameters.getPsr().begin(); it != syncParameters.getPsr().end();) {
                out << it->first << ":" << it->second;
                if (++it != syncParameters.getPsr().end()) {
                    out << ", ";
                }
            }
            out << "}" << std::endl
                << "Tscan: " << syncParameters.getTScan().count() << "ns" << std::endl
                << "Tswitch: " << syncParameters.getTSwitch().count() << "ns" << std::endl
                << "Teb: " << syncParameters.getTeb().count() << "ns" << std::endl
                << "}";
            return out;
        }

    private:
        std::vector<int> chs_;
        int s_;
        double pEB_;
        std::map<int, double> pSR_;
        std::chrono::nanoseconds tScan_{};
        std::chrono::nanoseconds tSwitch_{};
        std::chrono::nanoseconds tEB_{};

    };

}
#endif //M6SS_SYNCPARAMETERS_H
