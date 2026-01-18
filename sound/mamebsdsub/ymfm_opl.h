// BSD 3-Clause License
//
// Copyright (c) 2021, Aaron Giles
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// modified by SimK for np2 VC++2010

#ifndef YMFM_OPL_H
#define YMFM_OPL_H

#pragma once

#include "ymfm.h"
#include "ymfm_fm.h"

namespace ymfm
{



//*********************************************************
//  OPL3 IMPLEMENTATION CLASSES
//*********************************************************

// ======================> ymf262

class ymf262
{
public:
#define fm_engine fm_engine_base
#define output_data ymfm_output

	// constructor
	ymf262(ymfm_interface &intf);

	// reset
	void reset();

	// save/restore
	void save_restore(ymfm_saved_state &state);

	// pass-through helpers
	uint32_t sample_rate(uint32_t input_clock)       { return m_fm.sample_rate(input_clock); }
	void invalidate_caches() { m_fm.invalidate_caches(); }

	// read access
	uint8_t read_status();
	uint8_t read(uint32_t offset);

	// write access
	void write_address(uint8_t data);
	void write_data(uint8_t data);
	void write_address_hi(uint8_t data);
	void write(uint32_t offset, uint8_t data);

	// generate samples of sound
	void generate(output_data *output, uint32_t numsamples = 1, int32_t* lpHasdata = nullptr);

protected:
	// internal state
	uint16_t m_address;              // address register
	fm_engine m_fm;                  // core FM engine
};

}

#endif // YMFM_OPL_H
