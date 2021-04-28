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
#ifndef M6SS_TIMEINTERVAL_H
#define M6SS_TIMEINTERVAL_H

#include <chrono>
#include <optional>

namespace M6SS {
    /**
     * This class represents a time interval using nanoseconds precision.
     */
    class TimeInterval {
    public:
        /**
         * Creates an empty interval.
         */
        TimeInterval();

        /**
         * Creates a non-empty interval.
         * @param start the start of the interval
         * @param end the end of the interval
         * @throw std::invalid_argument if the parameter 'start' is greater than the parameter 'end'.
         */
        TimeInterval(std::chrono::nanoseconds start, std::chrono::nanoseconds end);

        [[nodiscard]] bool isEmpty() const;

        [[nodiscard]] std::chrono::nanoseconds length() const;

        [[nodiscard]] bool isSubsetOf(TimeInterval other) const;

        static TimeInterval intersection(TimeInterval interval1, TimeInterval interval2);

        [[nodiscard]] const std::optional<std::chrono::nanoseconds> &getStart() const;

        [[nodiscard]] const std::optional<std::chrono::nanoseconds> &getEnd() const;

        void set(std::chrono::nanoseconds start, std::chrono::nanoseconds end);

    private:
        std::optional<std::chrono::nanoseconds> start_;
        std::optional<std::chrono::nanoseconds> end_;

    };
}

#endif //M6SS_TIMEINTERVAL_H
