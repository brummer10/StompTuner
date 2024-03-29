/*
 * StompTuner audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier:  GPL-2.0 license 
 *
 * Copyright (C) 2023 brummer <brummer@web.de>
 */

#include "resampler.cc"
#include "resampler-table.cc"
#include "gx_resampler.cc"

#include "PluginStompTuner.hpp"
#include "tuner.cc"
#include "low_high_cut.cc"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PluginStompTuner::PluginStompTuner()
    : Plugin(paramCount, 0, 0),
      srChanged(false),
      needs_ramp_down(false),
      needs_ramp_up(false),
      bypassed(false),
      bypass_(2)
{
    lhcut = new low_high_cut::Dsp();
    dsp = new tuner([this] () {this->setFreq();});
    for (unsigned p = 0; p < paramCount; ++p) {
        Parameter param;
        initParameter(p, param);
        setParameterValue(p, param.ranges.def);
    }
    lhcut->init_static(getSampleRate(), lhcut);
    dsp->init(getSampleRate());
}

PluginStompTuner::~PluginStompTuner() {
    //dsp->activate(false);
    if (dsp) delete dsp;
    if (lhcut) delete lhcut;
}

// -----------------------------------------------------------------------
// Init

void PluginStompTuner::initParameter(uint32_t index, Parameter& parameter) {
    if (index >= paramCount)
        return;

    switch (index) {
        case dpf_bypass:
            parameter.name = "Bypass";
            parameter.shortName = "Bypass";
            parameter.symbol = "dpf_bypass";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.0f;
            parameter.designation = kParameterDesignationBypass;
            parameter.hints = kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger;
            break;
        case FREQ:
            parameter.name = "Frequency";
            parameter.shortName = "Freq";
            parameter.symbol = "FREQ";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1000.0f;
            parameter.hints = kParameterIsAutomatable|kParameterIsOutput;
            break;
        case REFFREQ:
            parameter.name = "Reference Frequency";
            parameter.shortName = "RefFreq";
            parameter.symbol = "REFFREQ";
            parameter.ranges.min = 432.0f;
            parameter.ranges.max = 452.0f;
            parameter.ranges.def = 440.0f;
            parameter.hints = kParameterIsAutomatable;
            break;
    }
}

// -----------------------------------------------------------------------
// Internal data

void PluginStompTuner::setFreq() {
    setOutputParameterValue(FREQ, dsp->get_freq());
    //fprintf(stderr, "Freq %f\n", fParams[FREQ]);
}

/**
  Optional callback to inform the plugin about a sample rate change.
*/
void PluginStompTuner::sampleRateChanged(double newSampleRate) {
    fSampleRate = newSampleRate;
    srChanged = true;
    lhcut->init_static(fSampleRate, lhcut);
    dsp->activate(false);
    dsp->init(fSampleRate);
    srChanged = false;
}

/**
  Get the current value of a parameter.
*/
float PluginStompTuner::getParameterValue(uint32_t index) const {
    //fprintf(stderr, "getParameterValue %i %f\n", index,fParams[index]);
    return fParams[index];
}

/**
  Change a parameter value.
*/
void PluginStompTuner::setParameterValue(uint32_t index, float value) {
    fParams[index] = value;
    //fprintf(stderr, "setParameterValue %i %f\n", index,value);
    //dsp->connect(index, value);
}

void PluginStompTuner::setOutputParameterValue(uint32_t index, float value)
{
    fParams[index] = value;
    //fprintf(stderr, "setOutputParameterValue %i %f\n", index,value);
}

// -----------------------------------------------------------------------
// Process

void PluginStompTuner::activate() {
    // plugin is activated
    fSampleRate = getSampleRate();
    ramp_down_step = 32 * (256 * fSampleRate) / 48000; 
    ramp_up_step = ramp_down_step;
    ramp_down = ramp_down_step;
    ramp_up = 0.0;
}

void PluginStompTuner::run(const float** inputs, float** outputs,
                              uint32_t frames) {

    if (srChanged) return;
    // get the left and right audio inputs
    const float* const inpL = inputs[0];
   // const float* const inpR = inputs[1];

    // get the left and right audio outputs
    float* const outL = outputs[0];
   // float* const outR = outputs[1];

    // do inplace processing on default
    if(outL != inpL)
        memcpy(outL, inpL, frames*sizeof(float));

    float buf0[frames];
    float buf[frames];
    memcpy(buf, inpL, frames*sizeof(float));
    // check if bypass is pressed
    if (bypass_ != static_cast<uint32_t>(fParams[dpf_bypass])) {
        bypass_ = static_cast<uint32_t>(fParams[dpf_bypass]);
        if (bypass_) {
            needs_ramp_down = true;
            needs_ramp_up = false;
        } else {
            needs_ramp_down = false;
            needs_ramp_up = true;
            bypassed = false;
        }
    }

    if (needs_ramp_down || needs_ramp_up) {
         memcpy(buf0, inpL, frames*sizeof(float));
    }
    if (!bypassed) {
        lhcut->compute_static(frames, buf, buf, lhcut);
        dsp->feed_tuner(frames, buf);
    }
    // check if ramping is needed
    if (needs_ramp_down) {
        float fade = 0;
        for (uint32_t i=0; i<frames; i++) {
            if (ramp_down >= 0.0) {
                --ramp_down; 
            }
            fade = MAX(0.0,ramp_down) /ramp_down_step ;
            outL[i] = outL[i] * fade + buf0[i] * (1.0 - fade);
        }
        if (ramp_down <= 0.0) {
            // when ramped down, clear buffer from dsp
            needs_ramp_down = false;
            bypassed = true;
            setOutputParameterValue(FREQ, 0.0);
            ramp_down = ramp_down_step;
            ramp_up = 0.0;
        } else {
            ramp_up = ramp_down;
        }
    } else if (needs_ramp_up) {
        float fade = 0;
        for (uint32_t i=0; i<frames; i++) {
            if (ramp_up < ramp_up_step) {
                ++ramp_up ;
            }
            fade = MIN(ramp_up_step,ramp_up) /ramp_up_step ;
            outL[i] = outL[i] * fade + buf0[i] * (1.0 - fade);
        }
        if (ramp_up >= ramp_up_step) {
            needs_ramp_up = false;
            ramp_up = 0.0;
            ramp_down = ramp_down_step;
        } else {
            ramp_down = ramp_up;
        }
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin() {
    return new PluginStompTuner();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
