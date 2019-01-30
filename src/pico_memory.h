#ifndef PICO_MEMORY_H
#define PICO_MEMORY_H

#include <stdint.h>
#include <array>

namespace pico_ram {

	// pico 8 memory map
	const uint16_t MEM_GFX_ADDR = 0x0000;
	const uint16_t MEM_GFX_SIZE = 0x1000;
	const uint16_t MEM_GFX2_MAP2_ADDR = 0x1000;
	const uint16_t MEM_GFX2_MAP2_SIZE = 0x1000;
	const uint16_t MEM_MAP_ADDR = 0x2000;
	const uint16_t MEM_MAP_SIZE = 0x1000;
	const uint16_t MEM_GFX_PROPS_ADDR = 0x3000;
	const uint16_t MEM_GFX_PROPS_SIZE = 0x0100;
	const uint16_t MEM_MUSIC_ADDR = 0x3100;
	const uint16_t MEM_MUSIC_SIZE = 0x0100;
	const uint16_t MEM_SFX_ADDR = 0x3200;
	const uint16_t MEM_SFX_SIZE = 0x1100;
	const uint16_t MEM_SCRATCH_ADDR = 0x4300;
	const uint16_t MEM_SCRATCH_SIZE = 0x1b00;
	const uint16_t MEM_CART_DATA_ADDR = 0x5e00;
	const uint16_t MEM_CART_DATA_SIZE = 0x0100;
	const uint16_t MEM_SCREEN_ADDR = 0x6000;
	const uint16_t MEM_SCREEN_SIZE = 0x2000;

	struct IMemoryArea {
	   public:
		virtual uint16_t address() const = 0;
		virtual uint16_t size() const = 0;
		virtual uint8_t peek(uint16_t addr) = 0;
		virtual void poke(uint16_t addr, uint8_t val) = 0;
	};

	struct MemoryArea : public IMemoryArea {
	   protected:
		uint8_t* m_data;
		uint16_t m_address;
		uint16_t m_size;

	   public:
		MemoryArea(uint8_t* data, uint16_t address, uint16_t size)
		    : m_data(data), m_address(address), m_size(size) {
		}

		void setData(uint8_t* data) {
			m_data = data;
		}

		virtual uint16_t address() const {
			return m_address;
		}
		virtual uint16_t size() const {
			return m_size;
		}

		virtual uint8_t peek(uint16_t addr) = 0;
		virtual void poke(uint16_t addr, uint8_t val) = 0;
	};  // namespace pico_ram

	struct LinearMemoryArea : public MemoryArea {
		using MemoryArea::MemoryArea;

		virtual uint8_t peek(uint16_t addr) {
			return m_data[addr];
		}

		virtual void poke(uint16_t addr, uint8_t val) {
			m_data[addr] = val;
		}
	};

	struct LinearMemoryAreaDF : public MemoryArea {
		using MemoryArea::MemoryArea;
		bool m_isDirty = false;

		virtual uint8_t peek(uint16_t addr) {
			return m_data[addr];
		}

		virtual void poke(uint16_t addr, uint8_t val) {
			m_data[addr] = val;
			m_isDirty = true;
		}

		void clearDirty() {
			m_isDirty = false;
		}

		bool isDirty() {
			return m_isDirty;
		}
	};

	struct All0MemoryArea : public MemoryArea {
		using MemoryArea::MemoryArea;

		virtual uint8_t peek(uint16_t addr) {
			return 0;
		}

		virtual void poke(uint16_t addr, uint8_t val) {
		}
	};

	struct All1MemoryArea : public MemoryArea {
		using MemoryArea::MemoryArea;

		virtual uint8_t peek(uint16_t addr) {
			return 0xff;
		}

		virtual void poke(uint16_t addr, uint8_t val) {
		}
	};

	struct SplitNibbleMemoryArea : public MemoryArea {
		using MemoryArea::MemoryArea;

		virtual uint8_t peek(uint16_t addr) {
			return (m_data[addr * 2] & 0xf) | ((m_data[addr * 2 + 1] & 0xf) << 4);
		}

		virtual void poke(uint16_t addr, uint8_t val) {
			m_data[addr * 2 + 1] = val >> 4;
			m_data[addr * 2] = val & 0xf;
		}
	};

	// reads from primary, writes to primary & secondary
	// allows 2 blocks of memory with diffent layouts
	// appear to be the same block of memory
	struct DualMemoryArea : public IMemoryArea {
	   private:
		IMemoryArea* m_primary;
		IMemoryArea* m_secondary;

	   public:
		DualMemoryArea(IMemoryArea* primary, IMemoryArea* secondary)
		    : m_primary(primary), m_secondary(secondary) {
		}

		virtual uint16_t address() const {
			return m_primary->address();
		}
		virtual uint16_t size() const {
			return m_primary->size();
		}

		virtual uint8_t peek(uint16_t addr) {
			return m_primary->peek(addr);
		}

		virtual void poke(uint16_t addr, uint8_t val) {
			m_primary->poke(addr, val);
			m_secondary->poke(addr, val);
		}
	};

	class RAM {
	   private:
		std::array<IMemoryArea*, 256> m_pages;

	   public:
		RAM();
		void addMemoryArea(IMemoryArea* area);
		uint8_t peek(uint16_t addr);
		void poke(uint16_t addr, uint8_t val);
		void dump(uint16_t from, uint16_t len);
	};
}  // namespace pico_ram

#endif /* PICO_MEMORY_H */
