/*
 * Copyright (C) 2023, 2010 Hermann Meyer
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
 ** Pitch Tracker
 **
 ** some code and ideas taken from K4Guitune (William Spinelli)
 ** changed to NSDF method (some code from tartini / Philip McLeod)
 **
 */


// downsampling factor
static const int DOWNSAMPLE = 2;
static const float SIGNAL_THRESHOLD_ON = 0.001;
static const float SIGNAL_THRESHOLD_OFF = 0.0009;
static const float TRACKER_PERIOD = 0.1;
// The size of the read buffer
static const int FFT_SIZE = 2048;

///////////////////////// INTERNAL WORKER CLASS   //////////////////////

PitchTrackerWorker::PitchTrackerWorker()
    : _execute(false) {
}

PitchTrackerWorker::~PitchTrackerWorker() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void PitchTrackerWorker::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        cv.notify_one();
        _thd.join();
    }
}

void PitchTrackerWorker::start(PitchTracker *pt) {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, pt]() {
        while (_execute.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            pt->busy.store(false, std::memory_order_release);
            // wait for signal from dsp that work is to do
            cv.wait(lk);
            //do work
            if (_execute.load(std::memory_order_acquire)) {
                pt->static_run(pt);
            }
        }
        // when done
    });
}

bool PitchTrackerWorker::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}


/////////////////////////  PitchTracker Class   ////////////////////////


void *PitchTracker::static_run(void *p) {
    (reinterpret_cast<PitchTracker *>(p))->run();
    return NULL;
}

PitchTracker::PitchTracker(std::function<void ()>setFreq_)
    : new_freq(setFreq_),
      error(false),
      tick(0),
      resamp(),
      m_sampleRate(),
      fixed_sampleRate(41000),
      m_freq(-1),
      signal_threshold_on(SIGNAL_THRESHOLD_ON),
      signal_threshold_off(SIGNAL_THRESHOLD_OFF),
      tracker_period(TRACKER_PERIOD),
      m_buffersize(),
      m_fftSize(),
      m_buffer(new float[FFT_SIZE]),
      m_bufferIndex(0),
      m_input(new float[FFT_SIZE]),
      m_audioLevel(false),
      m_fftwPlanFFT(0),
      m_fftwPlanIFFT(0) {
    busy.store(false, std::memory_order_release);
    const int size = FFT_SIZE + (FFT_SIZE+1) / 2;
    m_fftwBufferTime = reinterpret_cast<float*>
                       (fftwf_malloc(size * sizeof(*m_fftwBufferTime)));
    m_fftwBufferFreq = reinterpret_cast<float*>
                       (fftwf_malloc(size * sizeof(*m_fftwBufferFreq)));

    memset(m_buffer, 0, FFT_SIZE * sizeof(*m_buffer));
    memset(m_input, 0, FFT_SIZE * sizeof(*m_input));
    memset(m_fftwBufferTime, 0, size * sizeof(*m_fftwBufferTime));
    memset(m_fftwBufferFreq, 0, size * sizeof(*m_fftwBufferFreq));

    worker.start(this);
    if (!m_buffer || !m_input || !m_fftwBufferTime || !m_fftwBufferFreq) {
        error = true;
    }
}


PitchTracker::~PitchTracker() {
    worker.stop();
    fftwf_destroy_plan(m_fftwPlanFFT);
    fftwf_destroy_plan(m_fftwPlanIFFT);
    fftwf_free(m_fftwBufferTime);
    fftwf_free(m_fftwBufferFreq);
    delete[] m_input;
    delete[] m_buffer;
}

void PitchTracker::set_threshold(float v) {
    signal_threshold_on = v;
    signal_threshold_off = v*0.9;
}

void PitchTracker::set_fast_note_detection(bool v) {
    if (v) {
	signal_threshold_on = SIGNAL_THRESHOLD_ON * 5;
	signal_threshold_off = SIGNAL_THRESHOLD_OFF * 5;
	tracker_period = TRACKER_PERIOD / 10;
    } else {
	signal_threshold_on = SIGNAL_THRESHOLD_ON;
	signal_threshold_off = SIGNAL_THRESHOLD_OFF;
	tracker_period = TRACKER_PERIOD;
    }
}

bool PitchTracker::setParameters(int sampleRate, int buffersize) {
    assert(buffersize <= FFT_SIZE);

    if (error) {
        return false;
    }
    m_sampleRate = fixed_sampleRate / DOWNSAMPLE;
    resamp.setup(sampleRate, m_sampleRate, 1, 16); // 16 == least quality

    if (m_buffersize != buffersize) {
        m_buffersize = buffersize;
        m_fftSize = m_buffersize + (m_buffersize+1) / 2;
        fftwf_destroy_plan(m_fftwPlanFFT);
        fftwf_destroy_plan(m_fftwPlanIFFT);
        m_fftwPlanFFT = fftwf_plan_r2r_1d(
                            m_fftSize, m_fftwBufferTime, m_fftwBufferFreq,
                            FFTW_R2HC, FFTW_ESTIMATE);
        m_fftwPlanIFFT = fftwf_plan_r2r_1d(
                             m_fftSize, m_fftwBufferFreq, m_fftwBufferTime,
                             FFTW_HC2R, FFTW_ESTIMATE);
    }

    if (!m_fftwPlanFFT || !m_fftwPlanIFFT) {
        error = true;
        return false;
    }

    return !error;
}

void PitchTracker::init(unsigned int samplerate) {
    setParameters(samplerate, FFT_SIZE);
}

void PitchTracker::reset() {
    tick = 0;
    m_bufferIndex = 0;
    resamp.reset();
    m_freq = -1;
}

void PitchTracker::add(int count, float* input) {
    if (error) {
        return;
    }
    resamp.inp_count = count;
    resamp.inp_data = input;
    for (;;) {
        resamp.out_data = &m_buffer[m_bufferIndex];
        int n = FFT_SIZE - m_bufferIndex;
        resamp.out_count = n;
        resamp.process();
        n -= resamp.out_count; // n := number of output samples
        if (!n) { // all soaked up by filter
            return;
        }
        m_bufferIndex = (m_bufferIndex + n) % FFT_SIZE;
        if (resamp.inp_count == 0) {
            break;
        }
    }
    if (++tick * count >= m_sampleRate * DOWNSAMPLE * tracker_period) {
        if (busy.load(std::memory_order_acquire)) {
            return;
        }
        busy.store(true, std::memory_order_release);
        tick = 0;
        copy();
        worker.cv.notify_one();
    }
}

void PitchTracker::copy() {
    int start = (FFT_SIZE + m_bufferIndex - m_buffersize) % FFT_SIZE;
    int end = (FFT_SIZE + m_bufferIndex) % FFT_SIZE;
    int cnt = 0;
    if (start >= end) {
        cnt = FFT_SIZE - start;
        memcpy(m_input, &m_buffer[start], cnt * sizeof(*m_input));
        start = 0;
    }
    memcpy(&m_input[cnt], &m_buffer[start], (end - start) * sizeof(*m_input));
}

inline float sq(float x) {
    return x * x;
}

inline void parabolaTurningPoint(float y_1, float y0, float y1, float xOffset, float *x) {
    float yTop = y_1 - y1;
    float yBottom = y1 + y_1 - 2 * y0;
    if (yBottom != 0.0) {
        *x = xOffset + yTop / (2 * yBottom);
    } else {
        *x = xOffset;
    }
}

static int findMaxima(float *input, int len, int *maxPositions, int *length, int maxLen) {
    int pos = 0;
    int curMaxPos = 0;
    int overallMaxIndex = 0;

    while (pos < (len-1)/3 && input[pos] > 0.0) {
        pos += 1;  // find the first negitive zero crossing
    }
    while (pos < len-1 && input[pos] <= 0.0) {
        pos += 1;  // loop over all the values below zero
    }
    if (pos == 0) {
        pos = 1;  // can happen if output[0] is NAN
    }
    while (pos < len-1) {
        if (input[pos] > input[pos-1] && input[pos] >= input[pos+1]) {  // a local maxima
            if (curMaxPos == 0) {
                curMaxPos = pos;  // the first maxima (between zero crossings)
            } else if (input[pos] > input[curMaxPos]) {
                curMaxPos = pos;  // a higher maxima (between the zero crossings)
            }
        }
        pos += 1;
        if (pos < len-1 && input[pos] <= 0.0) {  // a negative zero crossing
            if (curMaxPos > 0) {  // if there was a maximum
                maxPositions[*length] = curMaxPos;  // add it to the vector of maxima
                *length += 1;
                if (overallMaxIndex == 0) {
                    overallMaxIndex = curMaxPos;
                } else if (input[curMaxPos] > input[overallMaxIndex]) {
                    overallMaxIndex = curMaxPos;
                }
                if (*length >= maxLen) {
                    return overallMaxIndex;
                }
                curMaxPos = 0;  // clear the maximum position, so we start looking for a new ones
            }
            while (pos < len-1 && input[pos] <= 0.0) {
                pos += 1;  // loop over all the values below zero
            }
        }
    }
    if (curMaxPos > 0) {  // if there was a maximum in the last part
        maxPositions[*length] = curMaxPos;  // add it to the vector of maxima
        *length += 1;
        if (overallMaxIndex == 0) {
            overallMaxIndex = curMaxPos;
        } else if (input[curMaxPos] > input[overallMaxIndex]) {
            overallMaxIndex = curMaxPos;
        }
        curMaxPos = 0;  // clear the maximum position, so we start looking for a new ones
    }
    return overallMaxIndex;
}

static int findsubMaximum(float *input, int len, float threshold) {
    int indices[10];
    int length = 0;
    int overallMaxIndex = findMaxima(input, len, indices, &length, 10);
    if (length == 0) {
        return -1;
    }
    threshold += (1.0 - threshold) * (1.0 - input[overallMaxIndex]);
    float cutoff = input[overallMaxIndex] * threshold;
    for (int j = 0; j < length; j++) {
        if (input[indices[j]] >= cutoff) {
            return indices[j];
        }
    }
    // should never get here
    return -1;
}

void PitchTracker::run() {
    float sum = 0.0;
    for (int k = 0; k < m_buffersize; ++k) {
        sum += fabs(m_input[k]);
    }
    float threshold = (m_audioLevel ? signal_threshold_off : signal_threshold_on);
    m_audioLevel = (sum / m_buffersize >= threshold);
    if ( m_audioLevel == false ) {
        if (m_freq != 0) {
            m_freq = 0;
            new_freq();
        }
        return;
    }

    memcpy(m_fftwBufferTime, m_input, m_buffersize * sizeof(*m_fftwBufferTime));
    memset(m_fftwBufferTime+m_buffersize, 0, (m_fftSize - m_buffersize) * sizeof(*m_fftwBufferTime));
    fftwf_execute(m_fftwPlanFFT);
    for (int k = 1; k < m_fftSize/2; k++) {
        m_fftwBufferFreq[k] = sq(m_fftwBufferFreq[k]) + sq(m_fftwBufferFreq[m_fftSize-k]);
        m_fftwBufferFreq[m_fftSize-k] = 0.0;
    }
    m_fftwBufferFreq[0] = sq(m_fftwBufferFreq[0]);
    m_fftwBufferFreq[m_fftSize/2] = sq(m_fftwBufferFreq[m_fftSize/2]);

    fftwf_execute(m_fftwPlanIFFT);

    double sumSq = 2.0 * static_cast<double>(m_fftwBufferTime[0]) / static_cast<double>(m_fftSize);
    for (int k = 0; k < m_fftSize - m_buffersize; k++) {
        m_fftwBufferTime[k] = m_fftwBufferTime[k+1] / static_cast<float>(m_fftSize);
    }

    int count = (m_buffersize + 1) / 2;
    for (int k = 0; k < count; k++) {
        sumSq  -= sq(m_input[m_buffersize-1-k]) + sq(m_input[k]);
        // dividing by zero is very slow, so deal with it seperately
        if (sumSq > 0.0) {
            m_fftwBufferTime[k] *= 2.0 / sumSq;
        } else {
            m_fftwBufferTime[k] = 0.0;
        }
    }
    const float thres = 0.99; // was 0.6
        int maxAutocorrIndex = findsubMaximum(m_fftwBufferTime, count, thres);

        float x = 0.0;
        if (maxAutocorrIndex >= 0) {
            parabolaTurningPoint(m_fftwBufferTime[maxAutocorrIndex-1],
                                 m_fftwBufferTime[maxAutocorrIndex],
                                 m_fftwBufferTime[maxAutocorrIndex+1],
                                 maxAutocorrIndex+1, &x);
            x = m_sampleRate / x;
            if (x > 999.0) {  // precision drops above 1000 Hz
                x = 0.0;
            }
        }
    if (m_freq != x) {
        m_freq = x;
        new_freq();
    }
}

float PitchTracker::get_estimated_note() {
    return m_freq <= 0.0 ? 1000.0 : 12 * log2f(2.272727e-03f * m_freq);
}


