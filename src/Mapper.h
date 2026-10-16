#ifndef _MAPPER_H_
#define _MAPPER_H_


#include "Cpu.h"
#include "MemoryMappedFile.h"
#include "ROMDatabase.h"
#include "VRAMBank.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>


enum class MapperDebugMemoryType {
	Unmapped,
	PRGROM,
	PRGRAM,
	CHRROM,
	CHRRAM
};


enum class MapperDebugMirroring {
	Unknown,
	SingleLow,
	SingleHigh,
	Vertical,
	Horizontal,
	FourScreen
};


struct mapper_debug_bank_t {
	uint16_t address = 0;
	uint32_t size    = 0;
	uint32_t bank    = 0;

	MapperDebugMemoryType type = MapperDebugMemoryType::Unmapped;

	bool readable = false;
	bool writable = false;
};


struct mapper_debug_state_t {
	mapper_debug_bank_t prg[5];
	mapper_debug_bank_t chr[8];

	MapperDebugMirroring mirroring = MapperDebugMirroring::Unknown;

	uint64_t revision = 0;
};


// -----------------------------------------------------------------------------
// ExpansionAudioType
//
// Identifies the type of mapper-provided expansion-audio hardware represented
// by a mapper audio debugger snapshot.
//
// None indicates that the mapper does not provide expansion audio.
// -----------------------------------------------------------------------------
enum class ExpansionAudioType {
	None,
	VRC6
};


// -----------------------------------------------------------------------------
// mapper_audio_debug_field_type
//
// Describes how an additional expansion-audio debugger field should be
// interpreted by the debugger UI.
//
// Value represents an ordinary numeric value. Boolean represents a true/false
// state that may be displayed using semantic state coloring.
// -----------------------------------------------------------------------------
enum class mapper_audio_debug_field_type {
	Value,
	Boolean,
	Hex,
	Text,
	Output,
	Percent,
	Timing
};


// -----------------------------------------------------------------------------
// mapper_audio_debug_field_t
//
// Describes one mapper-provided expansion-audio debugger field.
//
// Mapper-specific audio implementations use these fields to expose additional
// channel state without requiring the generic debugger interface to know about
// chip-specific concepts such as VRC6 duty, accumulator, or saw step.
//
// The label is static descriptive text owned by the mapper implementation.
// -----------------------------------------------------------------------------
struct mapper_audio_debug_field_t {
	const char *label = nullptr;

	uint32_t value = 0;
	uint32_t maximum = 100;

	const char *text = nullptr;

	mapper_audio_debug_field_type type = mapper_audio_debug_field_type::Value;
};

// -----------------------------------------------------------------------------
// mapper_audio_channel_debug_t
//
// Describes one mapper-provided expansion-audio channel.
//
// The common enabled, period, and output values are available directly.
// Additional mapper-specific channel information is exposed through fields[].
// -----------------------------------------------------------------------------
struct mapper_audio_channel_debug_t {
	std::string name;

	uint16_t period = 0;
	uint8_t output = 0;
	uint8_t output_maximum = 0;

	double frequency_hz = 0.0;
	bool frequency_valid = false;
	bool halted = false;

	bool enabled = false;

	mapper_audio_debug_field_t fields[8];
	size_t field_count = 0;
};


// -----------------------------------------------------------------------------
// mapper_audio_debug_state_t
//
// Snapshot of the expansion-audio state exposed by a mapper.
//
// Mappers without expansion audio return the default empty state. Expansion
// audio implementations populate the hardware type, optional mapper-wide
// fields, and one or more channels.
//
// Mapper-wide fields describe expansion-audio state that affects the whole
// device rather than one individual channel.
//
// Examples include VRC6 frequency-control state from $9003.
//
// -----------------------------------------------------------------------------
struct mapper_audio_debug_state_t {
	ExpansionAudioType type = ExpansionAudioType::None;

	std::string name;

	uint32_t output = 0;
	uint32_t output_maximum = 0;

	mapper_audio_debug_field_t fields[8];
	size_t field_count = 0;

	mapper_audio_channel_debug_t channels[3];
	size_t channel_count = 0;
};




// -----------------------------------------------------------------------------
// mapper_audio_scope_sample_t
//
// Describes one instantaneous mapper-provided expansion-audio oscilloscope
// sample.
//
// channels[] contains the current raw output level of each expansion-audio
// channel. channel_count identifies how many entries are valid.
//
// Mappers without expansion audio return the default empty sample.
// -----------------------------------------------------------------------------
struct mapper_audio_scope_sample_t {
	uint8_t channels[8] = {};
	size_t channel_count = 0;
};


class Mapper;

using create_func = std::function<std::unique_ptr<Mapper>()>;


class Mapper
{
	public:
	static std::unique_ptr<Mapper> create_mapper (int num, int submapper = 0,
												 const ROMDatabase::Entry *database_entry = nullptr);
	static void register_mapper(int num, create_func create_ptr);

	public:
	MemoryMappedFile open_sram(size_t size);

	public:
	// the standard mapper interface
	virtual void save() {}
	virtual void restore() {}
	virtual std::string name() const = 0;

	// debugger-facing resolved mapper state
	mapper_debug_state_t debug_state() const;

	protected:
	Mapper();
	Mapper(const Mapper &) = delete;
	Mapper &operator=(const Mapper &) = delete;

	public:
	virtual ~Mapper() = default;

	public:
	// write routines
	virtual void write_2 (uint_least16_t address, uint8_t value);
	virtual void write_3 (uint_least16_t address, uint8_t value);
	virtual void write_4 (uint_least16_t address, uint8_t value);
	virtual void write_5 (uint_least16_t address, uint8_t value);
	virtual void write_6 (uint_least16_t address, uint8_t value);
	virtual void write_7 (uint_least16_t address, uint8_t value);
	virtual void write_8 (uint_least16_t address, uint8_t value);
	virtual void write_9 (uint_least16_t address, uint8_t value);
	virtual void write_a (uint_least16_t address, uint8_t value);
	virtual void write_b (uint_least16_t address, uint8_t value);
	virtual void write_c (uint_least16_t address, uint8_t value);
	virtual void write_d (uint_least16_t address, uint8_t value);
	virtual void write_e (uint_least16_t address, uint8_t value);
	virtual void write_f (uint_least16_t address, uint8_t value);

	// read routines
	virtual uint8_t read_2 (uint_least16_t address);
	virtual uint8_t read_3 (uint_least16_t address);
	virtual uint8_t read_4 (uint_least16_t address);
	virtual uint8_t read_5 (uint_least16_t address);
	virtual uint8_t read_6 (uint_least16_t address);
	virtual uint8_t read_7 (uint_least16_t address);
	virtual uint8_t read_8 (uint_least16_t address);
	virtual uint8_t read_9 (uint_least16_t address);
	virtual uint8_t read_a (uint_least16_t address);
	virtual uint8_t read_b (uint_least16_t address);
	virtual uint8_t read_c (uint_least16_t address);
	virtual uint8_t read_d (uint_least16_t address);
	virtual uint8_t read_e (uint_least16_t address);
	virtual uint8_t read_f (uint_least16_t address);

	// hooks for mappers to intercept VRAM/VROM read/writes
	virtual void write_vram(uint_least16_t address, uint8_t value);
	virtual uint8_t read_vram(uint_least16_t address);

	virtual void cpu_sync();
	virtual void ppu_end_frame();
	virtual void vram_change_hook(uint_least16_t vram_address);

	// mapper lifecycle reset hook
	virtual void reset()
	{
	}


// -----------------------------------------------------------------------------
// audio_output
//
// Returns the mapper's current expansion-audio output level.
//
// Mappers without expansion audio return zero. Mapper implementations that
// provide additional sound hardware override this method.
//
// Returns:
//   Current unsigned expansion-audio level.
// -----------------------------------------------------------------------------
virtual uint8_t audio_output() const
{
	return 0;
}


// -----------------------------------------------------------------------------
// debug_audio_state
//
// Returns debugger-facing state for mapper-provided expansion audio.
//
// Mappers without expansion-audio hardware return an empty state. Mapper
// implementations that provide additional audio channels override this method
// and expose their current channel state.
//
// Returns:
//   Current mapper expansion-audio debug state.
// -----------------------------------------------------------------------------
virtual mapper_audio_debug_state_t debug_audio_state() const
{
	return {};
}



// -----------------------------------------------------------------------------
// debug_audio_scope_sample
//
// Returns instantaneous per-channel expansion-audio output levels for the
// debugger oscilloscope.
//
// Mappers without expansion-audio hardware return an empty sample. Mapper
// implementations that provide additional audio channels override this method.
//
// Returns:
//   Current mapper expansion-audio oscilloscope sample.
// -----------------------------------------------------------------------------
virtual mapper_audio_scope_sample_t debug_audio_scope_sample() const
{
	return {};
}


	protected:
	uint8_t read_memory (uint_least16_t address);

	protected:
	// ---- PRG ROM ----
	// 8K swaps
	void set_prg_67(int num);
	void set_prg_89(int num);
	void set_prg_ab(int num);
	void set_prg_cd(int num);
	void set_prg_ef(int num);

	// 16K swaps
	void set_prg_89ab(int num);
	void set_prg_cdef(int num);

	// 32K swap
	void set_prg_89abcdef(int num);

	// ---- CHR ROM ----
	// 1K-swaps
	void set_chr_0000_03ff(int num);
	void set_chr_0400_07ff(int num);
	void set_chr_0800_0bff(int num);
	void set_chr_0c00_0fff(int num);
	void set_chr_1000_13ff(int num);
	void set_chr_1400_17ff(int num);
	void set_chr_1800_1bff(int num);
	void set_chr_1c00_1fff(int num);

	// 2K swaps
	void set_chr_0000_07ff(int num);
	void set_chr_0800_0fff(int num);
	void set_chr_1000_17ff(int num);
	void set_chr_1800_1fff(int num);

	// 4K swaps
	void set_chr_0000_0fff(int num);
	void set_chr_1000_1fff(int num);

	// 8K swaps
	void set_chr_0000_1fff(int num);

	// ---- CHR RAM ----
	// 1K-swaps
	void set_chr_0000_03ff_ram(uint8_t *p, int num);
	void set_chr_0400_07ff_ram(uint8_t *p, int num);
	void set_chr_0800_0bff_ram(uint8_t *p, int num);
	void set_chr_0c00_0fff_ram(uint8_t *p, int num);
	void set_chr_1000_13ff_ram(uint8_t *p, int num);
	void set_chr_1400_17ff_ram(uint8_t *p, int num);
	void set_chr_1800_1bff_ram(uint8_t *p, int num);
	void set_chr_1c00_1fff_ram(uint8_t *p, int num);

	// 2K swaps
	void set_chr_0000_07ff_ram(uint8_t *p, int num);
	void set_chr_0800_0fff_ram(uint8_t *p, int num);
	void set_chr_1000_17ff_ram(uint8_t *p, int num);
	void set_chr_1800_1fff_ram(uint8_t *p, int num);

	// 4K swaps
	void set_chr_0000_0fff_ram(uint8_t *p, int num);
	void set_chr_1000_1fff_ram(uint8_t *p, int num);

	// 8K swaps
	void set_chr_0000_1fff_ram(uint8_t *p, int num);

	protected:
	void set_mirroring(uint8_t mir);
	void debug_set_mirroring_unknown();

	// Records the debugger-visible state of the $6000-$7FFF PRG-RAM region.
	void debug_set_prg_ram (bool enabled, bool writable = true, uint32_t bank = 0);

	protected:
	void swap_67(uint8_t *ptr)
	{
		page_[0x0c] = ptr + 0x0000;
		page_[0x0d] = ptr + 0x0800;
		page_[0x0e] = ptr + 0x1000;
		page_[0x0f] = ptr + 0x1800;
	}

	void swap_89(uint8_t *ptr)
	{
		page_[0x10] = ptr + 0x0000;
		page_[0x11] = ptr + 0x0800;
		page_[0x12] = ptr + 0x1000;
		page_[0x13] = ptr + 0x1800;
	}

	void swap_ab(uint8_t *ptr)
	{
		page_[0x14] = ptr + 0x0000;
		page_[0x15] = ptr + 0x0800;
		page_[0x16] = ptr + 0x1000;
		page_[0x17] = ptr + 0x1800;
	}

	void swap_cd(uint8_t *ptr)
	{
		page_[0x18] = ptr + 0x0000;
		page_[0x19] = ptr + 0x0800;
		page_[0x1a] = ptr + 0x1000;
		page_[0x1b] = ptr + 0x1800;
	}

	void swap_ef(uint8_t *ptr)
	{
		page_[0x1c] = ptr + 0x0000;
		page_[0x1d] = ptr + 0x0800;
		page_[0x1e] = ptr + 0x1000;
		page_[0x1f] = ptr + 0x1800;
	}

	void unmap_67()
	{
		page_[0x0c] = nullptr;
		page_[0x0d] = nullptr;
		page_[0x0e] = nullptr;
		page_[0x0f] = nullptr;

		debug_set_prg_bank(0, 0x6000, 0, MapperDebugMemoryType::Unmapped, false, false);
	}

	void unmap_89()
	{
		page_[0x10] = nullptr;
		page_[0x11] = nullptr;
		page_[0x12] = nullptr;
		page_[0x13] = nullptr;

		debug_set_prg_bank(1, 0x8000, 0, MapperDebugMemoryType::Unmapped, false, false);
	}

	void unmap_ab()
	{
		page_[0x14] = nullptr;
		page_[0x15] = nullptr;
		page_[0x16] = nullptr;
		page_[0x17] = nullptr;

		debug_set_prg_bank(2, 0xa000, 0, MapperDebugMemoryType::Unmapped, false, false);
	}

	void unmap_cd()
	{
		page_[0x18] = nullptr;
		page_[0x19] = nullptr;
		page_[0x1a] = nullptr;
		page_[0x1b] = nullptr;

		debug_set_prg_bank(3, 0xc000, 0, MapperDebugMemoryType::Unmapped, false, false);
	}

	void unmap_ef()
	{
		page_[0x1c] = nullptr;
		page_[0x1d] = nullptr;
		page_[0x1e] = nullptr;
		page_[0x1f] = nullptr;

		debug_set_prg_bank(4, 0xe000, 0, MapperDebugMemoryType::Unmapped, false, false);
	}

	protected:
	constexpr static uint8_t mirror_single_low  = 0x00;
	constexpr static uint8_t mirror_single_high = 0x55;
	constexpr static uint8_t mirror_vertical    = 0x44;
	constexpr static uint8_t mirror_horizontal  = 0x50;
	constexpr static uint8_t mirror_4screen     = 0xe4;

	public:
	// Update one resolved PRG mapping slot for debugger inspection.
	void debug_set_prg_bank (int slot, uint16_t address, uint32_t bank,
							 MapperDebugMemoryType type,
							 bool readable = true, bool writable = false);

	// Update one resolved CHR mapping slot for debugger inspection.
	void debug_set_chr_bank (int slot, uint16_t address, uint32_t bank,
							 MapperDebugMemoryType type);

	private:
	constexpr static unsigned int PageCount = 32;
	constexpr static unsigned int PageSize  = 0x10000 / PageCount;
	constexpr static unsigned int PageMask  = PageSize - 1;
	constexpr static unsigned int PageShift = 11;

	private:
	uint8_t nametables_[4 * 0x400] = {}; // nametable and attribute table data
										 // 4 nametables, each $03c0 bytes in
										 // size plus 4 corresponding $40 byte
										 // attribute tables
										 // even though the real thing only has 2
										 // tables, we currently simulate 4 for
										 // simplicity

	VRAMBank vram_banks_[0x10] = {};

	uint8_t *page_[PageCount] = {};

	// Resolved CPU/PPU bank mappings exposed to the debugger.
	mapper_debug_state_t debug_state_;

	private:
	// we use a function here to return a static object
	// to ensure order of initialization, otherwise, the mappers
	// could try to register themselves before the map was constructed
	static std::map<int, create_func> &registered_mappers_ines()
	{
		static std::map<int, create_func> static_map;
		return static_map;
	}
};


// template which is a class whose constructor adds a function to our mapper
// creation table in order to create an object of each type of mapper
template <class T>
struct MapperRegisterObject {
	explicit MapperRegisterObject(int n)
	{
		Mapper::register_mapper(n, []() {
			return std::make_unique<T>();
		});
	}
};


// macro to hide details of setting up a static instance of the previously
// mentioned class. The first line of each Mapper[NNN].cpp file should be
// a call to this macro passing the appropriate mapper number
#define SETUP_STATIC_INES_MAPPER_REGISTRAR(N)                 \
	namespace {                                               \
	static MapperRegisterObject<Mapper##N> MapperRegister(N); \
	}


#endif	// _MAPPER_H_

