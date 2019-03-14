
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico_memory.h"

namespace pico_ram {

	RAM::RAM() {
		m_pages.fill(nullptr);
	}

	void RAM::addMemoryArea(IMemoryArea* area) {
		for (int32_t i = 0; i < (int32_t)(area->size() / 256); i++) {
			m_pages[(area->address() >> 8) + i] = area;
		}
	}

	uint8_t RAM::peek(uint16_t addr) {
		auto area = m_pages[addr >> 8];
		if (area == nullptr) {
			return 0;
		}
		return area->peek(addr - area->address());
	}

	void RAM::poke(uint16_t addr, uint8_t val) {
		auto area = m_pages[addr >> 8];

		if (area) {
			area->poke(addr - area->address(), val);
		}
	}

	void RAM::dump(uint16_t from, uint16_t len) {
		int count = 0;
		for (uint16_t i = 0; i < len; i++) {
			if (count == 0) {
				printf("%04x : ", from + i);
			}
			printf("%02x ", peek(from + i));
			count++;
			if (count == 32) {
				printf("\n");
				count = 0;
			}
		}
	}

}  // namespace pico_ram