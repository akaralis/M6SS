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
#ifndef M6SS_SIMULATOR_H
#define M6SS_SIMULATOR_H

#include <vector>
#include <map>
#include <chrono>
#include "syncparameters.h"

namespace M6SS {

    /**
     * This class implements a simulator for the synchronization procedure that takes places when a node try to join a
     * network that uses the 6TiSCH minimal configuration.
     */
    class Simulator {
    public:
        class Results; // forward declaration

        /**
         * Executes the synchronization procedure numRuns times and returns a reference to the given object 'results',
         * where the results of the simulation are stored. It is noted that the function is thread-safe.
         * @param syncParams the synchronization parameters.
         * @param numRuns the number of times to repeat the synchronization procedure; the number of samples to collect.
         * @param results an object of type 'Results' (see below) where the results will be stored.
         * @return a reference to the Results object
         * @throw std::invalid_argument if numRuns is not greater than zero.
         */
        static Results& run(const SyncParameters &syncParams, long numRuns, Results& results);

        class Results {
            friend class Simulator;
        public:
            /**
             * Returns the average synchronization time captured during the simulations. In the special case where
             * numRuns (see the function 'run') is equal to 1, then the returned value is the synchronization time of a
             * single random synchronization attempt.
             */
            std::chrono::duration<double> avgSyncTime();

            /**
             * This function represents the cumulative distribution function (cdf) of the random variable X that
             * represents the number of (time) steps for the initial synchronization. Each step has the length of a
             * slotframe. The calculation of cdf is made based on the samples that were collected during the simulations.
             * @param steps the number of steps for which the cumulative probability will be calculated.
             * @return P(X ≤ steps)
             * @throw std::invalid_argument if steps is not greater than zero.
             */
             double cdf(size_t steps);

        private:
            std::chrono::duration<double> avgSyncTime_;
            std::vector<double> cdf_;
        };
    };

}
#endif //M6SS_SIMULATOR_H
