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

#ifndef YMFM_FM_H
#define YMFM_FM_H

#pragma once

#define YMFM_DEBUG_LOG_WAVFILES (0)

namespace ymfm
{

//*********************************************************
//  GLOBAL ENUMERATORS
//*********************************************************

// three different keyon sources; actual keyon is an OR over all of these
enum keyon_type : uint32_t
{
	KEYON_NORMAL = 0,
	KEYON_RHYTHM = 1,
	KEYON_CSM = 2
};



//*********************************************************
//  CORE IMPLEMENTATION
//*********************************************************

// ======================> opdata_cache

// this class holds data that is computed once at the start of clocking
// and remains static during subsequent sound generation
struct opdata_cache
{
	// set phase_step to this value to recalculate it each sample; needed
	// in the case of PM LFO changes

	uint16_t       *waveform;         // base of sine table
	uint32_t phase_step;              // phase step, or PHASE_STEP_DYNAMIC if PM is active
	uint32_t total_level;             // total level * 8 + KSL
	uint32_t block_freq;              // raw block frequency value (used to compute phase_step)
	int32_t detune;                   // detuning value (used to compute phase_step)
	uint32_t multiple;                // multiple value (x.1, used to compute phase_step)
	uint32_t eg_sustain;              // sustain level, shifted up to envelope values
	uint8_t eg_rate[EG_STATES];       // envelope rate, including KSR
	uint8_t eg_shift;             // envelope shift amount
};


// ======================> fm_registers_base

// base class for family-specific register classes; this provides a few
// constants, common defaults, and helpers, but mostly each derived class is
// responsible for defining all commonly-called methods
class fm_registers_base
{
public:
	//
	// the following constants need to be defined per family:
	//          uint32_t OUTPUTS: The number of outputs exposed (1-4)
	//         uint32_t CHANNELS: The number of channels on the chip
	//     uint32_t ALL_CHANNELS: A bitmask of all channels
	//        uint32_t OPERATORS: The number of operators on the chip
	//        uint32_t WAVEFORMS: The number of waveforms offered
	//        uint32_t REGISTERS: The number of 8-bit registers allocated
	// uint32_t DEFAULT_PRESCALE: The starting clock prescale
	// uint32_t EG_CLOCK_DIVIDER: The clock divider of the envelope generator
	// uint32_t CSM_TRIGGER_MASK: Mask of channels to trigger in CSM mode
	//         uint32_t REG_MODE: The address of the "mode" register controlling timers
	//     uint8_t STATUS_TIMERA: Status bit to set when timer A fires
	//     uint8_t STATUS_TIMERB: Status bit to set when tiemr B fires
	//       uint8_t STATUS_BUSY: Status bit to set when the chip is busy
	//        uint8_t STATUS_IRQ: Status bit to set when an IRQ is signalled
	//
	// the following constants are uncommon:
	//          bool DYNAMIC_OPS: True if ops/channel can be changed at runtime (OPL3+)
	//       bool EG_HAS_DEPRESS: True if the chip has a DP ("depress"?) envelope stage (OPLL)
	//        bool EG_HAS_REVERB: True if the chip has a faux reverb envelope stage (OPQ/OPZ)
	//           bool EG_HAS_SSG: True if the chip has SSG envelope support (OPN)
	//      bool MODULATOR_DELAY: True if the modulator is delayed by 1 sample (OPL pre-OPL3)
	//

	// system-wide register defaults
	uint32_t status_mask()                           { return 0; } // OPL only
	uint32_t irq_reset()                             { return 0; } // OPL only
	uint32_t noise_enable()                          { return 0; } // OPM only
	uint32_t rhythm_enable()                         { return 0; } // OPL only

	// per-operator register defaults
	uint32_t op_ssg_eg_enable(uint32_t opoffs)       { return 0; } // OPN(A) only
	uint32_t op_ssg_eg_mode(uint32_t opoffs)         { return 0; } // OPN(A) only

	// helper to encode four operator numbers into a 32-bit value in the
	// operator maps for each register class
	uint32_t operator_list(uint8_t o1 = 0xff, uint8_t o2 = 0xff, uint8_t o3 = 0xff, uint8_t o4 = 0xff)
	{
		return o1 | (o2 << 8) | (o3 << 16) | (o4 << 24);
	}

	// helper to apply KSR to the raw ADSR rate, ignoring ksr if the
	// raw value is 0, and clamping to 63
	uint32_t effective_rate(uint32_t rawrate, uint32_t ksr)
	{
		return (rawrate == 0) ? 0 : std::min<uint32_t>(rawrate + ksr, 63);
	}
};


//*********************************************************
//  REGISTER CLASSES
//*********************************************************

// ======================> opl_registers_base

//
// OPL/OPL2/OPL3/OPL4 register map:
//
//      System-wide registers:
//           01 xxxxxxxx Test register
//              --x----- Enable OPL compatibility mode [OPL2 only] (1 = enable)
//           02 xxxxxxxx Timer A value (4 * OPN)
//           03 xxxxxxxx Timer B value
//           04 x------- RST
//              -x------ Mask timer A
//              --x----- Mask timer B
//              ------x- Load timer B
//              -------x Load timer A
//           08 x------- CSM mode [OPL/OPL2 only]
//              -x------ Note select
//           BD x------- AM depth
//              -x------ PM depth
//              --x----- Rhythm enable
//              ---x---- Bass drum key on
//              ----x--- Snare drum key on
//              -----x-- Tom key on
//              ------x- Top cymbal key on
//              -------x High hat key on
//          101 --xxxxxx Test register 2 [OPL3 only]
//          104 --x----- Channel 6 4-operator mode [OPL3 only]
//              ---x---- Channel 5 4-operator mode [OPL3 only]
//              ----x--- Channel 4 4-operator mode [OPL3 only]
//              -----x-- Channel 3 4-operator mode [OPL3 only]
//              ------x- Channel 2 4-operator mode [OPL3 only]
//              -------x Channel 1 4-operator mode [OPL3 only]
//          105 -------x New [OPL3 only]
//              ------x- New2 [OPL4 only]
//
//     Per-channel registers (channel in address bits 0-3)
//     Note that all these apply to address+100 as well on OPL3+
//        A0-A8 xxxxxxxx F-number (low 8 bits)
//        B0-B8 --x----- Key on
//              ---xxx-- Block (octvate, 0-7)
//              ------xx F-number (high two bits)
//        C0-C8 x------- CHD output (to DO0 pin) [OPL3+ only]
//              -x------ CHC output (to DO0 pin) [OPL3+ only]
//              --x----- CHB output (mixed right, to DO2 pin) [OPL3+ only]
//              ---x---- CHA output (mixed left, to DO2 pin) [OPL3+ only]
//              ----xxx- Feedback level for operator 1 (0-7)
//              -------x Operator connection algorithm
//
//     Per-operator registers (operator in bits 0-5)
//     Note that all these apply to address+100 as well on OPL3+
//        20-35 x------- AM enable
//              -x------ PM enable (VIB)
//              --x----- EG type
//              ---x---- Key scale rate
//              ----xxxx Multiple value (0-15)
//        40-55 xx------ Key scale level (0-3)
//              --xxxxxx Total level (0-63)
//        60-75 xxxx---- Attack rate (0-15)
//              ----xxxx Decay rate (0-15)
//        80-95 xxxx---- Sustain level (0-15)
//              ----xxxx Release rate (0-15)
//        E0-F5 ------xx Wave select (0-3) [OPL2 only]
//              -----xxx Wave select (0-7) [OPL3+ only]
//


class opl3_registers : public fm_registers_base
{

public:
	// constants

	// constructor
	opl3_registers();

	// reset to initial state
	void reset();

	// save/restore
	void save_restore(ymfm_saved_state& state);

	// map channel number to register offset
	static uint32_t channel_offset(uint32_t chnum)
	{
		assert(chnum < CHANNELS);
		if (!IsOpl3Plus)
			return chnum;
		else
			return (chnum % 9) + 0x100 * (chnum / 9);
	}

	// map operator number to register offset
	static uint32_t operator_offset(uint32_t opnum)
	{
		assert(opnum < OPERATORS);
		if (!IsOpl3Plus)
			return opnum + 2 * (opnum / 6);
		else
			return (opnum % 18) + 2 * ((opnum % 18) / 6) + 0x100 * (opnum / 18);
	}

	// return an array of operator indices for each channel
	void operator_map(operator_mapping& dest);

	// OPL4 apparently can read back FM registers?
	uint8_t read(uint16_t index) { return m_regdata[index]; }

	// handle writes to the register array
	bool write(uint16_t index, uint8_t data, uint32_t& chan, uint32_t& opmask);

	// clock the noise and LFO, if present, returning LFO PM value
	int32_t clock_noise_and_lfo();

	// reset the LFO
	void reset_lfo() { m_lfo_am_counter = m_lfo_pm_counter = 0; }

	// return the AM offset from LFO for the given channel
	// on OPL this is just a fixed value
	uint32_t lfo_am_offset(uint32_t choffs) { return m_lfo_am; }

	// return LFO/noise states
	uint32_t noise_state() { return m_noise_lfsr >> 23; }

	// caching helpers
	void cache_operator_data(uint32_t choffs, uint32_t opoffs, opdata_cache& cache);

	// compute the phase step, given a PM value
	uint32_t compute_phase_step(uint32_t choffs, uint32_t opoffs, opdata_cache& cache, int32_t lfo_raw_pm);

	// log a key-on event
	std::string log_keyon(uint32_t choffs, uint32_t opoffs);

	// system-wide registers
	uint32_t test() { return byte(0x01, 0, 8); }
	uint32_t waveform_enable() { return IsOpl2 ? byte(0x01, 5, 1) : (IsOpl3Plus ? 1 : 0); }
	uint32_t timer_a_value() { return byte(0x02, 0, 8) * 4; } // 8->10 bits
	uint32_t timer_b_value() { return byte(0x03, 0, 8); }
	uint32_t status_mask() { return byte(0x04, 0, 8) & 0x78; }
	uint32_t irq_reset() { return byte(0x04, 7, 1); }
	uint32_t reset_timer_b() { return byte(0x04, 7, 1) | byte(0x04, 5, 1); }
	uint32_t reset_timer_a() { return byte(0x04, 7, 1) | byte(0x04, 6, 1); }
	uint32_t enable_timer_b() { return 1; }
	uint32_t enable_timer_a() { return 1; }
	uint32_t load_timer_b() { return byte(0x04, 1, 1); }
	uint32_t load_timer_a() { return byte(0x04, 0, 1); }
	uint32_t csm() { return IsOpl3Plus ? 0 : byte(0x08, 7, 1); }
	uint32_t note_select() { return byte(0x08, 6, 1); }
	uint32_t lfo_am_depth() { return byte(0xbd, 7, 1); }
	uint32_t lfo_pm_depth() { return byte(0xbd, 6, 1); }
	uint32_t rhythm_enable() { return byte(0xbd, 5, 1); }
	uint32_t rhythm_keyon() { return byte(0xbd, 4, 0); }
	uint32_t newflag() { return IsOpl3Plus ? byte(0x105, 0, 1) : 0; }
	uint32_t new2flag() { return IsOpl4Plus ? byte(0x105, 1, 1) : 0; }
	uint32_t fourop_enable() { return IsOpl3Plus ? byte(0x104, 0, 6) : 0; }

	// per-channel registers
	uint32_t ch_block_freq(uint32_t choffs) { return word(0xb0, 0, 5, 0xa0, 0, 8, choffs); }
	uint32_t ch_feedback(uint32_t choffs) { return byte(0xc0, 1, 3, choffs); }
	uint32_t ch_algorithm(uint32_t choffs) { return byte(0xc0, 0, 1, choffs) | (IsOpl3Plus ? (8 | (byte(0xc3, 0, 1, choffs) << 1)) : 0); }
	uint32_t ch_output_any(uint32_t choffs) { return newflag() ? byte(0xc0 + choffs, 4, 4) : 1; }
	uint32_t ch_output_0(uint32_t choffs) { return newflag() ? byte(0xc0 + choffs, 4, 1) : 1; }
	uint32_t ch_output_1(uint32_t choffs) { return newflag() ? byte(0xc0 + choffs, 5, 1) : (IsOpl3Plus ? 1 : 0); }
	uint32_t ch_output_2(uint32_t choffs) { return newflag() ? byte(0xc0 + choffs, 6, 1) : 0; }
	uint32_t ch_output_3(uint32_t choffs) { return newflag() ? byte(0xc0 + choffs, 7, 1) : 0; }

	// per-operator registers
	uint32_t op_lfo_am_enable(uint32_t opoffs) { return byte(0x20, 7, 1, opoffs); }
	uint32_t op_lfo_pm_enable(uint32_t opoffs) { return byte(0x20, 6, 1, opoffs); }
	uint32_t op_eg_sustain(uint32_t opoffs) { return byte(0x20, 5, 1, opoffs); }
	uint32_t op_ksr(uint32_t opoffs) { return byte(0x20, 4, 1, opoffs); }
	uint32_t op_multiple(uint32_t opoffs) { return byte(0x20, 0, 4, opoffs); }
	uint32_t op_ksl(uint32_t opoffs) { uint32_t temp = byte(0x40, 6, 2, opoffs); return bitfield(temp, 1) | (bitfield(temp, 0) << 1); }
	uint32_t op_total_level(uint32_t opoffs) { return byte(0x40, 0, 6, opoffs); }
	uint32_t op_attack_rate(uint32_t opoffs) { return byte(0x60, 4, 4, opoffs); }
	uint32_t op_decay_rate(uint32_t opoffs) { return byte(0x60, 0, 4, opoffs); }
	uint32_t op_sustain_level(uint32_t opoffs) { return byte(0x80, 4, 4, opoffs); }
	uint32_t op_release_rate(uint32_t opoffs) { return byte(0x80, 0, 4, opoffs); }
	uint32_t op_waveform(uint32_t opoffs) { return IsOpl2Plus ? byte(0xe0, 0, newflag() ? 3 : 2, opoffs) : 0; }

protected:
	// return a bitfield extracted from a byte
	uint32_t byte(uint32_t offset, uint32_t start, uint32_t count, uint32_t extra_offset = 0)
	{
		return bitfield(m_regdata[offset + extra_offset], start, count);
	}

	// return a bitfield extracted from a pair of bytes, MSBs listed first
	uint32_t word(uint32_t offset1, uint32_t start1, uint32_t count1, uint32_t offset2, uint32_t start2, uint32_t count2, uint32_t extra_offset = 0)
	{
		return (byte(offset1, start1, count1, extra_offset) << count2) | byte(offset2, start2, count2, extra_offset);
	}

	// helper to determine if the this channel is an active rhythm channel
	bool is_rhythm(uint32_t choffs)
	{
		return rhythm_enable() && (choffs >= 6 && choffs <= 8);
	}

	// internal state
	uint16_t m_lfo_am_counter;            // LFO AM counter
	uint16_t m_lfo_pm_counter;            // LFO PM counter
	uint32_t m_noise_lfsr;                // noise LFSR state
	uint8_t m_lfo_am;                     // current LFO AM value
	uint8_t m_regdata[REGISTERS];         // register data
	uint16_t m_waveform[WAVEFORMS][WAVEFORM_LENGTH]; // waveforms
};


//*********************************************************
//  CORE ENGINE CLASSES
//*********************************************************

// forward declarations
class fm_engine_base;

// ======================> fm_operator

// fm_operator represents an FM operator (or "slot" in FM parlance), which
// produces an output sine wave modulated by an envelope
class fm_operator
{
	// "quiet" value, used to optimize when we can skip doing work

public:
	// constructor
	fm_operator(fm_engine_base &owner, uint32_t opoffs);

	// save/restore
	void save_restore(ymfm_saved_state &state);

	// reset the operator state
	void reset();

	// return the operator/channel offset
	uint32_t opoffs()       { return m_opoffs; }
	uint32_t choffs()       { return m_choffs; }

	// set the current channel
	void set_choffs(uint32_t choffs) { m_choffs = choffs; }

	// prepare prior to clocking
	bool prepare();

	// master clocking function
	void clock(uint32_t env_counter, int32_t lfo_raw_pm);

	// return the current phase value
	uint32_t phase()       { return m_phase >> 10; }

	// compute operator volume
	int32_t compute_volume(uint32_t phase, uint32_t am_offset);

	// compute volume for the OPM noise channel
	int32_t compute_noise_volume(uint32_t am_offset);

	// key state control
	void keyonoff(uint32_t on, keyon_type type);

	// return a reference to our registers
	opl3_registers &regs()       { return m_regs; }

	// simple getters for debugging
	envelope_state debug_eg_state()       { return m_env_state; }
	uint16_t debug_eg_attenuation()       { return m_env_attenuation; }
	uint8_t debug_ssg_inverted()       { return m_ssg_inverted; }
	opdata_cache &debug_cache() { return m_cache; }

private:
	// start the attack phase
	void start_attack(bool is_restart = false);

	// start the release phase
	void start_release();

	// clock phases
	void clock_keystate(uint32_t keystate);
	void clock_ssg_eg_state();
	void clock_envelope(uint32_t env_counter);
	void clock_phase(int32_t lfo_raw_pm);

	// return effective attenuation of the envelope
	uint32_t envelope_attenuation(uint32_t am_offset);

	// internal state
	uint32_t m_choffs;                     // channel offset in registers
	uint32_t m_opoffs;                     // operator offset in registers
	uint32_t m_phase;                      // current phase value (10.10 format)
	uint16_t m_env_attenuation;            // computed envelope attenuation (4.6 format)
	envelope_state m_env_state;            // current envelope state
	uint8_t m_ssg_inverted;                // non-zero if the output should be inverted (bit 0)
	uint8_t m_key_state;                   // current key state: on or off (bit 0)
	uint8_t m_keyon_live;                  // live key on state (bit 0 = direct, bit 1 = rhythm, bit 2 = CSM)
	uint8_t m_keyon_request;               // request key on (set to 1 when a key on is requested, and cleared after the key on occurs)
	opdata_cache m_cache;                  // cached values for performance
	opl3_registers&m_regs;                  // direct reference to registers
	fm_engine_base &m_owner; // reference to the owning engine
};


// ======================> fm_channel

// fm_channel represents an FM channel which combines the output of 2 or 4
// operators into a final result
class fm_channel
{
#define output_data	ymfm_output

public:
	// constructor
	fm_channel(fm_engine_base &owner, uint32_t choffs);

	// save/restore
	void save_restore(ymfm_saved_state &state);

	// reset the channel state
	void reset();

	// return the channel offset
	uint32_t choffs()       { return m_choffs; }

	// assign operators
	void assign(uint32_t index, fm_operator *op)
	{
		assert(index < m_op.size());
		m_op[index] = op;
		if (op != nullptr)
			op->set_choffs(m_choffs);
	}

	// signal key on/off to our operators
	void keyonoff(uint32_t states, keyon_type type, uint32_t chnum);

	// prepare prior to clocking
	bool prepare();

	// master clocking function
	void clock(uint32_t env_counter, int32_t lfo_raw_pm);

	// specific 2-operator and 4-operator output handlers
	void output_2op(output_data &output, uint32_t rshift, int32_t clipmax);
	void output_4op(output_data &output, uint32_t rshift, int32_t clipmax);

	// compute the special OPL rhythm channel outputs
	void output_rhythm_ch6(output_data &output, uint32_t rshift, int32_t clipmax);
	void output_rhythm_ch7(uint32_t phase_select, output_data &output, uint32_t rshift, int32_t clipmax);
	void output_rhythm_ch8(uint32_t phase_select, output_data &output, uint32_t rshift, int32_t clipmax);

	// are we a 4-operator channel or a 2-operator one?
	bool is4op()
	{
		if (DYNAMIC_OPS)
			return (m_op[2] != nullptr);
		return (OPERATORS / CHANNELS == 4);
	}

	// return a reference to our registers
	opl3_registers &regs()       { return m_regs; }

	// simple getters for debugging
	fm_operator *debug_operator(uint32_t index)       { return m_op[index]; }

private:
	// helper to add values to the outputs based on channel enables
	void add_to_output(uint32_t choffs, output_data &output, int32_t value)
	{
		// create these constants to appease overzealous compilers checking array
		// bounds in unreachable code (looking at you, clang)
		int out0_index = 0;
		int out1_index = 1 % OUTPUTS;
		int out2_index = 2 % OUTPUTS;
		int out3_index = 3 % OUTPUTS;

		if (OUTPUTS == 1 || m_regs.ch_output_0(choffs))
			output.data[out0_index] += value;
		if (OUTPUTS >= 2 && m_regs.ch_output_1(choffs))
			output.data[out1_index] += value;
		if (OUTPUTS >= 3 && m_regs.ch_output_2(choffs))
			output.data[out2_index] += value;
		if (OUTPUTS >= 4 && m_regs.ch_output_3(choffs))
			output.data[out3_index] += value;
	}

	// internal state
	uint32_t m_choffs;                     // channel offset in registers
	int16_t m_feedback[2];                 // feedback memory for operator 1
	mutable int16_t m_feedback_in;         // next input value for op 1 feedback (set in output)
	std::array<fm_operator*, 4> m_op; // up to 4 operators
	opl3_registers&m_regs;                  // direct reference to registers
	fm_engine_base &m_owner; // reference to the owning engine
};


// ======================> fm_engine_base

// fm_engine_base represents a set of operators and channels which together
// form a Yamaha FM core; chips that implement other engines (ADPCM, wavetable,
// etc) take this output and combine it with the others externally
class fm_engine_base : public ymfm_engine_callbacks
{
public:
	// expose the correct output class
#define output_data ymfm_output

	// constructor
	fm_engine_base(ymfm_interface &intf);

	// save/restore
	void save_restore(ymfm_saved_state &state);

	// reset the overall state
	void reset();

	// master clocking function
	uint32_t clock(uint32_t chanmask);

	// compute sum of channel outputs
	void output(output_data &output, uint32_t rshift, int32_t clipmax, uint32_t chanmask);

	// write to the OPN registers
	void write(uint16_t regnum, uint8_t data);

	// return the current status
	uint8_t status();

	// set/reset bits in the status register, updating the IRQ status
	uint8_t set_reset_status(uint8_t set, uint8_t reset)
	{
		m_status = (m_status | set) & ~(reset | STATUS_BUSY);
		m_intf.ymfm_sync_check_interrupts();
		return m_status & ~m_regs.status_mask();
	}

	// set the IRQ mask
	void set_irq_mask(uint8_t mask) { m_irq_mask = mask; m_intf.ymfm_sync_check_interrupts(); }

	// return the current clock prescale
	uint32_t clock_prescale()       { return m_clock_prescale; }

	// set prescale factor (2/3/6)
	void set_clock_prescale(uint32_t prescale) { m_clock_prescale = prescale; }

	// compute sample rate
	uint32_t sample_rate(uint32_t baseclock)
	{
#if (YMFM_DEBUG_LOG_WAVFILES)
		for (uint32_t chnum = 0; chnum < CHANNELS; chnum++)
			m_wavfile[chnum].set_samplerate(baseclock / (m_clock_prescale * OPERATORS));
#endif
		return baseclock / (m_clock_prescale * OPERATORS);
	}

	// return the owning device
	ymfm_interface &intf()       { return m_intf; }

	// return a reference to our registers
	opl3_registers &regs() { return m_regs; }

	// invalidate any caches
	void invalidate_caches() { m_modified_channels = ALL_CHANNELS; }

	// simple getters for debugging
	fm_channel *debug_channel(uint32_t index)       { return m_channel[index].get(); }
	fm_operator *debug_operator(uint32_t index)       { return m_operator[index].get(); }

public:
	// timer callback; called by the interface when a timer fires
	virtual void engine_timer_expired(uint32_t tnum) override;

	// check interrupts; called by the interface after synchronization
	virtual void engine_check_interrupts() override;

	// mode register write; called by the interface after synchronization
	virtual void engine_mode_write(uint8_t data) override;

protected:
	// assign the current set of operators to channels
	void assign_operators();

	// update the state of the given timer
	void update_timer(uint32_t which, uint32_t enable, int32_t delta_clocks);

	// internal state
	ymfm_interface &m_intf;          // reference to the system interface
	uint32_t m_env_counter;          // envelope counter; low 2 bits are sub-counter
	uint8_t m_status;                // current status register
	uint8_t m_clock_prescale;        // prescale factor (2/3/6)
	uint8_t m_irq_mask;              // mask of which bits signal IRQs
	uint8_t m_irq_state;             // current IRQ state
	uint8_t m_timer_running[2];      // current timer running state
	uint8_t m_total_clocks;          // low 8 bits of the total number of clocks processed
	uint32_t m_active_channels;      // mask of active channels (computed by prepare)
	uint32_t m_modified_channels;    // mask of channels that have been modified
	uint32_t m_prepare_count;        // counter to do periodic prepare sweeps
	opl3_registers m_regs;             // register accessor
	std::unique_ptr<fm_channel> m_channel[CHANNELS]; // channel pointers
	std::unique_ptr<fm_operator> m_operator[OPERATORS]; // operator pointers
#if (YMFM_DEBUG_LOG_WAVFILES)
	mutable ymfm_wavfile<1> m_wavfile[CHANNELS]; // for debugging
#endif
};

}

#endif // YMFM_FM_H
