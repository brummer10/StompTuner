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
 
#pragma once

#ifndef TUNER_H_
#define TUNER_H_


#include "pitch_tracker.h"

/****************************************************************
 ** class tuner
 */

class tuner {
private:
    PitchTracker pitch_tracker;
public:
    // sigc::signal<void >& signal_freq_changed() { return pitch_tracker.new_freq; }
   // Glib::Dispatcher& signal_freq_changed() { return pitch_tracker.new_freq; }
    void feed_tuner(int count, float *input);
    int activate(bool start);
    void init(unsigned int samplingFreq);
    static void del_instance(tuner *self);
    float get_freq() { return pitch_tracker.get_estimated_freq(); }
    float get_note() { return pitch_tracker.get_estimated_note(); }
    static inline float db2power(float db) {return pow(10.,db*0.05);}
    static void set_threshold_level(tuner& self,float v) {self.pitch_tracker.set_threshold(db2power(v)); }
    static void set_fast_note(tuner& self,bool v) {self.pitch_tracker.set_fast_note_detection(v); }
    tuner(std::function<void ()>setFreq_);
    ~tuner() {};
};

#endif
