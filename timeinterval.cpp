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
#include "timeinterval.h"
#include <stdexcept>
#include <algorithm>

using namespace std::chrono_literals;

M6SS::TimeInterval::TimeInterval() = default;


M6SS::TimeInterval::TimeInterval(std::chrono::nanoseconds start, std::chrono::nanoseconds end) {
    if (start > end) {
        throw std::invalid_argument("The parameter 'start' must be greater than the parameter 'end'.");
    }

    set(start, end);
}

bool M6SS::TimeInterval::isEmpty() const {
    return !start_.has_value();
}


std::chrono::nanoseconds M6SS::TimeInterval::length() const {
    return isEmpty() ? 0ns : end_.value() - start_.value();
}

bool M6SS::TimeInterval::isSubsetOf(M6SS::TimeInterval other) const {
    if (isEmpty() || other.isEmpty()) {
        return false;
    }
    return other.getStart().value() <= start_.value() && other.getEnd().value() >= end_.value();
}


M6SS::TimeInterval
M6SS::TimeInterval::intersection(M6SS::TimeInterval interval1,
                                 M6SS::TimeInterval interval2) {
    if (interval1.isEmpty() || interval2.isEmpty()
        || interval1.getStart().value() > interval2.getEnd().value()
        || interval1.getEnd().value() < interval2.getStart().value()) {
        return TimeInterval(); // return empty interval
    }

    return TimeInterval(
            std::max(interval1.getStart().value(), interval2.getStart().value()),
            std::min(interval1.getEnd().value(), interval2.getEnd().value())
    );
}

const std::optional<std::chrono::nanoseconds> &M6SS::TimeInterval::getStart() const {
    return start_;
}

const std::optional<std::chrono::nanoseconds> &M6SS::TimeInterval::getEnd() const {
    return end_;
}

void M6SS::TimeInterval::set(std::chrono::nanoseconds start, std::chrono::nanoseconds end) {
    this->start_ = start;
    this->end_ = end;
}



