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
#ifndef M6SS_MODEL_H
#define M6SS_MODEL_H

#include <chrono>
#include "syncparameters.h"

namespace M6SS {

    class Model {
    public:
        class Results; // forward declaration

        /**
         * This function calculate the average synchronization time, as well as the cumulative distribution function (cdf)
         * of the random variable X that represents the number of (time) steps for the initial synchronization in the
         * minimal 6TiSCH configuration. Note that, the model assumes that the channel switch delay is negligible and
         * does not take it into account, even if a non-zero channel switch delay has been defined in syncParams.
         * @param syncParams the synchronization procedure parameters for which the calculation will be made.
         * @param results an object of type 'Results' (see below), which contains the results of the calculation (i.e.,
         * the average synchronization type and the cdf)
         * @return a reference to the Results object
         */
        static Results& calculate(const SyncParameters &syncParams, Results &results);

        class Results {
            friend class Model;

        public:
            /**
             * Returns the average synchronization time.
             */
            std::chrono::duration<double> avgSyncTime();

            /**
             * This function represents the cumulative distribution function (cdf) of the random variable X that
             * represents the number of (time) steps for the initial synchronization.
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

#endif //M6SS_MODEL_H
