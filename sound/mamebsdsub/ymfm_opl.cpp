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

#include "ymfm_opl.h"
#include "ymfm_fm.ipp"

namespace ymfm
{

//-------------------------------------------------
//  opl_key_scale_atten - converts an
//  OPL concatenated block (3 bits) and fnum
//  (10 bits) into an attenuation offset; values
//  here are for 6dB/octave, in 0.75dB units
//  (matching total level LSB)
//-------------------------------------------------

inline uint32_t opl_key_scale_atten(uint32_t block, uint32_t fnum_4msb)
{
	// this table uses the top 4 bits of FNUM and are the maximal values
	// (for when block == 7). Values for other blocks can be computed by
	// subtracting 8 for each block below 7.
	static uint8_t const fnum_to_atten[16] = { 0,24,32,37,40,43,45,47,48,50,51,52,53,54,55,56 };
	int32_t result = fnum_to_atten[fnum_4msb] - 8 * (block ^ 7);
	return std::max<int32_t>(0, result);
}


//*********************************************************
//  OPL REGISTERS
//*********************************************************

//-------------------------------------------------
//  opl3_registers - constructor
//-------------------------------------------------

opl3_registers::opl3_registers() :
	m_lfo_am_counter(0),
	m_lfo_pm_counter(0),
	m_noise_lfsr(1),
	m_lfo_am(0)
{
	// create these pointers to appease overzealous compilers checking array
	// bounds in unreachable code (looking at you, clang)
	uint16_t *wf0 = &m_waveform[0][0];
	uint16_t *wf1 = &m_waveform[1 % WAVEFORMS][0];
	uint16_t *wf2 = &m_waveform[2 % WAVEFORMS][0];
	uint16_t *wf3 = &m_waveform[3 % WAVEFORMS][0];
	uint16_t *wf4 = &m_waveform[4 % WAVEFORMS][0];
	uint16_t *wf5 = &m_waveform[5 % WAVEFORMS][0];
	uint16_t *wf6 = &m_waveform[6 % WAVEFORMS][0];
	uint16_t *wf7 = &m_waveform[7 % WAVEFORMS][0];

	// create the waveforms
	for (uint32_t index = 0; index < WAVEFORM_LENGTH; index++)
		wf0[index] = abs_sin_attenuation(index) | (bitfield(index, 9) << 15);

	if (WAVEFORMS >= 4)
	{
		uint16_t zeroval = wf0[0];
		for (uint32_t index = 0; index < WAVEFORM_LENGTH; index++)
		{
			wf1[index] = bitfield(index, 9) ? zeroval : wf0[index];
			wf2[index] = wf0[index] & 0x7fff;
			wf3[index] = bitfield(index, 8) ? zeroval : (wf0[index] & 0x7fff);
			if (WAVEFORMS >= 8)
			{
				wf4[index] = bitfield(index, 9) ? zeroval : wf0[index * 2];
				wf5[index] = bitfield(index, 9) ? zeroval : wf0[(index * 2) & 0x1ff];
				wf6[index] = bitfield(index, 9) << 15;
				wf7[index] = (bitfield(index, 9) ? (index ^ 0x13ff) : index) << 3;
			}
		}
	}

	// OPL3/OPL4 have dynamic operators, so initialize the fourop_enable value here
	// since operator_map() is called right away, prior to reset()
	m_regdata[0x104 % REGISTERS] = 0;
}


//-------------------------------------------------
//  reset - reset to initial state
//-------------------------------------------------

void opl3_registers::reset()
{
	std::fill_n(&m_regdata[0], REGISTERS, 0);
}


//-------------------------------------------------
//  save_restore - save or restore the data
//-------------------------------------------------


void opl3_registers::save_restore(ymfm_saved_state &state)
{
	state.save_restore(m_lfo_am_counter);
	state.save_restore(m_lfo_pm_counter);
	state.save_restore(m_lfo_am);
	state.save_restore(m_noise_lfsr);
	state.save_restore(m_regdata);
}


//-------------------------------------------------
//  operator_map - return an array of operator
//  indices for each channel; for OPL this is fixed
//-------------------------------------------------


void opl3_registers::operator_map(operator_mapping &dest)
{
	// OPL3/OPL4 can be configured for 2 or 4 operators
	uint32_t fourop = fourop_enable();

	dest.chan[ 0] = bitfield(fourop, 0) ? operator_list(  0,  3,  6,  9 ) : operator_list(  0,  3 );
	dest.chan[ 1] = bitfield(fourop, 1) ? operator_list(  1,  4,  7, 10 ) : operator_list(  1,  4 );
	dest.chan[ 2] = bitfield(fourop, 2) ? operator_list(  2,  5,  8, 11 ) : operator_list(  2,  5 );
	dest.chan[ 3] = bitfield(fourop, 0) ? operator_list() : operator_list(  6,  9 );
	dest.chan[ 4] = bitfield(fourop, 1) ? operator_list() : operator_list(  7, 10 );
	dest.chan[ 5] = bitfield(fourop, 2) ? operator_list() : operator_list(  8, 11 );
	dest.chan[ 6] = operator_list( 12, 15 );
	dest.chan[ 7] = operator_list( 13, 16 );
	dest.chan[ 8] = operator_list( 14, 17 );

	dest.chan[ 9] = bitfield(fourop, 3) ? operator_list( 18, 21, 24, 27 ) : operator_list( 18, 21 );
	dest.chan[10] = bitfield(fourop, 4) ? operator_list( 19, 22, 25, 28 ) : operator_list( 19, 22 );
	dest.chan[11] = bitfield(fourop, 5) ? operator_list( 20, 23, 26, 29 ) : operator_list( 20, 23 );
	dest.chan[12] = bitfield(fourop, 3) ? operator_list() : operator_list( 24, 27 );
	dest.chan[13] = bitfield(fourop, 4) ? operator_list() : operator_list( 25, 28 );
	dest.chan[14] = bitfield(fourop, 5) ? operator_list() : operator_list( 26, 29 );
	dest.chan[15] = operator_list( 30, 33 );
	dest.chan[16] = operator_list( 31, 34 );
	dest.chan[17] = operator_list( 32, 35 );
}


//-------------------------------------------------
//  write - handle writes to the register array
//-------------------------------------------------


bool opl3_registers::write(uint16_t index, uint8_t data, uint32_t &channel, uint32_t &opmask)
{
	assert(index < REGISTERS);

	// writes to the mode register with high bit set ignore the low bits
	if (index == REG_MODE && bitfield(data, 7) != 0)
		m_regdata[index] |= 0x80;
	else
		m_regdata[index] = data;

	// handle writes to the rhythm keyons
	if (index == 0xbd)
	{
		channel = RHYTHM_CHANNEL;
		opmask = bitfield(data, 5) ? bitfield(data, 0, 5) : 0;
		return true;
	}

	// handle writes to the channel keyons
	if ((index & 0xf0) == 0xb0)
	{
		channel = index & 0x0f;
		if (channel < 9)
		{
			if (IsOpl3Plus)
				channel += 9 * bitfield(index, 8);
			opmask = bitfield(data, 5) ? 15 : 0;
			return true;
		}
	}
	return false;
}


//-------------------------------------------------
//  clock_noise_and_lfo - clock the noise and LFO,
//  handling clock division, depth, and waveform
//  computations
//-------------------------------------------------

static int32_t opl_clock_noise_and_lfo(uint32_t &noise_lfsr, uint16_t &lfo_am_counter, uint16_t &lfo_pm_counter, uint8_t &lfo_am, uint32_t am_depth, uint32_t pm_depth)
{
	// OPL has a 23-bit noise generator for the rhythm section, running at
	// a constant rate, used only for percussion input
	noise_lfsr <<= 1;
	noise_lfsr |= bitfield(noise_lfsr, 23) ^ bitfield(noise_lfsr, 9) ^ bitfield(noise_lfsr, 8) ^ bitfield(noise_lfsr, 1);

	// OPL has two fixed-frequency LFOs, one for AM, one for PM

	// the AM LFO has 210*64 steps; at a nominal 50kHz output,
	// this equates to a period of 50000/(210*64) = 3.72Hz
	uint32_t am_counter = lfo_am_counter++;
	if (am_counter >= 210*64 - 1)
		lfo_am_counter = 0;

	// low 8 bits are fractional; depth 0 is divided by 2, while depth 1 is times 2
	int shift = 9 - 2 * am_depth;

	// AM value is the upper bits of the value, inverted across the midpoint
	// to produce a triangle
	lfo_am = ((am_counter < 105*64) ? am_counter : (210*64+63 - am_counter)) >> shift;

	// the PM LFO has 8192 steps, or a nominal period of 6.1Hz
	uint32_t pm_counter = lfo_pm_counter++;

	// PM LFO is broken into 8 chunks, each lasting 1024 steps; the PM value
	// depends on the upper bits of FNUM, so this value is a fraction and
	// sign to apply to that value, as a 1.3 value
	static int8_t const pm_scale[8] = { 8, 4, 0, -4, -8, -4, 0, 4 };
	return pm_scale[bitfield(pm_counter, 10, 3)] >> (pm_depth ^ 1);
}


int32_t opl3_registers::clock_noise_and_lfo()
{
	return opl_clock_noise_and_lfo(m_noise_lfsr, m_lfo_am_counter, m_lfo_pm_counter, m_lfo_am, lfo_am_depth(), lfo_pm_depth());
}


//-------------------------------------------------
//  cache_operator_data - fill the operator cache
//  with prefetched data; note that this code is
//  also used by ymopna_registers, so it must
//  handle upper channels cleanly
//-------------------------------------------------


void opl3_registers::cache_operator_data(uint32_t choffs, uint32_t opoffs, opdata_cache &cache)
{
	// set up the easy stuff
	cache.waveform = &m_waveform[op_waveform(opoffs) % WAVEFORMS][0];

	// get frequency from the channel
	uint32_t block_freq = cache.block_freq = ch_block_freq(choffs);

	// compute the keycode: block_freq is:
	//
	//     111  |
	//     21098|76543210
	//     BBBFF|FFFFFFFF
	//     ^^^??
	//
	// the 4-bit keycode uses the top 3 bits plus one of the next two bits
	uint32_t keycode = bitfield(block_freq, 10, 3) << 1;

	// lowest bit is determined by note_select(); note that it is
	// actually reversed from what the manual says, however
	keycode |= bitfield(block_freq, 9 - note_select(), 1);

	// no detune adjustment on OPL
	cache.detune = 0;

	// multiple value, as an x.1 value (0 means 0.5)
	// replace the low bit with a table lookup to give 0,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15
	uint32_t multiple = op_multiple(opoffs);
	cache.multiple = ((multiple & 0xe) | bitfield(0xc2aa, multiple)) * 2;
	if (cache.multiple == 0)
		cache.multiple = 1;

	// phase step, or PHASE_STEP_DYNAMIC if PM is active; this depends on block_freq, detune,
	// and multiple, so compute it after we've done those
	if (op_lfo_pm_enable(opoffs) == 0)
		cache.phase_step = compute_phase_step(choffs, opoffs, cache, 0);
	else
		cache.phase_step = PHASE_STEP_DYNAMIC;

	// total level, scaled by 8
	cache.total_level = op_total_level(opoffs) << 3;

	// pre-add key scale level
	uint32_t ksl = op_ksl(opoffs);
	if (ksl != 0)
		cache.total_level += opl_key_scale_atten(bitfield(block_freq, 10, 3), bitfield(block_freq, 6, 4)) << ksl;

	// 4-bit sustain level, but 15 means 31 so effectively 5 bits
	cache.eg_sustain = op_sustain_level(opoffs);
	cache.eg_sustain |= (cache.eg_sustain + 1) & 0x10;
	cache.eg_sustain <<= 5;

	// determine KSR adjustment for enevlope rates
	uint32_t ksrval = keycode >> (2 * (op_ksr(opoffs) ^ 1));
	cache.eg_rate[EG_ATTACK] = effective_rate(op_attack_rate(opoffs) * 4, ksrval);
	cache.eg_rate[EG_DECAY] = effective_rate(op_decay_rate(opoffs) * 4, ksrval);
	cache.eg_rate[EG_SUSTAIN] = op_eg_sustain(opoffs) ? 0 : effective_rate(op_release_rate(opoffs) * 4, ksrval);
	cache.eg_rate[EG_RELEASE] = effective_rate(op_release_rate(opoffs) * 4, ksrval);
	cache.eg_rate[EG_DEPRESS] = 0x3f;
}


//-------------------------------------------------
//  compute_phase_step - compute the phase step
//-------------------------------------------------

static uint32_t opl_compute_phase_step(uint32_t block_freq, uint32_t multiple, int32_t lfo_raw_pm)
{
	// OPL phase calculation has no detuning, but uses FNUMs like
	// the OPN version, and computes PM a bit differently

	// extract frequency number as a 12-bit fraction
	uint32_t fnum = bitfield(block_freq, 0, 10) << 2;

	// apply the phase adjustment based on the upper 3 bits
	// of FNUM and the PM depth parameters
	fnum += (lfo_raw_pm * bitfield(block_freq, 7, 3)) >> 1;

	// keep fnum to 12 bits
	fnum &= 0xfff;

	// apply block shift to compute phase step
	uint32_t block = bitfield(block_freq, 10, 3);
	uint32_t phase_step = (fnum << block) >> 2;

	// apply frequency multiplier (which is cached as an x.1 value)
	return (phase_step * multiple) >> 1;
}


uint32_t opl3_registers::compute_phase_step(uint32_t choffs, uint32_t opoffs, opdata_cache &cache, int32_t lfo_raw_pm)
{
	return opl_compute_phase_step(cache.block_freq, cache.multiple, op_lfo_pm_enable(opoffs) ? lfo_raw_pm : 0);
}


//-------------------------------------------------
//  log_keyon - log a key-on event
//-------------------------------------------------


std::string opl3_registers::log_keyon(uint32_t choffs, uint32_t opoffs)
{
	uint32_t chnum = (choffs & 15) + 9 * bitfield(choffs, 8);
	uint32_t opnum = (opoffs & 31) - 2 * ((opoffs & 31) / 8) + 18 * bitfield(opoffs, 8);

	char buffer[256];
	int end = 0;

	end += snprintf(&buffer[end], sizeof(buffer) - end, "%2u.%02u freq=%04X fb=%u alg=%X mul=%X tl=%02X ksr=%u ns=%u ksl=%u adr=%X/%X/%X sl=%X sus=%u",
		chnum, opnum,
		ch_block_freq(choffs),
		ch_feedback(choffs),
		ch_algorithm(choffs),
		op_multiple(opoffs),
		op_total_level(opoffs),
		op_ksr(opoffs),
		note_select(),
		op_ksl(opoffs),
		op_attack_rate(opoffs),
		op_decay_rate(opoffs),
		op_release_rate(opoffs),
		op_sustain_level(opoffs),
		op_eg_sustain(opoffs));

	if (OUTPUTS > 1)
		end += snprintf(&buffer[end], sizeof(buffer) - end, " out=%c%c%c%c",
			ch_output_0(choffs) ? 'L' : '-',
			ch_output_1(choffs) ? 'R' : '-',
			ch_output_2(choffs) ? '0' : '-',
			ch_output_3(choffs) ? '1' : '-');
	if (op_lfo_am_enable(opoffs) != 0)
		end += snprintf(&buffer[end], sizeof(buffer) - end, " am=%u", lfo_am_depth());
	if (op_lfo_pm_enable(opoffs) != 0)
		end += snprintf(&buffer[end], sizeof(buffer) - end, " pm=%u", lfo_pm_depth());
	if (waveform_enable() && op_waveform(opoffs) != 0)
		end += snprintf(&buffer[end], sizeof(buffer) - end, " wf=%u", op_waveform(opoffs));
	if (is_rhythm(choffs))
		end += snprintf(&buffer[end], sizeof(buffer) - end, " rhy=1");
	if (DYNAMIC_OPS)
	{
		operator_mapping map;
		operator_map(map);
		if (bitfield(map.chan[chnum], 16, 8) != 0xff)
			end += snprintf(&buffer[end], sizeof(buffer) - end, " 4op");
	}

	return buffer;
}


//*********************************************************
//  YMF262
//*********************************************************

//-------------------------------------------------
//  ymf262 - constructor
//-------------------------------------------------

ymf262::ymf262(ymfm_interface &intf) :
	m_address(0),
	m_fm(intf)
{
}


//-------------------------------------------------
//  reset - reset the system
//-------------------------------------------------

void ymf262::reset()
{
	// reset the engines
	m_fm.reset();
}


//-------------------------------------------------
//  save_restore - save or restore the data
//-------------------------------------------------

void ymf262::save_restore(ymfm_saved_state &state)
{
	state.save_restore(m_address);
	m_fm.save_restore(state);
}


//-------------------------------------------------
//  read_status - read the status register
//-------------------------------------------------

uint8_t ymf262::read_status()
{
	return m_fm.status();
}


//-------------------------------------------------
//  read - handle a read from the device
//-------------------------------------------------

uint8_t ymf262::read(uint32_t offset)
{
	uint8_t result = 0xff;
	switch (offset & 3)
	{
		case 0: // status port
			result = read_status();
			break;

		case 1:
		case 2:
		case 3:
			break;
	}
	return result;
}


//-------------------------------------------------
//  write_address - handle a write to the address
//  register
//-------------------------------------------------

void ymf262::write_address(uint8_t data)
{
	// YMF262 doesn't expose a busy signal, but it does indicate that
	// address writes should be no faster than every 32 clocks
	m_fm.intf().ymfm_set_busy_end(32 * m_fm.clock_prescale());

	// just set the address
	m_address = data;
}


//-------------------------------------------------
//  write_data - handle a write to the data
//  register
//-------------------------------------------------

void ymf262::write_data(uint8_t data)
{
	// YMF262 doesn't expose a busy signal, but it does indicate that
	// data writes should be no faster than every 32 clocks
	m_fm.intf().ymfm_set_busy_end(32 * m_fm.clock_prescale());

	// write to FM
	m_fm.write(m_address, data);
}


//-------------------------------------------------
//  write_address_hi - handle a write to the upper
//  address register
//-------------------------------------------------

void ymf262::write_address_hi(uint8_t data)
{
	// YMF262 doesn't expose a busy signal, but it does indicate that
	// address writes should be no faster than every 32 clocks
	m_fm.intf().ymfm_set_busy_end(32 * m_fm.clock_prescale());

	// just set the address
	m_address = data | 0x100;

	// tests reveal that in compatibility mode, upper bit is masked
	// except for register 0x105
	if (m_fm.regs().newflag() == 0 && m_address != 0x105)
		m_address &= 0xff;
}


//-------------------------------------------------
//  write - handle a write to the register
//  interface
//-------------------------------------------------

void ymf262::write(uint32_t offset, uint8_t data)
{
	switch (offset & 3)
	{
		case 0: // address port
			write_address(data);
			break;

		case 1: // data port
			write_data(data);
			break;

		case 2: // address port
			write_address_hi(data);
			break;

		case 3: // data port
			write_data(data);
			break;
	}
}


//-------------------------------------------------
//  generate - generate samples of sound
//-------------------------------------------------

void ymf262::generate(output_data *output, uint32_t numsamples, int32_t* lpHasdata)
{
	int32_t hasdata = 0;
	for (uint32_t samp = 0; samp < numsamples; samp++, output++)
	{
		// clock the system
		m_fm.clock(ALL_CHANNELS);

		// update the FM content; mixing details for YMF262 need verification
		m_fm.output(output->clear(), 0, 32767, ALL_CHANNELS);

		// YMF262 output is 16-bit offset serial via YAC512 DAC
		output->clamp16();

		hasdata |= output->data[0] | output->data[1] | output->data[2] | output->data[3];
	}
	if (lpHasdata) *lpHasdata = hasdata;
}


}
