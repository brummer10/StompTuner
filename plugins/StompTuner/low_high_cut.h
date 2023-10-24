

#pragma once

#ifndef LOW_HIGH_CUT_H
#define LOW_HIGH_CUT_H

namespace low_high_cut {

class Dsp {
private:
	uint32_t fSampleRate;
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	double fConst4;
	double fConst5;
	double fConst6;
	int iVec0[2];
	double fRec4[2];
	double fVec1[2];
	double fConst7;
	double fRec3[2];
	double fRec2[2];
	double fConst8;
	double fConst9;
	double fRec1[3];
	double fConst10;
	double fRec0[3];

	void clear_state_f();
	void init(uint32_t sample_rate);
	void compute(int count, float *input0, float *output0);

public:
	Dsp();
	~Dsp();
	static void clear_state_f_static(Dsp*);
	static void init_static(uint32_t sample_rate, Dsp*);
	static void compute_static(int count, float *input0, float *output0, Dsp*);
	static void del_instance(Dsp *p);
};

}

#endif

