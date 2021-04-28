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
#ifndef M6SS_MODELVALIDATION_H
#define M6SS_MODELVALIDATION_H

#include <sqlite3.h>

namespace M6SS {

    /**
     * This class makes an informal validation of the model by comparing it with the results of the simulator.
     * In this direction, we use a huge sample on the simulator side (by default 1000000 samples are taken).
     * The comparison takes places for a large number of random cases (by default 100000) for each of the flows of the
     * model. Each random case is a random selection of the synchronization parameters (see SyncParameters class), except
     * the channel switching delay, which is assumed to be negligible.
     * Furthermore, during the validation of the model, each random case is compared to the same case but with the
     * scan period found optimal through our analysis (i.e., C slotframes, where C the number of available channels).
     */
    class ModelValidator {
    public:

        /**
         * Make the aforementioned comparisons and returns:
         * -1 if the model is not valid,
         * 0 if the model is valid but the scan period that we found optimal through our analysis is
         * not actually optimal,
         * 1 if both the model and the scan period found optimal through our analysis are valid.
         * We note that the model is considered valid if the difference between the model and the simulator is
         * negligible (by default lower than 1%).
         * Detailed information about the comparisons that took place between the model and the simulator are stored in
         * an SQLite database named modelvalidation.db.
         * @param numThreads the number of threads to use for the computations.
         * @return  -1 if the model is not considered valid, or, 0 if the model is considered valid but the scan period
         * that we found optimal through our analysis is not actually optimal, otherwise  (i.e., if both the model and
         * the scan period that we found optimal through our analysis are valid) returns 1.
         * @throw std::invalid_argument if numThreads is less than 1.
         */
        static int makeValidation(int numThreads = 1);

    private:

        /********************************* Sqlite3-Related Functions and Variables ****************/

        static void prepareDBSession();

        static void save(const SyncParameters &syncParameters, double relativeErrorInAVG, double maxAbsoluteErrorInCDF);

        static void closeDBSession();

        static inline sqlite3 *db = nullptr;
        static inline sqlite3_stmt *stmt = nullptr;
        static inline long insertCounter = 0;
        static constexpr long NUM_INSERTIONS_TO_CACHE = 100;
        /*****************************************************************************************/

        /* the number of random cases to check for each of the flows of the model */
        static constexpr long NUM_RANDOM_CASES = 100000;

        /* the number of the simulation samples to use in each case */
        static constexpr long NUM_SIM_SAMPLES_PER_CASE = 1000000;

        /* MAX_ALLOWED_ERROR indicates the max allowed difference between the model and the simulator, in percent.
         * It is noted that, in the case of average synchronization time the relative error is taken into account, while
         * in the case of cdf the absolute error. */
        static constexpr double MAX_ALLOWED_ERROR = 0.01; // 1% MAX_ALLOWED_ERROR

        /*
         * This is a custom real distribution that we use to create random values for n in the case where n is a real
         * number greater than 1 and is not an integer (i.e., the scan period is greater than the step, but is not an
         * integer multiple of the step.
         * We do not use the standard uniform real distribution because it always produces real values with many decimal
         * digits, which result in an almost zero probability that during the scan process a scan period will not finish
         * in a switch step.
         */
        template<class Realtype = double>
        class custom_real_n_distribution {
        public:

            custom_real_n_distribution(Realtype a, Realtype b);

            template<class Generator>
            double operator()(Generator &g);

        private:
            Realtype a_, b_;
        };

    };
}
#endif //M6SS_MODELVALIDATION_H
