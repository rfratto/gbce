#include "CPU.h"

const char *open = "log/results" ".log";
const char *params[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A" };
const char *aparams[] = {"BC", "DE", "HL", "SP"};

FILE *logfile = NULL; 

int printd(gameBoy *cpu, const char *str, ...) {
	char *cstr = NULL;
	va_list list1; 
	va_start(list1, str);
	vasprintf(&cstr, str, list1);
	va_end(list1);
	cpu->last = "";
	cpu->last = (std::string)cstr;
	delete cstr; 
#if defined(DEBUG) 
	va_list list2; 
	static FILE *f = fopen(open, "w");
	if (!f) {
		printf("ERROR WITH FILE.\n");
		return 0;
	}

	va_start(list2, str);
	vfprintf(f, str, list2);
	va_end(list2);
#endif 
	return 0;
}

void gameBoy::NOP(uint8_t opcode) {
	printd(this, "($%04X) ($%02X) NOP\n", m_registers.PC - 1, opcode);
	last_cycles += 4; 	
}

void gameBoy::STOP(uint8_t opcode) {
	if (opcode == 0x10) {
		printd(this, "($%04X) ($%02X) STOP\n", m_registers.PC - 1, opcode);
	} else {
		printd(this, "($%04X) ($%02X) HALT\n", m_registers.PC - 1, opcode);
		m_registers.halted = true;
	} 	
	last_cycles += 4;
}

void gameBoy::RST(uint8_t opcode) {
	uint8_t off = opcode - 0xC7;
	last_cycles += 4;

	printd(this, "($%04X) ($%02X) RST $%02x\n", m_registers.PC - 1, opcode, off);

	m_registers.SP -= 2;
	write_word(m_registers.SP, m_registers.PC); 

	last_cycles += 32; 
	m_registers.PC = off;  
}

bool inRange(int v, int min, int max) {
	return (v >= min && v <= max);
} 

void gameBoy::SCF(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	m_registers.m_flags.C = 1;
	m_registers.m_flags.H = 0;
	m_registers.m_flags.N = 0;
	last_cycles += 4; 
	printd(this, "($%04X) ($%02X) SCF\n", startPC, opcode);
}

void gameBoy::DAA(uint8_t opcode) { // opcode $27 
	uint16_t startPC = m_registers.PC - 1;
	uint8_t hnib = (m_registers.A >> 4) & 0x0F; 
	uint8_t lnib = m_registers.A & 0x0F; 

	bool ran = false;
	int v = (m_registers.m_flags.C << 1) | m_registers.m_flags.H; 

	if (m_registers.m_flags.N == 0) {
		switch (v) {
			case 0:
				if (inRange(hnib, 0x0, 0x9) && inRange(lnib, 0x0, 0x9)) {
					m_registers.A += 0x00;
					m_registers.m_flags.C = 0;
					ran = true;
					break;
				}
				if (inRange(hnib, 0x0, 0x8) && inRange(lnib, 0xA, 0xF)) {
					m_registers.m_flags.C = 0;
					m_registers.A += 0x06; 
					ran = true;
					break;
				}
				if (inRange(hnib, 0xA, 0xF) && inRange(lnib, 0x0, 0x9)) {
					m_registers.A += 0x60;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				if (inRange(hnib, 0x9, 0xF) && inRange(lnib, 0xA, 0xF)) {
					m_registers.A += 0x66;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				break;
			case 1:
				if (inRange(hnib, 0x0, 0x9) && inRange(lnib, 0x0, 0x9)) {
					m_registers.A += 0x06;
					m_registers.m_flags.C = 0; 
					ran = true;
					break;
				}			
				if (inRange(hnib, 0x0, 0x8) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0x06;
					m_registers.m_flags.C = 0; 
					ran = true;
					break;
				}
				if (inRange(hnib, 0x9, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0x66;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				break;				
			case 2:
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0x9)) {
					m_registers.A += 0x60;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				else if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0xA, 0xF)) {
					m_registers.A += 0x66;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				break;
			case 3:
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0x66;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				break;
		} 
	} else {
		switch (v) {
			case 0:
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0;
					m_registers.m_flags.C = 0; 
					ran = true;
					break;
				}			
				break;
			case 1:
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0xFA;
					m_registers.m_flags.C = 0; 
					ran = true;
					break;
				}			
				break;
			case 2:
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0xA0;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
			 	break;
			case 3: 
				if (inRange(hnib, 0x0, 0xF) && inRange(lnib, 0x0, 0xF)) {
					m_registers.A += 0x9A;
					m_registers.m_flags.C = 1; 
					ran = true;
					break;
				}			
				break;
		}
	}

	last_cycles += 4; 
	m_registers.m_flags.H = 0; 
	m_registers.m_flags.Z = (m_registers.A == 0);
	printd(this, "($%04X) ($%02X) DAA\n", startPC, opcode);
} 

void gameBoy::INC(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint8_t lnib = opcode & 0x0F; 

	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	if (lnib != 3) {
		uint8_t *param = map[(opcode & 0x38) >> 3];
		int carry = 0;
		int res = 0;

		if (param) {
			res = *param + 1; 
			carry = res ^ *param ^ 1; 

			*param = res & 0xFF;

			last_cycles += 4; 
		} else {
			res = read_byte(m_registers.HL()) + 1; 
			carry = res ^ read_byte(m_registers.HL()) ^ 1; 
			write_byte(m_registers.HL(), res & 0xFF);

			last_cycles += 8;
		}

		m_registers.m_flags.Z = ((res & 0xFF) == 0);
		m_registers.m_flags.H = ((carry & 0x10) > 0);
		m_registers.m_flags.N = 0;

		printd(this, "($%04X) ($%02X) INC %s\n", startPC, opcode, params[(opcode & 0x38) >> 3]);
	} else {
		last_cycles += 8; 

		int carry = 0, res = 0;

		switch (opcode) {
			case 0x03: // INC BC
				res = m_registers.BC() + 1; 
				carry = res ^ m_registers.BC() ^ 1;
				m_registers.BC(res & 0xFFFF);
				printd(this, "($%04X) ($%02X) INC BC\n", startPC, opcode);
				break; 
			case 0x13: // INC DE
				res = m_registers.DE() + 1; 
				carry = res ^ m_registers.BC() ^ 1;
				m_registers.DE(res & 0xFFFF);
				printd(this, "($%04X) ($%02X) INC DE\n", startPC, opcode);
				break; 
			case 0x23: // INC HL
				res = m_registers.HL() + 1; 
				carry = res ^ m_registers.HL() ^ 1;
				m_registers.HL(res & 0xFFFF);
				printd(this, "($%04X) ($%02X) INC HL\n", startPC, opcode);
				break; 
			case 0x33: // INC SP 
				res = m_registers.SP + 1; 
				carry = res ^ m_registers.SP ^ 1;
				m_registers.SP = res & 0xFFFF;
				printd(this, "($%04X) ($%02X) INC SP\n", startPC, opcode);
				break; 
		}
	}
}


void gameBoy::DEC(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0x0F; 

	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	int res = 0; 
	if (lnib != 0x0B) {
		uint8_t off = (lnib == 0xD); 
		uint8_t *t = map[(hnib * 2) + off]; 

		res = (t ? *t : read_byte(m_registers.HL())) - 1; 

		last_cycles += (!t ? 12 : 4);

		if (t) 
			*t = (uint8_t)res;
		else 
			write_byte(m_registers.HL(), (uint8_t)res); 

		printd(this, "($%04X) ($%02X) DEC %s\n",  startPC, opcode, params[(hnib * 2) + off]);			

		m_registers.m_flags.Z = (res == 0);
		m_registers.m_flags.N = 1;
		m_registers.m_flags.H = ((res ^ (uint8_t)(-1) ^ (res+1)) & 0x10) == 0; 
	}	else {	
		const char *lparam[] = {"BC", "DE", "HL", "SP"}; 
		uint16_t map2[] = {m_registers.BC(), m_registers.DE(), m_registers.HL(), m_registers.SP};
		int res = map2[hnib] - 1; 

		uint8_t *h = map[(hnib * 2)];
		uint8_t *l = map[(hnib * 2) + 1];

		last_cycles += 8;

		if (h && l) {
			*h = (res >> 8) & 0xFF;
			*l = res & 0xFF; 
		} else {
			m_registers.SP = res; 
		}

		printd(this, "($%04X) ($%02X) DEC %s\n",  startPC, opcode, lparam[hnib]);		
	}
}

void gameBoy::CPL(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 

	m_registers.m_flags.N = 1;
	m_registers.m_flags.H = 1;

	m_registers.A ^= 0xFF; 

	last_cycles += 4; 

	printd(this, "($%04X) ($%02X) CPL\n",  startPC, opcode);		
}

void gameBoy::XOR(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 

	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint8_t param = 0;

	last_cycles += 4; 
	if ((opcode & 0xF0) == 0xA0) {
		uint8_t *target = map[(opcode & 0xF) % 8]; 
		if (!target) last_cycles += 8; 
		param = (target ? *target : read_byte(m_registers.HL()));

		printd(this, "($%04X) ($%02X) XOR %s\n",  startPC, opcode, params[(opcode & 0xF) % 8]);		
	}
	else { // XOR = n 
		last_cycles += 8; 
		param = get_byte();		
		printd(this, "($%04X) ($%02X) XOR $%02X\n",  startPC, opcode, param);		
	}

	m_registers.A ^= param; 

	m_registers.m_flags.Z = (m_registers.A == 0); 
	m_registers.m_flags.N = 0;
	m_registers.m_flags.H = 0;
	m_registers.m_flags.C = 0;
}

void gameBoy::ADD(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	if (hnib <= 3) { // 16 bit ADDS 
		uint16_t opmap[] = {m_registers.BC(), m_registers.DE(), m_registers.HL(), m_registers.SP};

		last_cycles += 8; 

		uint16_t param1 = m_registers.HL();  
		uint16_t param2 = opmap[hnib]; 
		uint32_t result = param1 + param2; 
		uint32_t carry  = result ^ param1 ^ param2; 

		m_registers.H = (result & 0xFF00) >> 8; 
		m_registers.L = result & 0xFF;  

		m_registers.m_flags.N = 0; 
		m_registers.m_flags.H = ((carry & 0x01000) > 0);
		m_registers.m_flags.C = ((carry & 0x10000) > 0);  

		printd(this, "($%04X) ($%02X) ADD HL, %s\n", startPC, opcode, aparams[hnib]);
	} else if (hnib == 8 || hnib == 0xC) {
		last_cycles += map[lnib] ? 4 : 8; 

		uint8_t  param1 = m_registers.A;
		uint8_t  param2 = map[lnib] ? *map[lnib] : read_byte(m_registers.HL()); 
		if (hnib == 0xC) param2 = get_byte();

		uint16_t result = param1 + param2; 
		uint16_t carry  = param1 ^ param2 ^ result; 

		m_registers.A = result & 0xFF; 

		m_registers.m_flags.Z = (m_registers.A == 0); 
		m_registers.m_flags.N = 0; 
		m_registers.m_flags.H = ((carry & 0x010) > 0); 
		m_registers.m_flags.C = ((carry & 0x100) > 0); 

		if (hnib == 8) 
			printd(this, "($%04X) ($%02X) ADD A, %s\n", startPC, opcode, params[lnib]);
		else 
			printd(this, "($%04X) ($%02X) ADD A, $%02X\n", startPC, opcode, param2);
	} else if (hnib == 0xE) {
		last_cycles += 16; 

		int8_t   param1 = get_byte(); 
		uint16_t param2 = m_registers.SP; 
		uint32_t result = param1 + param2;  
		uint32_t carry  = param1 ^ param2 ^ result; 

		m_registers.SP = result & 0xFFFF; 

		m_registers.m_flags.Z = 0;
		m_registers.m_flags.N = 0;
		m_registers.m_flags.H = ((carry & 0x00010) > 0);
		m_registers.m_flags.C = ((carry & 0x00100) > 0);  	

		printd(this, "($%04X) ($%02X) ADD SP, $%02X\n", startPC, opcode, param1);
	}
}

void gameBoy::SUB(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	last_cycles += map[lnib] ? 4 : 8; 

	uint32_t  param1 = m_registers.A;
	uint32_t  param2 = map[lnib] ? *map[lnib] : read_byte(m_registers.HL()); 
	if (hnib == 0xD) param2 = get_byte();

	uint32_t result = param1 - param2; 
	uint32_t carry  = param1 ^ param2 ^ result; 

	m_registers.A = result & 0xFF; 

	m_registers.m_flags.Z = (m_registers.A == 0); 
	m_registers.m_flags.N = 1; 
	m_registers.m_flags.H = ((carry & 0x010) > 0); 
	m_registers.m_flags.C = ((carry & 0x100) > 0); 

	if (hnib == 9) 
		printd(this, "($%04X) ($%02X) SUB A, %s\n", startPC, opcode, params[lnib]);
	else 
		printd(this, "($%04X) ($%02X) SUB A, $%02X\n", startPC, opcode, param2);
}

void gameBoy::CP(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	last_cycles += 4;

	uint8_t v = 0; 
	if (hnib == 0xB) {
		uint8_t *t = map[lnib % 8];
		last_cycles += (t ? 4 : 8);
		v = (t ? *t : read_byte(m_registers.HL()));

		printd(this, "($%04X) ($%02X) CP %s\n",  startPC, opcode, params[lnib % 8]);		
	} else {
		last_cycles += 8;
		v = get_byte();

		printd(this, "($%04X) ($%02X) CP $%02X\n",  startPC, opcode, v);		
	}

	int res = m_registers.A - v; 
	int carry = res ^ m_registers.A ^ v; 

	m_registers.m_flags.Z = (res == 0);
	m_registers.m_flags.N = 1;
	m_registers.m_flags.H = ((carry & 0x010) > 0); 
	m_registers.m_flags.C = ((carry & 0x100) > 0);

	return;
}


void gameBoy::DI(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	m_registers.IME = 0; 
	last_cycles += 4;
	printd(this, "($%04X) ($%02X) DI\n", startPC, opcode);
}

void gameBoy::EI(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	m_registers.IME = 1;
	last_cycles += 4;
	printd(this, "($%04X) ($%02X) EI\n", startPC, opcode);
}

void gameBoy::CCF(uint8_t opcode) {
		uint16_t startPC = m_registers.PC - 1; 

	if (opcode == 0x3F) {
		m_registers.m_flags.H = 0;
		m_registers.m_flags.C ^= 1;
		m_registers.m_flags.N = 0;
		last_cycles += 4; 

		printd(this, "($%04X) ($%02X) CCF\n", startPC, opcode);
	} else {
		m_registers.m_flags.H = 0;
		m_registers.m_flags.C = 0;
		m_registers.m_flags.N = 0;
		last_cycles += 4; 

		printd(this, "($%04X) ($%02X) CCF\n", startPC, opcode);
	}
}

void gameBoy::SBC(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	if (hnib == 9 || hnib == 0xD) {
		last_cycles += map[lnib % 8] ? 4 : 8; 

		uint8_t param1 = m_registers.A;
		uint8_t param2 = map[lnib % 8] ? *map[lnib % 8] : read_byte(m_registers.HL()); 
		uint8_t param3 = m_registers.m_flags.C;  
		if (hnib == 0xD) param2 = get_byte();

		uint16_t result = param1 - param2 - param3; 
		uint16_t carry  = param1 ^ param2 ^ param3 ^ result; 

		m_registers.A = result & 0xFF; 

		m_registers.m_flags.Z = (m_registers.A == 0); 
		m_registers.m_flags.N = 1; 
		m_registers.m_flags.H = ((carry & 0x010) > 0); 
		m_registers.m_flags.C = ((carry & 0x100) > 0); 
 
	 	if (hnib == 9) 
			printd(this, "($%04X) ($%02X) SBC A, %s\n", startPC, opcode, params[lnib % lnib]);
		else 
			printd(this, "($%04X) ($%02X) SBC A, $%02X\n", startPC, opcode, param2);
	} 	
}

void gameBoy::ADC(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	if (hnib == 8) {
		last_cycles += map[lnib % 8] ? 4 : 8; 

		uint8_t param1 = m_registers.A;
		uint8_t param2 = map[lnib % 8] ? *map[lnib % 8] : read_byte(m_registers.HL()); 
		uint8_t param3 = m_registers.m_flags.C;  

		uint16_t result = param1 + param2 + param3; 
		unsigned int carry  = param1 ^ param2 ^ param3 ^ result; 

		m_registers.A = result & 0xFF; 

		m_registers.m_flags.Z = (m_registers.A == 0); 
		m_registers.m_flags.N = 0; 
		m_registers.m_flags.H = ((carry & 0x010) > 0); 
		m_registers.m_flags.C = ((carry & 0x100) > 0); 

		printd(this, "($%04X) ($%02X) ADC A, %s\n", startPC, opcode, params[lnib % lnib]);
	} else {
		last_cycles += 8;

		uint8_t param1 = get_byte();
		uint8_t param2 = m_registers.A; 
		uint8_t param3 = m_registers.m_flags.C; 
		uint16_t res = param1 + param2 + param3;
		uint16_t carry = res ^ param1 ^ param2 ^ param3; 

		m_registers.A = res & 0xFF;

		m_registers.m_flags.Z = (m_registers.A == 0); 
		m_registers.m_flags.N = 0; 
		m_registers.m_flags.H = ((carry & 0x010) > 0); 
		m_registers.m_flags.C = ((carry & 0x100) > 0); 

		printd(this, "($%04X) ($%02X) ADC A, %02X\n", startPC, opcode, param1);
	}
}

void gameBoy::OR(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	last_cycles += 4;

	uint8_t v = 0; 
	if (hnib == 0xB) {
		uint8_t *t = map[lnib % 8];
		last_cycles += (t ? 4 : 8);
		v = (t ? *t : read_byte(m_registers.HL()));

		printd(this, "($%04X) ($%02X) OR %s\n",  startPC, opcode, params[lnib % 8]);		
	} else {
		last_cycles += 8;
		v = get_byte();

		printd(this, "($%04X) ($%02X) OR $%02X\n",  startPC, opcode,v);		
	}

	m_registers.A |= v; 

	m_registers.m_flags.Z = (m_registers.A == 0);
	m_registers.m_flags.N = 0;
	m_registers.m_flags.H = 0; 
	m_registers.m_flags.C = 0;

	return;
}

void gameBoy::AND(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 
	uint8_t lnib = opcode & 0xF; 

	last_cycles += 4;

	uint8_t v = 0; 
	if (hnib == 0xA) {
		uint8_t *t = map[lnib % 8];
		last_cycles += (t ? 4 : 8);
		v = (t ? *t : read_byte(m_registers.HL()));

		printd(this, "($%04X) ($%02X) AND %s\n",  startPC, opcode, params[lnib % 8]);		
	} else {
		last_cycles += 8;
		v = get_byte();

		printd(this, "($%04X) ($%02X) AND $%02X\n",  startPC, opcode, v);		
	}

	m_registers.A &= v; 

	m_registers.m_flags.Z = (m_registers.A == 0);
	m_registers.m_flags.N = 0;
	m_registers.m_flags.H = 1; 
	m_registers.m_flags.C = 0;

	return;
}

bool hasSavedFlags = false;
uint16_t savedFlags[4]; // AF, BC, DE, HL 
void gameBoy::interrupt(uint16_t addr) {
	m_registers.halted = false;
	m_registers.IME = 0;

	m_registers.SP -= 2;
	write_word(m_registers.SP, m_registers.PC); 

	savedFlags[0] = m_registers.AF();
	savedFlags[1] = m_registers.BC();
	savedFlags[2] = m_registers.DE();
	savedFlags[3] = m_registers.HL();
	hasSavedFlags = true;

	m_registers.PC = addr;  
}

void gameBoy::RET(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 

	bool shouldJump = false; 

	last_cycles += 8; 

	switch (opcode) {
		case 0xC0:
			shouldJump = (m_registers.m_flags.Z == 0); 
			printd(this, "($%04X) ($%02X) RET NZ\n",  startPC, opcode);		
			break; 
		case 0xC8:
			shouldJump = (m_registers.m_flags.Z == 1); 
			printd(this, "($%04X) ($%02X) RET Z\n",  startPC, opcode);		
			break;
		case 0xC9:
			shouldJump = true; 
			printd(this, "($%04X) ($%02X) RET\n",  startPC, opcode);		
			break;
		case 0xD0:
			shouldJump = (m_registers.m_flags.C == 0);
			printd(this, "($%04X) ($%02X) RET NC\n",  startPC, opcode);		
			break;
		case 0xD8:
			shouldJump = (m_registers.m_flags.C == 1);
			printd(this, "($%04X) ($%02X) RET C\n",  startPC, opcode);		
			break;
		case 0xD9: // RETI
			shouldJump = true; 
			printd(this, "($%04X) ($%02X) RETI\n",  startPC, opcode);		
			break;
	}

	if (shouldJump) {
		m_registers.PC = read_word(m_registers.SP);
		m_registers.SP += 2; 

		if (opcode == 0xD9) {
			m_registers.IME = 1; 

			if (hasSavedFlags == true) {
				m_registers.AF(savedFlags[0]);
				m_registers.BC(savedFlags[1]);
				m_registers.DE(savedFlags[2]);
				m_registers.HL(savedFlags[3]);
				hasSavedFlags = false;
			}
		}
	}
}


void gameBoy::CALL(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint16_t param = get_word();

	bool shouldJump = false; 

	last_cycles += 12; 

	switch (opcode) {
		case 0xC4:
			shouldJump = (m_registers.m_flags.Z == 0); 
			printd(this, "($%04X) ($%02X) CALL NZ, $%04X\n",  startPC, opcode, param);		
			break; 
		case 0xCC:
			shouldJump = (m_registers.m_flags.Z == 1); 
			printd(this, "($%04X) ($%02X) CALL Z, $%04X\n",  startPC, opcode, param);		
			break;
		case 0xCD:
			shouldJump = true; 
			printd(this, "($%04X) ($%02X) CALL $%04X\n",  startPC, opcode, param);		
			break;
		case 0xD4:
			shouldJump = (m_registers.m_flags.C == 0);
			printd(this, "($%04X) ($%02X) CALL NC, $%04X\n",  startPC, opcode, param);		
			break;
		case 0xDC:
			shouldJump = (m_registers.m_flags.C == 1);
			printd(this, "($%04X) ($%02X) CALL C, $%04X\n",  startPC, opcode, param);		
			break;
	}

	if (shouldJump) {
		m_registers.SP -= 2; 
		write_word(m_registers.SP, m_registers.PC);
		m_registers.PC = param;
	}
}

void gameBoy::POP(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 

	uint16_t val = read_word(m_registers.SP); 
	m_registers.SP += 2; 

	switch (opcode) {
		case 0xC1: 
			m_registers.BC(val);
			break;
		case 0xD1:
			m_registers.DE(val);
			break;
		case 0xE1:
			m_registers.HL(val);
			break;
		case 0xF1:
			m_registers.AF(val);
			break; 
	}

	last_cycles += 12;
	const char *lparam[] = {"BC", "DE", "HL", "AF"}; 
	printd(this, "($%04X) ($%02X) POP %s\n",  startPC, opcode, lparam[hnib - 0xC]);		
}

void gameBoy::PUSH(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint8_t hnib = opcode >> 4; 

	uint16_t map[] = {m_registers.BC(), m_registers.DE(), m_registers.HL(), m_registers.AF()}; 
	uint16_t val = map[hnib - 0xC]; 

	m_registers.SP -= 2; 
	write_word(m_registers.SP, val);
	last_cycles += 12;

	const char *lparam[] = {"BC", "DE", "HL", "AF"}; 
	printd(this, "($%04X) ($%02X) PUSH %s\n",  startPC, opcode, lparam[hnib - 0xC]);		
}

void gameBoy::JR(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	int8_t param = (int8_t)get_byte();

	bool shouldJump = false; 

	last_cycles += 8; 

	switch (opcode) {
		case 0x20:
			shouldJump = (m_registers.m_flags.Z == 0); 
			printd(this, "($%04X) ($%02X) JR NZ, %02X\n",  startPC, opcode, (uint8_t)param);		
			break; 
		case 0x18:
			shouldJump = true;
			printd(this, "($%04X) ($%02X) JR %02X\n",  startPC, opcode, (uint8_t)param);		
			break;
		case 0x28:
			shouldJump = (m_registers.m_flags.Z == 1); 
			printd(this, "($%04X) ($%02X) JR Z, %02X\n",  startPC, opcode, (uint8_t)param);		
			break;
		case 0x30:
			shouldJump = (m_registers.m_flags.C == 0);
			printd(this, "($%04X) ($%02X) JR NC, %02X\n",  startPC, opcode, (uint8_t)param);		
			break;
		case 0x38:
			shouldJump = (m_registers.m_flags.C == 1);
			printd(this, "($%04X) ($%02X) JR C, %02X\n",  startPC, opcode, (uint8_t)param);		
			break;
	}

	if (shouldJump) 
		m_registers.PC += param;

	return; 
}


void gameBoy::JP(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	uint16_t param = 0; 

	if (opcode != 0xE9)
		param = get_word();

	bool shouldJump = false; 

	last_cycles += 12; 

	switch (opcode) {
		case 0xC2:
			shouldJump = (m_registers.m_flags.Z == 0); 
			printd(this, "($%04X) ($%02X) JP NZ, %04X\n",  startPC, opcode, param);		
			break; 
		case 0xC3:
			shouldJump = true;
			printd(this, "($%04X) ($%02X) JP %04X\n",  startPC, opcode, param);		
			break;
		case 0xCA:
			shouldJump = (m_registers.m_flags.Z == 1); 
			printd(this, "($%04X) ($%02X) JP Z, %04X\n",  startPC, opcode, param);		
			break;
		case 0xD2:
			shouldJump = (m_registers.m_flags.C == 0);
			printd(this, "($%04X) ($%02X) JP NC, %04X\n",  startPC, opcode, param);		
			break;
		case 0xDA:
			shouldJump = (m_registers.m_flags.C == 1);
			printd(this, "($%04X) ($%02X) JP C, %04X\n",  startPC, opcode, param);		
			break;
		case 0xE9: 
			last_cycles += 4; 
			shouldJump = true; 

			param = m_registers.HL();
			printd(this, "($%04X) ($%02X) JP (HL)\n",  startPC, opcode);		
			break;
	}

	if (shouldJump) 
		m_registers.PC = param;

	return; 
}

void gameBoy::LD(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 

	uint8_t hnum = (opcode & 0xF0) >> 4; 
	uint8_t lnum = (opcode & 0x0F);  

	if ((opcode & 0xF0) <= 0x30 && (opcode & 0x0F) == 1) {
		const char *lparam[] = {"BC", "DE", "HL", "SP"}; 

		uint8_t *map[] = {&m_registers.B, &m_registers.C, 
			&m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, nullptr }; 

		uint8_t idx = (opcode & 0xF0) >> 4; 
		uint8_t *h = map[(idx * 2) + 0]; 
		uint8_t *l = map[(idx * 2) + 1]; 

		last_cycles += 12; 
		uint16_t arg = get_word();

		if (h != nullptr && l != nullptr) {
			*h = arg >> 8; 
			*l = arg & 0xFF; 
		} else {
			m_registers.SP = arg; 
		}

		printd(this, "($%04X) ($%02X) LD %s, $%04X\n",  startPC, opcode, lparam[idx], arg);		
		return;
	} 
	else if ((opcode & 0xF0) <= 0x30 && (opcode & 0x0F) == 2) {
		const char *lparam[] = {"(BC)", "(DE)", "(HL+)", "(HL-)"};

		uint16_t map[] = {m_registers.BC(), m_registers.DE(), m_registers.HL(), m_registers.HL()}; 
		int8_t incmap[] = {0, 0, 1, -1};

		uint8_t idx = (opcode & 0xF0) >> 4; 

		write_byte(map[idx], m_registers.A);
		m_registers.HL(m_registers.HL() + incmap[idx]);

		last_cycles += 12;

		printd(this, "($%04X) ($%02X) LD %s, A\n",  startPC, opcode, lparam[idx]);		
		return; 
	}
	else if (hnum <= 3 && (lnum == 0x6 || lnum == 0xE)) {
		uint8_t *map[] = {&m_registers.B, &m_registers.C, 
			&m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, NULL, &m_registers.A}; 

		uint8_t offset = (lnum == 0xE);
		uint8_t *l = map[(hnum * 2) + offset];

		uint8_t param = get_byte();

		if (l != nullptr) {
			last_cycles += 8; 
			*l = param; 
		}
		else {
			last_cycles += 12; 
			write_byte(m_registers.HL(), param);
		}
		
		printd(this, "($%04X) ($%02X) LD %s, $%02X\n",  startPC, opcode, params[(hnum * 2) + offset], param);		
		return;  
	}


	// 0x(0->3)A 
	else if (hnum <= 3 && lnum == 0xA) {
		last_cycles += 8; 

		switch (opcode) {
			case 0x0A:
				m_registers.A = read_byte(m_registers.BC());
				printd(this, "($%04X) ($%02X) LD A, (BC)\n",  startPC, opcode); 
				break; 
			case 0x1A: 
				m_registers.A = read_byte(m_registers.DE());
				printd(this, "($%04X) ($%02X) LD A, (DE)\n",  startPC, opcode); 
				break;
			case 0x2A:
				m_registers.A = read_byte(m_registers.HL()); // HL+
				m_registers.HL(m_registers.HL() + 1); 
				printd(this, "($%04X) ($%02X) LD A, (HL+)\n",  startPC, opcode); 
				break;
			case 0x3A: 
				m_registers.A = read_byte(m_registers.HL()); // HL- 
				m_registers.HL(m_registers.HL() - 1); 
				printd(this, "($%04X) ($%02X) LD A, (HL-)\n",  startPC, opcode); 
				break;
		}	

		return;
	}

	else if (opcode >= 0x40 && opcode < 0x80) {
		uint8_t *map[] = {&m_registers.B, &m_registers.C, 
			&m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, NULL, &m_registers.A}; 

		uint8_t hidx = (opcode >> 4) - 4;  
		uint8_t lidx = (opcode & 0x0F) % 8; 
		uint8_t off = ((opcode & 0x0F) > 7); 

		uint8_t *h = map[(hidx * 2) + off]; 
		uint8_t *l = map[lidx];

		last_cycles += 4;
		if (h == nullptr || l == nullptr) last_cycles += 8;

		if (h != nullptr) 
			*h = (l != nullptr ? *l : read_word(m_registers.HL())); 
		else {
			write_byte(m_registers.HL(), *l); 
		}

		printd(this, "($%04X) ($%02X) LD %s, %s\n",  startPC, opcode, params[(hidx * 2) + off], params[lidx]); 
		return;
	} 


	if (opcode == 0xE0) {
		uint8_t param = get_byte();
		write_byte(0xFF00 + param, m_registers.A);
		printd(this, "($%04X) ($%02X) LDH ($%02X), A\n",  startPC, opcode, 0xFF00 + param); 

		last_cycles += 12;
		return;	
	}
	else if (opcode == 0xE2) {
		last_cycles += 8;
		write_byte(0xFF00 + m_registers.C, m_registers.A);
		printd(this, "($%04X) ($%02X) LDH ($FF00 + C), A\n",  startPC, opcode); 

		return;		
	}
	else if (opcode == 0xF0) {
		uint8_t param = get_byte();
		m_registers.A = read_byte(0xFF00 + param); 
		printd(this, "($%04X) ($%02X) LDH A, ($%02X)\n",  startPC, opcode, 0xFF00 + param); 

		last_cycles += 12;
		return;			
	}
	else if (opcode == 0xEA) {
		last_cycles += 16; 

		uint16_t param = get_word();

		write_byte(param, m_registers.A); 

		printd(this, "($%04X) ($%02X) LDH ($%04X), A\n",  startPC, opcode, param); 
		return; 
	}
	else if (opcode == 0xFA) {
		last_cycles += 16; 

		uint16_t param = get_word();
		m_registers.A = read_byte(param);

		printd(this, "($%04X) ($%02X) LDH A, ($%04X)\n",  startPC, opcode, param); 
		return;
	}
	else if (opcode == 0xF8) {
		last_cycles += 12; 

		int8_t param1 = get_byte();
		uint16_t param2 = m_registers.SP;
		uint32_t result = param1 + param2; 
		uint32_t carry  = param1 ^ param2 ^ result;

		m_registers.HL(result); 

		m_registers.m_flags.Z = 0;
		m_registers.m_flags.N = 0;
		m_registers.m_flags.H = ((carry & 0x00010) > 0);
		m_registers.m_flags.C = ((carry & 0x00100) > 0);  	

		printd(this, "($%04X) ($%02X) LDHL SP, ($%02X)\n",  startPC, opcode, param1); 
		return; 
	} else if (opcode == 0xF9) {
		last_cycles += 8; 
		m_registers.SP = m_registers.HL();

		printd(this, "($%04X) ($%02X) LD SP, HL\n",  startPC, opcode); 
		return; 
	} else if (opcode == 0xF2) {
		last_cycles += 8;
		m_registers.A = read_byte(0xFF00 + m_registers.C);

		printd(this, "($%04X) ($%02X) LD A, (C)\n",  startPC, opcode); 
		return; 
	} else if (opcode == 0x08) {
		last_cycles += 20;
		uint16_t param = get_word();
		write_word(param, m_registers.SP);

		printd(this, "($%04X) ($%02X) LD ($%04X), SP\n", startPC, opcode, param); 
		return;
	}


	printf("Stopping LD at opcode %02X\n", opcode);
	exit(0);
}

void gameBoy::ROT(uint8_t opcode) {
	uint16_t startPC = m_registers.PC - 1; 
	last_cycles += 4; 

	if (opcode == 0x0F) { // RRCA 
		uint8_t bit0 = m_registers.A & 1; 
		m_registers.A >>= 1; 
		m_registers.A |= (bit0 << 7);

		m_registers.m_flags.C = bit0;
		m_registers.m_flags.Z = 0; 
		m_registers.m_flags.H = 0;
		m_registers.m_flags.N = 0;

		printd(this, "($%04X) ($%02X) RRCA\n", startPC, opcode);
	} else if (opcode == 0x1F) { // RRA 
		uint8_t old = m_registers.m_flags.C; 
		uint8_t bit0 = m_registers.A & 1; 

		m_registers.A >>= 1; 
		m_registers.A |= (old << 7);

		m_registers.m_flags.C = bit0;
		m_registers.m_flags.Z = 0; 
		m_registers.m_flags.H = 0;
		m_registers.m_flags.N = 0;

		printd(this, "($%04X) ($%02X) RRA\n", startPC, opcode);		
	} else if (opcode == 0x07) { // RLCA 
		uint8_t old = (m_registers.A >> 7) & 1;
		m_registers.A <<= 1; 
		m_registers.A |= old; 

		m_registers.m_flags.C = old;
		m_registers.m_flags.Z = 0; 
		m_registers.m_flags.N = 0;
		m_registers.m_flags.H = 0; 

		printd(this, "($%04X) ($%02X) RLCA\n", startPC, opcode);		
	} else if (opcode == 0x17) { // RLA 
		uint8_t old = (m_registers.A >> 7);
		m_registers.A <<= 1; 
		m_registers.A |= m_registers.m_flags.C; 

		m_registers.m_flags.C = old;
		m_registers.m_flags.Z = 0; 
		m_registers.m_flags.H = 0;
		m_registers.m_flags.N = 0;

		printd(this, "($%04X) ($%02X) RLA\n", startPC, opcode);	
	}
}

void gameBoy::CB(uint8_t opcode) {
	uint8_t *map[] = {&m_registers.B, &m_registers.C, 
		&m_registers.D, &m_registers.E, 
		&m_registers.H, &m_registers.L, nullptr, &m_registers.A }; 

	uint16_t startPC = m_registers.PC - 1; 
	uint8_t nopcode = get_byte();

	uint8_t hnib = nopcode >> 4; 
	uint8_t lnib = nopcode & 0xF; 

	if (nopcode < 0x20) {
		uint8_t doCarry = (hnib == 0x0);
		uint8_t dir = (lnib > 7);
		uint8_t *t = map[lnib % 8]; 

		if (!t) {
			// when using RLCx, the offset is the old bit 8 
			// get value at HL
			uint8_t v = read_byte(m_registers.HL()); 

			uint8_t carryOver = 0;
			if (dir) { // right 
				if (doCarry) { 
					carryOver = v & 1; 
					v >>= 1; 
					v |= (carryOver << 7);
				} else {
					carryOver = v & 1; 
					v >>= 1; 
					v |= (m_registers.m_flags.C << 7);
				}
			} else { // left 
				if (doCarry) { 
					carryOver = (v & 0x80) ? 1 : 0; 
					v <<= 1; 
					v |= carryOver;
				} else {
					carryOver = (v & 0x80) ? 1 : 0; 
					v <<= 1; 
					v |= m_registers.m_flags.C;
				}
			}

			m_registers.m_flags.C = carryOver;

			last_cycles += 16;
			write_byte(m_registers.HL(), v);
			m_registers.m_flags.Z = (v == 0);
		} else {
			last_cycles += 8;
			uint8_t v = *t; 

			uint8_t carryOver = 0;
			if (dir) { // right 
				if (doCarry) { 
					carryOver = v & 1; 
					v >>= 1; 
					v |= (carryOver << 7);
				} else {
					carryOver = v & 1; 
					v >>= 1; 
					v |= (m_registers.m_flags.C << 7);
				}
			} else { // left 
				if (doCarry) { 
					carryOver = (v & 0x80) ? 1 : 0; 
					v <<= 1; 
					v |= carryOver;
				} else {
					carryOver = (v & 0x80) ? 1 : 0; 
					v <<= 1; 
					v |= m_registers.m_flags.C;
				}
			}

			*t = v;
			m_registers.m_flags.C = carryOver;
			m_registers.m_flags.Z = (v == 0);
		}

		m_registers.m_flags.N = 0;
		m_registers.m_flags.H = 0;

		printd(this, "($%04X) ($%02X) R%s%s %s\n",  startPC, nopcode, (dir ? "R" : "L"), (doCarry ? "C" : ""), params[lnib % 8]); 
		return;
	}

	if (hnib == 0x2) {
		uint8_t dir = (lnib > 7);
		uint8_t *t = map[lnib % 8]; 

		uint8_t v = (t ? *t : read_byte(m_registers.HL()));

		uint8_t carryOver = 0;
		if (dir == 1) { // right 
			uint8_t c = v & (1 << 7); 
			carryOver = v & 0x1; 
			v >>= 1; 
			v |= c; 
		} else { // left 
			carryOver = (v & 0x80) ? 1 : 0; 
			v <<= 1; 
		}

		if (t)
			*t = v; 
		else 
			write_byte(m_registers.HL(), v);

		m_registers.m_flags.Z = (v == 0);
		m_registers.m_flags.H = 0;
		m_registers.m_flags.N = 0;
		m_registers.m_flags.C = carryOver;
		last_cycles += t ? 8 : 16; 

		printd(this, "($%04X) ($%02X) S%sA (HL)\n",  startPC, nopcode, (dir ? "R" : "L")); 			
		return;
	}

	if (hnib == 0x3) {
		if (lnib > 7) { // SRL
			uint8_t *t = map[lnib % 8]; 

			uint8_t v = (t ? *t : read_byte(m_registers.HL()));

			uint8_t carryOver = 0;
			carryOver = v & 0x1; 
			v >>= 1; 

			if (t)
				*t = v; 
			else 
				write_byte(m_registers.HL(), v);

			m_registers.m_flags.Z = (v == 0);
			m_registers.m_flags.H = 0;
			m_registers.m_flags.N = 0; 
			m_registers.m_flags.C = carryOver;
			last_cycles += t ? 8 : 16; 

			printd(this, "($%04X) ($%02X) SRL %s\n",  startPC, nopcode, params[lnib % 8]); 			
			return;
		}
	}

	if (nopcode >= 0x40 && nopcode < 0x80) {
		uint8_t off = (lnib >= 8); 

		uint8_t bit = ((hnib - 4) * 2) + off; 
		uint8_t *t = map[lnib % 8]; 
		last_cycles += (t ? 8 : 16); 

		uint8_t v = (t ? *t : read_byte(m_registers.HL())); 
		int res = v & (1 << bit);

		m_registers.m_flags.Z = (res == 0); 
		m_registers.m_flags.N = 0;
		m_registers.m_flags.H = 1;

		printd(this, "($%04X) ($%02X) BIT %d, %s\n",  startPC, nopcode, bit, params[lnib % 8]);
		return; 
	}

	if (nopcode >= 0x30 && nopcode <= 0x37) {
		last_cycles += 8;

		uint8_t *t = map[lnib % 8];
		uint8_t v = 0;
		if (t) 
			v = *t; 
		else 
			v = read_byte(m_registers.HL()), last_cycles += 16;

		uint8_t res = 0; 
		res |= ((v & 0xF0) >> 4);
		res |= ((v & 0x0F) << 4); 

		if (t) 
			*t = res; 
		else 
			write_byte(m_registers.HL(), res);

		m_registers.m_flags.Z = (res == 0);
		m_registers.m_flags.N = 0;	
		m_registers.m_flags.H = 0;
		m_registers.m_flags.C = 0;

		printd(this, "($%04X) ($%02X) SWAP %s\n",  startPC, nopcode, params[lnib % 8]);
		return;
	}	

	if (nopcode >= 0x80 && nopcode < 0xC0) {
		uint8_t bitn = (hnib-8) * 2;
		bitn += (lnib > 7);

		if (lnib == 0xE || lnib == 0x6)
			last_cycles += 16;
		else 
			last_cycles += 8;

		uint8_t param = map[lnib % 8] ? *map[lnib % 8] : read_byte(m_registers.HL()); 
		param &= (0xff ^ (1 << bitn));

		if (map[lnib % 8]) {
			*map[lnib % 8] = param; 
		}
		else {
			write_byte(m_registers.HL(), param);
		}

		printd(this, "($%04X) ($%02X) RES %d, %s\n",  startPC, nopcode, bitn, params[lnib % 8]);
		return; 
	}

	if (nopcode >= 0xC0) {
		uint8_t bitn = (hnib-0xC) * 2;
		bitn += (lnib > 7);

		if (lnib == 0xE || lnib == 0x6)
			last_cycles += 16;
		else 
			last_cycles += 8;

		uint8_t param = map[lnib % 8] ? *map[lnib % 8] : read_byte(m_registers.HL()); 
		param |= (1 << bitn);

		if (map[lnib % 8]) {
			*map[lnib % 8] = param; 
		}
		else {
			write_byte(m_registers.HL(), param);
		}

		printd(this, "($%04X) ($%02X) SET %d, %s\n",  startPC, nopcode, bitn, params[lnib % 8]);
		return; 
	}

	printf("Stopping CB at CB %02X\n", nopcode);
	exit(0); 
}
