#include "CPU.h"
#include "helper.h"

size_t getFileSize(std::ifstream *file) {
	if (file->is_open() == false) 
		return 0; 

	size_t orig_pos = file->tellg();

	file->seekg(0, std::ios::end);
	size_t ret = file->tellg();
	file->seekg(orig_pos);

	return ret; 
}

void gameBoy::keyEvent(int key, int action) {
	const int KEY_UP = 265;
	const int KEY_DOWN = 264;
	const int KEY_LEFT = 263;
	const int KEY_RIGHT = 262;
	const int KEY_A = 88;
	const int KEY_B = 90;
	const int KEY_SELECT = 259;
	const int KEY_START = 257;

	if (action == GLFW_PRESS) {
		if (key == KEY_RIGHT) m_registers.joypadDir &= (0xFF ^ 0x1); 
		if (key == KEY_LEFT) m_registers.joypadDir &= (0xFF ^ 0x2);
		if (key == KEY_UP) m_registers.joypadDir &= (0xFF ^ 0x4); 
		if (key == KEY_DOWN) m_registers.joypadDir &= (0xFF ^ 0x8); 	

		if (key == KEY_A) m_registers.joypadButt &= (0xFF ^ 0x1); 
		if (key == KEY_B) m_registers.joypadButt &= (0xFF ^ 0x2);
		if (key == KEY_SELECT) m_registers.joypadButt &= (0xFF ^ 0x4); 
		if (key == KEY_START) m_registers.joypadButt &= (0xFF ^ 0x8); 					
	} else if (action == GLFW_RELEASE) {
		if (key == KEY_RIGHT) m_registers.joypadDir |= 0x1; 
		if (key == KEY_LEFT) m_registers.joypadDir |= 0x2;
		if (key == KEY_UP) m_registers.joypadDir |= 0x4; 
		if (key == KEY_DOWN) m_registers.joypadDir |= 0x8; 

		if (key == KEY_A) m_registers.joypadButt |= 0x1; 
		if (key == KEY_B) m_registers.joypadButt |= 0x2;
		if (key == KEY_SELECT) m_registers.joypadButt |= 0x4; 
		if (key == KEY_START) m_registers.joypadButt |= 0x8; 
	}

	// joypad interrupt 
	m_registers.IF |= (1 << 4);
}

void gameBoy::dump() {
	printf("A: %02X B: %02X C: %02X\n", m_registers.A, m_registers.B, m_registers.C);
	printf("D: %02X E: %02X H: %02X\n", m_registers.D, m_registers.E, m_registers.H);
	printf("L: %02X\n", m_registers.L);
	printf("SP: %04X PC: %04X\n", m_registers.SP, m_registers.PC);
	printf("\n");
	printf("Flags:\n");
	printf("Z: %d N: %d H: %d C: %d\n", m_registers.m_flags.Z, m_registers.m_flags.N,
		m_registers.m_flags.H, m_registers.m_flags.C);
	printf("Last instruction: %s\n", last.c_str());
}

gameBoy::~gameBoy() {
	dump();
}

gameBoy::gameBoy() {
	gpu = new GPU(this);
}

uint8_t gameBoy::io_port(uint16_t addr, bool write, uint8_t value) {
	const static int sendList[] = {
		0xFF40,0xFF41,0xFF42,0xFF43,0xFF44,0xFF45,
		0xFF4A,0xFF4B,0xFF47,0xFF48,0xFF49,0xFF4F,
		0xFF68,0xFF69,0xFF6A,0xFF6B,0xFF4C
	};

	bool found = 0;
	for (int laddr : sendList) {
		if (addr == laddr) {
			found = 1; 
			break; 
		}
	}

	if (found == 1) {
		if (write)
			gpu->write(addr, value);
		else 
			return gpu->read(addr);
		return 0; 
	}

	if (addr == 0xFF50 && write == true && value != 0) {
		m_registers.inBios = false;
	}

	if (addr == 0xFF00) { // joypad 
		if (write) {
			m_registers.joypad = value & (3 << 4);
		}
		else {
			uint8_t ret = 0xF; 
			if ((m_registers.joypad & (1 << 4)) == 0) 
				ret &= m_registers.joypadDir;
			if ((m_registers.joypad & (1 << 5)) == 0)
				ret &= m_registers.joypadButt;
			return m_registers.joypad | ret;
		}
	}

	if (addr == 0xFF46) {
		if (value > 0xF1) value = 0xF1; 
		uint16_t source = (value << 8); 
		uint16_t dest = 0xFE00; 

		for (int i = 0; i < 0xA0; i++) {
			write_byte(dest, read_byte(source)); 
			dest++; 
			source++;
		}

		last_cycles += 20;

		return 0;
	}

	static uint8_t DMAsrc_high = 0;
	static uint8_t DMAsrc_low = 0;
	static uint8_t DMAdest_high = 0;
	static uint8_t DMAdest_low = 0;
	uint16_t src, dest, sz; 

	switch (addr) {
		case 0xFF51:
			if (write)
				DMAsrc_high = value; 
			else return DMAsrc_high;
			break; 
		case 0xFF52:
			if (write)
				DMAsrc_low = value & 0xF0; 
			else return DMAsrc_low;
			break;
		case 0xFF53:
			if (write)
				DMAdest_high = value; 
			else return DMAdest_high;
			break;  
		case 0xFF54:
			if (write)
				DMAdest_low = value & 0xF0;
			else return DMAsrc_low;
			break; 
		case 0xFF55:
			src = (DMAsrc_high << 8) | DMAsrc_low;
			dest = (DMAdest_high << 8) | DMAdest_low;
			sz = ((value & 0x7) + 1) * 0x10;
			for (int i = 0; i < sz; i++)
				write_byte(dest++, read_byte(src++));
			if (!write)
				return 0; 
			break; 
	}
	/*
		CGB only: FF70
	*/
	const static int unimplementedList[] = {0xFF01, 0xFF02, 
		0xFF40, 0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF47, 0xFF48,
		0xFF49, 0xFF4A, 0xFF4B, 0xFF4D, 0xFF4F, 0xFF56, 0xFF68, 0xFF69, 0xFF6A, 0xFF6B
	}; 
	found = 0;
	for (int laddr : unimplementedList) {
		if (addr == laddr) {
			found = 1; 
			break; 
		} 
	}

	if (found && m_registers.inBios == false) {
		if (write) 
			printf("Trying to write to %04X (%d)\n", addr, value); 
		else 
			printf("Trying to read from %04X\n", addr); 
	}

	if (addr == 0xFF04) {
		if (write) 
			m_registers.DIV = 0;
		else 
			return m_registers.DIV;
	}

	if (addr >= 0xFF04 && addr <= 0xFF07) {
		if (write)
			printf("Timer write..(%04X) (%02X)\n", addr, value);
		else 
			printf("Timer read...(%04X)\n", addr);
	}
	
	if (addr == 0xFF0F) {
		if (!write) 
			return m_registers.IF; 
		else 
			m_registers.IF = value;
		return 0; 
	}

	if (addr == 0xFF70 && gpu->m_registers.GBMode != 1) {
		if (write == false) return m_registers.WRAMBank;
		else {
			if (value == 0) value = 1; 
			m_registers.WRAMBank = value; 
		}
	} else if (addr == 0xFF47) {
		// 0: white, 1: black, 2: black, 3: black 
		if (write) {
			m_registers.shades[0] = value & 0x3;
			m_registers.shades[1] = (value & 0xC) >> 2; 
			m_registers.shades[2] = (value & 0x30) >> 4; 
			m_registers.shades[3] = (value & 0xC0) >> 6; 
		} else {
			return (m_registers.shades[0]) | 
						 (m_registers.shades[1] << 2) | 
						 (m_registers.shades[2] << 4) | 
						 (m_registers.shades[3] << 6);
		}
	} else {
	}

	return 0;
}

void gameBoy::tickTimer() {
	if (bitSet(m_registers.Clock.DH, 6)) return;

	m_registers.Clock.S++; 
	
	if (m_registers.Clock.S > 59) {
		m_registers.Clock.S %= 60; 
		m_registers.Clock.M++;
	}	

	if (m_registers.Clock.M > 59) {
		m_registers.Clock.M %= 60; 
		m_registers.Clock.H++;		
	}

	if (m_registers.Clock.H > 23) {
		m_registers.Clock.H %= 24; 
		m_registers.Clock.DL++;		

		if (m_registers.Clock.DL == 0) {
			if (bitSet(m_registers.Clock.DH, 0) == false)
				m_registers.Clock.DH |= 1; // MSB of day
			else 
				m_registers.Clock.DH |= (1 << 7); // set overflow
		}
	}	
}

uint8_t gameBoy::read_byte(uint16_t addr) {
	static struct MBCInfo info = getMBCInfo(m_registers.type); 

	if (info.MBCType == TYPE_ROM_ONLY) {
		switch (addr & 0xF000) {
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return m_rom[addr & 0x7FFF]; 
				break; 

			case 0xA000:
			case 0xB000:
				return m_eram[addr & 0x1FFF];
				break;
		}
	}

	if (info.MBCType == TYPE_MBC1) {
		switch (addr & 0xF000) {
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return m_rom[(addr - 0x4000) + (m_registers.ROMBank * 0x4000)]; 
				break; 

			case 0xA000:
			case 0xB000:
				return m_eram[(addr - 0xA000) + (m_registers.RAMBank * 0x2000)]; 
		}
	}

	if (info.MBCType == TYPE_MBC3) {
		switch (addr & 0xF000) {
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				return m_rom[(addr-0x4000) + (m_registers.ROMBank * 0x4000)]; 
				break; 

			case 0xA000:
			case 0xB000:
				if (m_registers.BankMode == 1) {
					return m_eram[(addr - 0xA000) + (m_registers.RAMBank * 0x2000)]; 
				} else {
					switch (m_registers.ClockRegister) {
						last_cycles += 20;
						case 0x8:
							return m_registers.LatchedClock.S;
							break;
						case 0x9:
							return m_registers.LatchedClock.M;
							break;
						case 0xA:
							return m_registers.LatchedClock.H;
							break;
						case 0xB:
							return m_registers.LatchedClock.DL;
							break;
						case 0xC:
							return m_registers.LatchedClock.DH;
							break;
					}
				}
				break; 
		}
	}

	switch (addr & 0xF000) {
		case 0x0000:
			if (m_registers.inBios == true) {
				if (addr < 0x100 || (addr >= 0x200 && addr < 0x900)) {
					return m_bios[addr]; 
					break;
				}
			}
		case 0x1000:
		case 0x2000:
		case 0x3000:
			return m_rom[addr & 0x3FFF]; 
			break;

		case 0x8000:
		case 0x9000:
			return m_vram[m_registers.VRAMBank][addr & 0x1FFF];
			break; 

		case 0xC000:
		case 0xE000:
			return m_wramb[addr & 0xFFF][0];
			break; 

		case 0xD000:
			return m_wramb[addr & 0xFFF][1];
			break; 				

		case 0xF000: 
			if (addr < 0xFE00) {
				return m_wramb[addr & 0xFFF][1];
				break; 				
			}			
			switch (addr & 0xF00) {
				case 0xE00:
					if (addr < 0xFEA0)
						return m_oam[addr & 0xFF];
					break; 
				case 0xF00:
					if (addr < 0xFF80) 
						return io_port(addr, false, 0);
					else if (addr < 0xFFFF)
						return m_hram[addr & 0x7F];
					else 
						return m_registers.IE; 
					break; 
			}
			break; 
	}	

	printf("Unhandled address %04X\n", addr);
	return 0;
}

void gameBoy::write_byte(uint16_t addr, uint8_t value) {
	static struct MBCInfo info = getMBCInfo(m_registers.type); 
	if (info.MBCType == TYPE_ROM_ONLY) {
		switch (addr & 0xF000) {
			case 0xA000:
			case 0xB000:
				if (info.hasBattery == true)
					shouldSave = true;
				m_eram[addr & 0x1FFF] = value;
				return;
		}
	}

	if (info.MBCType == TYPE_MBC1) {
		switch (addr & 0xF000) {
			case 0x2000:
			case 0x3000:
				m_registers.ROMBank = (value & 0x1F);
				if (m_registers.ROMBank == 0) 
					m_registers.ROMBank = 1; 
				if (m_registers.ROMBank == 0x20) 
					m_registers.ROMBank = 0x21;
				if (m_registers.ROMBank == 0x40) 
					m_registers.ROMBank = 0x41;
				if (m_registers.ROMBank == 0x60) 
					m_registers.ROMBank = 0x61;
				return;

			case 0x4000:
			case 0x5000:
				if (m_registers.BankMode == 1) 
					m_registers.RAMBank = value & 3;
				else {
					m_registers.ROMBank |= (value << 5);

					if (m_registers.ROMBank == 0) 
						m_registers.ROMBank = 1; 
					if (m_registers.ROMBank == 0x20) 
						m_registers.ROMBank = 0x21;
					if (m_registers.ROMBank == 0x40) 
						m_registers.ROMBank = 0x41;
					if (m_registers.ROMBank == 0x60) 
						m_registers.ROMBank = 0x61;
				}
				return;

			case 0x6000:
			case 0x7000:
				m_registers.BankMode = value & 1; 
				return;

			case 0xA000:
			case 0xB000:
				if (info.hasBattery == true)
					shouldSave = true;
			 	m_eram[(addr - 0xA000) + (m_registers.RAMBank * 0x2000)] = value; 
			 	return;
		}
	}

	static bool beginLatch = false;
	if (info.MBCType == TYPE_MBC3) {
		switch (addr & 0xF000) {
			case 0x0000:
			case 0x1000:
				// Enable RAM and RTC. unnecessary though
				return; 

			case 0x2000:
			case 0x3000:
				m_registers.ROMBank = (value & 0x7F);
				if (m_registers.ROMBank == 0) 
					m_registers.ROMBank = 1; 
				return; 

			case 0x4000:
			case 0x5000:
				if (value <= 3) {
					m_registers.BankMode = 1; 
					m_registers.RAMBank = value; 
				} else if (value >= 0x8 && value <= 0xC) {
					m_registers.BankMode = 0;
					m_registers.ClockRegister = value;
				}
				return;

			case 0x6000:
			case 0x7000:
				if (beginLatch == false && value == 0x00) 
					beginLatch = true; 
				if (beginLatch == true && value == 0x01) {
					m_registers.LatchedClock = m_registers.Clock;
					beginLatch = false;
				}
				return;

			case 0xA000:
			case 0xB000:
				if (m_registers.BankMode == 1) {
					if (info.hasBattery == true)
						shouldSave = true;
					m_eram[(addr - 0xA000) + (m_registers.RAMBank * 0x2000)] = value;
				} else {
					switch (m_registers.ClockRegister) {
						case 0x8:
							m_registers.Clock.S = value;
							break;
						case 0x9:
							m_registers.Clock.M = value;
							break;
						case 0xA:
							m_registers.Clock.H = value;
							break;
						case 0xB:
							m_registers.Clock.DL = value;
							break;
						case 0xC:
							m_registers.Clock.DH = value;
							break;
					}
				}
				return; 
		}
	}

	switch (addr & 0xF000) {
		case 0x8000:
		case 0x9000:
			m_vram[m_registers.VRAMBank][addr & 0x1FFF] = value;
			return;
			break;

		case 0xC000:
			m_wramb[addr & 0xFFF][0] = value;
			return;
			break; 

		case 0xD000:
			m_wramb[addr & 0xFFF][1] = value;
			return;
			break; 				

		case 0xE000:
			m_wramb[addr & 0xFFF][0] = value;
			return;
			break; 

		case 0xF000: 
			if (addr < 0xFE00) {
				m_wramb[addr & 0xFFF][1] = value;
				return;
				break; 				
			}			
			switch (addr & 0xF00) {
				case 0xE00:
					if (addr < 0xFEA0)
						m_oam[addr & 0xFF] = value;
					return;	
					break; 
				case 0xF00:
					if (addr < 0xFF80) 
						io_port(addr, true, value);
					else if (addr < 0xFFFF)
						m_hram[addr & 0x7F] = value;
					else {
						m_registers.IE = value;
					}
					return;
					break; 
			}
			break; 
	}

	printf("Unhandled write address %04X\n", addr);
}

uint16_t gameBoy::read_word(uint16_t addr) {
	return (read_byte(addr + 1) << 8) | read_byte(addr);
}

void gameBoy::write_word(uint16_t addr, uint16_t value) {
	write_byte(addr + 1, value >> 8);
	write_byte(addr, value & 0xFF); 
}

uint8_t gameBoy::get_byte() {
#ifdef TESTF
	return m_registers.tbytes[m_registers.testingOffset++]; 
#else 
	uint8_t ret = read_byte(m_registers.PC); 
	m_registers.PC++;
	return ret;
#endif  
}

uint16_t gameBoy::get_word() {
#ifdef TESTF
	return (m_registers.tbytes[1] << 8) | (m_registers.tbytes[0]);
#else 
	uint16_t ret = read_word(m_registers.PC);
	m_registers.PC += 2; 
	return ret; 
#endif
}

uint8_t gameBoy::getParam(uint8_t opcode, uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t H, 
	uint8_t L, uint8_t HL, uint8_t n) 
{
	uint8_t param = 0;

	last_cycles = 4; 

			 if (opcode == A) 	{ param = m_registers.A; }
	else if (opcode == B) 	{ param = m_registers.B; }
	else if (opcode == C)  	{	param = m_registers.C; }
	else if (opcode == D)  	{ param = m_registers.D; }
	else if (opcode == E)  	{ param = m_registers.E; }
	else if (opcode == H) 	{ param = m_registers.H; }
	else if (opcode == L) 	{ param = m_registers.L; }
	else if (opcode == HL) 	{ param = read_byte(m_registers.HL()); last_cycles = 8; }
	else if (opcode == n) 	{ param = get_byte(); last_cycles = 8; }

	return param; 
}

void gameBoy::save() {
	if (shouldSave == false) return; 
	std::ofstream saveFile(saveFileName, std::ofstream::out | std::ofstream::trunc);
	saveFile.write((const char *)m_eram, saveBytes);
	saveFile.close();
	shouldSave = false;
}


void gameBoy::reset(const char *bios_file, const char *rom_file) {
	saveFileName = (std::string)rom_file + ".sav"; 
	insmap.resize(0x100);

	last_cycles = 0; 
	std::ifstream bios(bios_file); 
	if (bios.is_open() == false) printf("Error: BIOS not found\n"); 
	size_t bios_sz = getFileSize(&bios);

	m_bios = new uint8_t[bios_sz];
	bios.read((char *)m_bios, bios_sz); 
	bios.close();

	std::ifstream rom(rom_file);
	if (rom.is_open() == false) printf("Error: ROM not found\n");
	size_t rom_sz = getFileSize(&rom);

	m_rom = new uint8_t[rom_sz];
	rom.read((char *)m_rom, rom_sz);
	rom.close();

	m_registers.reset();

	char *title = (char *)(m_rom + 0x0134);
	printf("Loaded %s\n", title);

	m_registers.type = m_rom[0x147];
	m_registers.romSz = m_rom[0x148]; 
	m_registers.ramSz = m_rom[0x149];

	printf("Cart Type: %02X\n", m_registers.type);
	printf("ROM Size: %02X\n", m_registers.romSz);

	// allocate space for external ram
	uint16_t sz = 0; 
	switch (m_registers.ramSz) {
		case 0: // None 
			sz = 0x0000; 
			break; 
		case 1: // 2 kB
			sz = 0x1000;  
			break; 
		case 2: // 8 kB
			sz = 0x2000; 
			break; 
		case 3: // 32 kB
			sz = 0x8000; 
			break; 
	}


	saveBytes = sz; 
	printf("Allocating $%04X bytes for ERAM\n", sz);
	m_eram = new uint8_t[sz];

	std::ifstream saveFile(saveFileName);
	if (saveFile.is_open()) {
		saveFile.read((char *)m_eram, saveBytes);
	}
	saveFile.close();

	uint8_t LD_opcodes[] = {
		0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x7F, 0x78, 0x79, 0x7A, 
		0x7B, 0x7C, 0x7D, 0x7E, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 
		0x46, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x50, 0x51, 
		0x52, 0x53, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 
		0x5D, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x68,
		0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 0x73,
		0x74, 0x75, 0x36, 0x7F, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D,
		0x0A, 0x1A, 0x7E, 0xFA, 0x3E, 0x7F, 0x47, 0x4F, 0x57, 0x5F,
		0x67, 0x6F, 0x02, 0x12, 0x77, 0xEA, 0xF2, 0xE2, 0x3A, 0x32, 
		0x2A, 0x22, 0xE0, 0xF0, 0x01, 0x11, 0x21, 0x31, 0xF9, 0x08,
		0x32, 0xF8
	};

	for (uint8_t code : LD_opcodes) 
		insmap[code] = &gameBoy::LD;

	uint8_t JP_opcodes[] = { 
		0xC2, 0xC3, 0xCA, 0xD2, 0xDA, 0xE9 
	};

	for (uint8_t code : JP_opcodes) 
		insmap[code] = &gameBoy::JP;

	uint8_t CALL_opcodes[] = { 
		0xC4, 0xCC, 0xCD, 0xD4, 0xDC
	};

	for (uint8_t code : CALL_opcodes) 
		insmap[code] = &gameBoy::CALL;

	uint8_t AND_opcodes[] = { 
		0xA7, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xE6
	};

	for (uint8_t code : AND_opcodes) 
		insmap[code] = &gameBoy::AND;

	uint8_t OR_opcodes[] = { 
		0xB7, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xF6
	};

	for (uint8_t code : OR_opcodes) 
		insmap[code] = &gameBoy::OR;

	uint8_t ADC_opcodes[] = { 
		0x8F, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0xCE
	};

	for (uint8_t code : ADC_opcodes) 
		insmap[code] = &gameBoy::ADC;

	uint8_t SBC_opcodes[] = { 
		0x9F, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9E,
		0xDE
	};

	for (uint8_t code : SBC_opcodes) 
		insmap[code] = &gameBoy::SBC;

	insmap[0x3F] = &gameBoy::CCF;

	uint8_t CP_opcodes[] = { 
		0xBF, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xFE
	};

	for (uint8_t code : CP_opcodes) 
		insmap[code] = &gameBoy::CP;		

	uint8_t SUB_opcodes[] = { 
		0x97, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0xD6
	};

	for (uint8_t code : SUB_opcodes) 
		insmap[code] = &gameBoy::SUB;			

	uint8_t ADD_opcodes[] = { 
		0x09, 0x19, 0x29, 0x39, 0x80, 0x81, 0x82, 0x83, 0x84,
		0x85, 0x86, 0x87, 0xC6, 0xC8, 0xE8
	};

	for (uint8_t code : ADD_opcodes) 
		insmap[code] = &gameBoy::ADD;		

	uint8_t RST_opcodes[] = { 
		0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF
	};

	for (uint8_t code : RST_opcodes) 
		insmap[code] = &gameBoy::RST;		

	insmap[0x00] = &gameBoy::NOP;

	uint8_t XOR_opcodes[] = { 
		0xAF, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xEE
	};

	for (uint8_t code : XOR_opcodes) 
		insmap[code] = &gameBoy::XOR;		

	uint8_t DEC_opcodes[] = { 
		0x05, 0x0B, 0x0D, 0x15, 0x1B, 0x1D, 0x25, 0x2B, 0x2D,
		0x35, 0x3B, 0x3D
	};

	for (uint8_t code : DEC_opcodes) 
		insmap[code] = &gameBoy::DEC;		

	uint8_t INC_opcodes[] = { 
		0x03, 0x04, 0x0C, 0x13, 0x14, 0x1C, 0x23, 0x24, 0x2C,
		0x33, 0x34, 0x3C
	};

	for (uint8_t code : INC_opcodes) 
		insmap[code] = &gameBoy::INC;		


	uint8_t JR_opcodes[] = { 
		0x18, 0x20, 0x28, 0x30, 0x38
	};

	for (uint8_t code : JR_opcodes) 
		insmap[code] = &gameBoy::JR;		

	uint8_t RET_opcodes[] = { 
		0xC0, 0xC8, 0xC9, 0xD0, 0xD8, 0xD9
	};

	for (uint8_t code : RET_opcodes) 
		insmap[code] = &gameBoy::RET;		

	uint8_t PUSH_opcodes[] = { 
		0xC5, 0xD5, 0xE5, 0xF5
	};

	for (uint8_t code : PUSH_opcodes) 
		insmap[code] = &gameBoy::PUSH;		

	uint8_t POP_opcodes[] = { 
		0xC1, 0xD1, 0xE1, 0xF1
	};

	for (uint8_t code : POP_opcodes) 
		insmap[code] = &gameBoy::POP;		

	uint8_t ROT_opcodes[] = { 
		0x07, 0x0F, 0x17, 0x1F
	};

	for (uint8_t code : ROT_opcodes) 
		insmap[code] = &gameBoy::ROT;		

	insmap[0xCB] = &gameBoy::CB;
	insmap[0x2F] = &gameBoy::CPL;
	insmap[0x27] = &gameBoy::DAA;
	insmap[0x37] = &gameBoy::SCF;
	insmap[0xF3] = &gameBoy::DI;
	insmap[0xFB] = &gameBoy::EI;
	insmap[0x10] = &gameBoy::STOP;
	insmap[0x76] = &gameBoy::STOP;

	m_registers.inBios = false;
	m_registers.PC = 0x100;
}

bool debugIt = false;

void gameBoy::ins() {
	if (m_registers.halted == false) {
		uint8_t opcode = get_byte(); 

		try {
			insmap[opcode](this, opcode);
		} catch (std::exception& e) {
			printf("Error: opcode %02X not implemented.\n", opcode);
			exit(0);
		}
	}

	m_registers.DIV += (last_cycles ? last_cycles : 1);

	if (m_registers.IME == 1) {
		uint8_t interrupts = m_registers.IF & m_registers.IE; 

		// vblank interrupt 
		if (bitSet(interrupts, 0)) {
			m_registers.IF &= (0xFF ^ (1 << 0));
			interrupt(0x0040); 
		} 

 		// STAT interrupt 
		if (bitSet(interrupts, 1)) {
			m_registers.IF &= (0xFF ^ (1 << 1));
			interrupt(0x0048); 
		}

		// timer interrupt 
		if (bitSet(interrupts, 2)) {
			m_registers.IF &= (0xFF ^ (1 << 2));
			interrupt(0x0050); 
		}

		// serial interrupt 	
		if (bitSet(interrupts, 3)) {
			m_registers.IF &= (0xFF ^ (1 << 3));
			interrupt(0x0058); 
		}

		// joypad interrupt 
		if (bitSet(interrupts, 4)) {
			m_registers.IF &= (0xFF ^ (1 << 4)); 
			interrupt(0x0060);
		}
	} 
}