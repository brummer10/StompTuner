/*
 * Copyright (C) 2009, 2010 Hermann Meyer, James Warden, Andreas Degert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

#include "pitch_tracker.h"

/****************************************************************
 ** class tuner
 */

tuner::tuner(std::function<void ()>setFreq_)
    : // trackable(),
      pitch_tracker(setFreq_) {}

void tuner::init(unsigned int samplingFreq) {
    int priority = 0, policy = 0;
#ifdef _POSIX_PRIORITY_SCHEDULING
    int priomax = sched_get_priority_max(SCHED_FIFO);
    if ((priomax/2.2) > 0) {
        priority = priomax/2.2;
        policy = SCHED_FIFO;
    }
#endif
    pitch_tracker.init(policy, priority, samplingFreq);
}

int tuner::activate(bool start) {
    if (!start) {
        pitch_tracker.reset();
    }
    return 0;
}

void tuner::feed_tuner(int count, float* input) {
    pitch_tracker.add(count, input);
}

void tuner::del_instance(tuner *self)
{
    delete self;
}
