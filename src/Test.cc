#include "CPU.h"
#include <functional>
#include <map>
#include <sstream>
#include <iomanip>

#define PASS ""

#define TEST(a,b) v[a] = [&]() b;

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KYEL  "\x1B[33m"
#define KGRN  "\x1B[32m"
#define CYCLES(a) if (last_cycles != a) return "Expected last_cycles to be "#a""; return PASS;

#define EXPECT(a,b) if (a != (b)) return "Expected "#a" to be "#b".";
#define OPCODE(a) insmap[a](this, a);

void gameBoy::test() {
	auto loadValues = [&](uint8_t a, uint8_t b) {
		m_registers.testingOffset = 0;
		m_registers.tbytes[0] = a;
		m_registers.tbytes[1] = b;
	};

	printf("\n\nTESTING FUNCTIONS...\n\n\n");

	std::map<std::string, std::function<std::string()>> v;

	// internal tests
	v["loadValues"] = [&]() {
		loadValues(0x12, 0x34);
		if (m_registers.tbytes[0] != 0x12 || m_registers.tbytes[1] != 0x34) 
			return "Expected m_registers.tbytes to be (0x12, 0x34).";
		return PASS;
	};

	TEST("LD DE, $0104", {
		loadValues(0x04, 0x01);
		OPCODE(0x11);

		EXPECT(m_registers.DE(), 0x0104); 
		return ""; 
	});

	TEST("RRA BUG", {
		// 0x1f 
		m_registers.A = 0x43;
		m_registers.m_flags.C = 1; 
		OPCODE(0x1F);
		EXPECT(m_registers.A, 0xA1);
		EXPECT(m_registers.m_flags.C, 1);
		return "";
	}); 


	TEST("NOP", {
		OPCODE(0x0)
		CYCLES(4) 
	})

	TEST("STOP", {
		OPCODE(0x10);
		CYCLES(4);
	})

	// relative jumps 
	for (int i = 0; i < 5; i++) {
		const int opmap[] = {0x18, 0x20, 0x28, 0x30, 0x38};
		const char *map[] = {"", "NZ, ", "Z, ", "NC, ", "C, "}; 
		uint8_t flags[] = {0x1, 0x0, 0x1, 0x0, 0x8};

		std::stringstream stm; 
		stm << "JR " << map[i] << "n";

		v[stm.str()] = [&, i]() {
			if (opmap[i] == 0x18) {
				loadValues(0x01, 0x00);
				OPCODE(opmap[i]);
				EXPECT(m_registers.PC, 0x0001);

				loadValues(0xFF, 0x00);
				OPCODE(opmap[i]);
				EXPECT(m_registers.PC, 0x0000);

				CYCLES(8);
			}

			uint8_t *loc = (uint8_t *)&m_registers.m_flags;
			*loc = 0x00;

			*loc = flags[i];  												
			loadValues(0x01, 0x00);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x0001);

			*loc = flags[(opmap[i]&8)?i-1:i+1];  		
			loadValues(0x01, 0x00);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x0001);		

			*loc = flags[i];  							
			loadValues(0xFF, 0x00);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x0000);		

			*loc = flags[(opmap[i]&8)?i-1:i+1];  													
			loadValues(0xFF, 0x00);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x0000);		

			*loc = 0;

			CYCLES(8);
		}; 
	}

	// $01, $11, $21, $31 
	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"BC", "DE", "HL", "SP"};
		// c = 12 

		std::stringstream stm;
		stm << "LD " << cmap[i] << ", nn"; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 1;  
			loadValues(0x34, 0x12);

			OPCODE(opcode);

			switch (opcode) {
				case 0x01: 
					EXPECT(m_registers.BC(), 0x1234)
					break;
				case 0x11: 
					EXPECT(m_registers.DE(), 0x1234)
					break;
				case 0x21: 
					EXPECT(m_registers.HL(), 0x1234)
					break;
				case 0x31: 
					EXPECT(m_registers.SP, 0x1234)
					break;
			}

			CYCLES(12);
		};
	}

	// $02, $12, $22, $32
	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"(BC)", "(DE)", "(HL+)", "(HL-)"};

		std::stringstream stm;
		stm << "LD " << cmap[i] << ", A"; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 2;  
			switch (opcode) {
				case 0x02: 
					m_registers.B = 0x80; 
					break;
				case 0x12: 
					m_registers.D = 0x80; 
					break;
				case 0x22: 
				case 0x32: 
					m_registers.H = 0x80;
					break; 
			}
			m_registers.A = 0x12;

			OPCODE(opcode);

			switch (opcode) {
				case 0x02: 
					EXPECT(read_byte(m_registers.BC()), 0x12);
					break;
				case 0x12: 
					EXPECT(read_byte(m_registers.DE()), 0x12);
					break;
				case 0x22: 
					EXPECT(m_registers.HL(), 0x8001)
					EXPECT(read_byte(m_registers.HL() - 1), 0x12);
					break;
				case 0x32: 
					EXPECT(m_registers.HL(), 0x7FFF)
					EXPECT(read_byte(m_registers.HL() + 1), 0x12);
					break;	
			}

			CYCLES(12);
		};
	}

	// $03, $13, $23, $33
	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"BC", "DE", "HL", "SP"};

		std::stringstream stm;
		stm << "INC " << cmap[i]; 
	
		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 3;
			switch (opcode) {
				case 0x03:
					m_registers.C = 0xff; 
					break;
				case 0x13:
					m_registers.E = 0xff; 
					break;
				case 0x23:
					m_registers.L = 0xff; 
					break;
				case 0x33:
					m_registers.SP = 0xff;
					break; 
			}

			OPCODE(opcode);

			switch (opcode) {
				case 0x03:
					EXPECT(m_registers.BC(), 0x100);
					break;
				case 0x13:
					EXPECT(m_registers.DE(), 0x100);
					break;
				case 0x23:
					EXPECT(m_registers.HL(), 0x100);
					break;
				case 0x33:
					EXPECT(m_registers.SP, 0x100);
					break;
			}

			switch (opcode) {
				case 0x03:
					m_registers.B = 0xff; 
					m_registers.C = 0xff; 
					break;
				case 0x13:
					m_registers.D = 0xff; 
					m_registers.E = 0xff; 
					break;
				case 0x23:
					m_registers.H = 0xff; 
					m_registers.L = 0xff; 
					break;
				case 0x33:
					m_registers.SP = 0xffff;
					break; 
			}

			OPCODE(opcode);

			switch (opcode) {
				case 0x03:
					EXPECT(m_registers.BC(), 0);
					break;
				case 0x13:
					EXPECT(m_registers.DE(), 0);
					break;
				case 0x23:
					EXPECT(m_registers.HL(), 0);
					break;
				case 0x33:
					EXPECT(m_registers.SP, 0);
					break;
			}

			CYCLES(8);
		};
	}


	for (int i = 0; i < 8; i++) {
		const char *smap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		uint8_t opcodes[] = {0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C};

		std::stringstream stm;
		stm << "INC " << smap[i]; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = opcodes[i];

			int expectCycles = 4;

			if (map[i]) {
				*map[i] = 0x0f;
			} else {
				m_registers.H = 0x80; 			
				m_registers.L = 0x00; 
				write_byte(0x8000, 0x0f);	
				expectCycles = 8;
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0x10);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0x10); 
			}

			EXPECT(m_registers.m_flags.H, 1);

			if (map[i]) {
				*map[i] = 0xFF;
			} else {
				m_registers.H = 0x80; 			
				m_registers.L = 0x00; 
				write_byte(0x8000, 0xFF);	
				expectCycles = 12;
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0x00);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0x00); 
			}

			EXPECT(m_registers.m_flags.Z, 1)
			CYCLES(expectCycles);
		};
	}

	for (int i = 0; i < 8; i++) {
		const char *smap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		uint8_t opcodes[] = {0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D};

		std::stringstream stm;
		stm << "DEC " << smap[i]; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = opcodes[i];

			int expectCycles = 4;

			if (map[i]) {
				*map[i] = 0x10;
			} else {
				m_registers.H = 0x80; 			
				m_registers.L = 0x00; 
				write_byte(0x8000, 0x10);	
				expectCycles = 8;
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0x0f);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0x0f); 
			}

			EXPECT(m_registers.m_flags.H, 1);

			if (map[i]) {
				*map[i] = 0x1;
			} else {
				m_registers.H = 0x80; 			
				m_registers.L = 0x00; 
				write_byte(0x8000, 0x1);	
				expectCycles = 12;
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0x00);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0x00); 
			}

			EXPECT(m_registers.m_flags.Z, 1)

			if (map[i]) {
				*map[i] = 0x00;
			} else {
				m_registers.H = 0x80; 			
				m_registers.L = 0x00; 
				write_byte(0x8000, 0x00);	
				expectCycles = 12;
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0xFF);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0xFF); 
			}

			CYCLES(expectCycles);
		};
	}

	for (int i = 0; i < 9; i++) {
		const char *smap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		uint8_t opcodes[] = {0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E, 0x08};

		if (i == 8) {
			// LD (nn), SP

			TEST("LD (nn), SP", {
				loadValues(0x00, 0x80);
				m_registers.SP = 0x1234; 
				OPCODE(0x08);

				EXPECT(read_word(0x8000), 0x1234); 

				CYCLES(20); 
			});

			continue;
		}

		std::stringstream stm; 
		stm << "LD " << smap[i] << ", n";

		v[stm.str()] = [&,i]() {
			uint8_t opcode = opcodes[i];
			loadValues(0x12, 0x00);

			int expectCycles = 8; 

			if (!map[i]) {
				m_registers.H = 0x80;
				m_registers.L = 0x00;

				expectCycles = 12; 
			}

			OPCODE(opcode);

			if (map[i]) {
				EXPECT(*map[i], 0x12);
			} else {
				EXPECT(read_byte(m_registers.HL()), 0x12);
			}

			CYCLES(expectCycles);
		};
	}

	// $0B, $1B, $2B, $3B
	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"BC", "DE", "HL", "SP"};

		std::stringstream stm;
		stm << "DEC " << cmap[i]; 
	
		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 0xB;
			switch (opcode) {
				case 0x0B:
					m_registers.B = 0x1; 
					break;
				case 0x1B:
					m_registers.D = 0x1; 
					break;
				case 0x2B:
					m_registers.H = 0x1; 
					break;
				case 0x3B:
					m_registers.SP = 0x100;
					break; 
			}

			OPCODE(opcode);

			switch (opcode) {
				case 0x03:
					EXPECT(m_registers.BC(), 0xff);
					break;
				case 0x13:
					EXPECT(m_registers.DE(), 0xff);
					break;
				case 0x23:
					EXPECT(m_registers.HL(), 0xff);
					break;
				case 0x33:
					EXPECT(m_registers.SP, 0xff);
					break;
			}

			switch (opcode) {
				case 0x03:
					m_registers.B = 0x00; 
					m_registers.C = 0x00; 
					break;
				case 0x13:
					m_registers.D = 0x00; 
					m_registers.E = 0x00; 
					break;
				case 0x23:
					m_registers.H = 0x00; 
					m_registers.L = 0x00; 
					break;
				case 0x33:
					m_registers.SP = 0x0000;
					break; 
			}

			OPCODE(opcode);

			switch (opcode) {
				case 0x03:
					EXPECT(m_registers.BC(), 0xffff);
					break;
				case 0x13:
					EXPECT(m_registers.DE(), 0xffff);
					break;
				case 0x23:
					EXPECT(m_registers.HL(), 0xffff);
					break;
				case 0x33:
					EXPECT(m_registers.SP, 0xffff);
					break;
			}

			CYCLES(8);
		};
	}

	// $x9 
	for (int i = 0; i < 4; i++) {
		const char *smap[] = {"BC", "DE", "HL", "SP"}; 

		std::stringstream stm;
		stm << "ADD HL, " << smap[i]; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 9;
			switch (i) {
				case 0:
					m_registers.BC(0x1111);
					break;
				case 1:
					m_registers.DE(0x1111);
					break;
				case 2:
					m_registers.HL(0x1111);
					break;
				case 3:
					m_registers.SP = 0x1111;
					break;
			}

			m_registers.HL(0x1111);
			OPCODE(opcode);

			EXPECT(m_registers.HL(), 0x2222);

			CYCLES(8);
		};
	}

	// $xA 
	for (int i = 0; i < 4; i++) {
		const char *smap[] = {"(BC)", "(DE)", "(HL+)", "(HL-)"};

		std::stringstream stm;
		stm << "ADD A, " << smap[i]; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = (i << 4) + 0xA;
			switch (i) {
				case 0:
					m_registers.BC(0x8000);
					break;
				case 1:
					m_registers.DE(0x8000);
					break;
				case 2:
				case 3:
					m_registers.HL(0x8000);
					break;
			}

			write_byte(0x8000, 0x12);
			OPCODE(opcode);


			EXPECT(m_registers.A, 0x12);

			switch (i) {
				case 2:
					EXPECT(m_registers.HL(), 0x8001);
					break;
				case 3:
					EXPECT(m_registers.HL(), 0x7FFF);
					break;
			}

			CYCLES(8);
		};
	}

	TEST("RLCA", {
		m_registers.A = 0x3; 
		OPCODE(0x07);
		EXPECT(m_registers.A, 6);

		m_registers.A = 0x81; 
		OPCODE(0x07);
		EXPECT(m_registers.A, 3);
		EXPECT(m_registers.m_flags.C, 1);

		CYCLES(4);
	});

	TEST("RLA", {
		m_registers.A = 0x3; 
		OPCODE(0x17);
		EXPECT(m_registers.A, 6);

		m_registers.m_flags.C = 1; 
		m_registers.A = 0x3; 
		OPCODE(0x17);
		EXPECT(m_registers.A, 7);

		m_registers.m_flags.C = 0; 
		m_registers.A = 0x81; 
		OPCODE(0x17);
		EXPECT(m_registers.A, 0x2);
		EXPECT(m_registers.m_flags.C, 1);

		CYCLES(4);
	});

	TEST("DAA", {
		m_registers.A = 0x3C;
		OPCODE(0x27); 
		EXPECT(m_registers.A, 0x42);
		EXPECT(m_registers.m_flags.H, 0);

		CYCLES(4); 
	});

	TEST("SCF", {
		m_registers.m_flags.C = 0;
		OPCODE(0x37);
		EXPECT(m_registers.m_flags.C, 1);
		EXPECT(m_registers.m_flags.H, 0);
		EXPECT(m_registers.m_flags.N, 0);

		CYCLES(4);
	});	

	TEST("RRC A", {
		// 1000 0001 0 
		// 1100 0000 1 
		m_registers.A = 0x81; 
		OPCODE(0xF)
		EXPECT(m_registers.A, 0xC0)
		EXPECT(m_registers.m_flags.C, 1)

		CYCLES(4)
	});

	TEST("RRA", {
		// 1000 0001 
		// 0100 0000 1 
		// 1010 0000 0
		m_registers.A = 0x81; 
		OPCODE(0x1F);
		EXPECT(m_registers.A, 0x40);
		EXPECT(m_registers.m_flags.C, 1);

		OPCODE(0x1F); 
		EXPECT(m_registers.A, 0xA0);
		EXPECT(m_registers.m_flags.C, 0);

		CYCLES(4);		
	});

	TEST("CPL", {
		m_registers.A = 0xAA;
		OPCODE(0x2F);
		EXPECT(m_registers.A, 0x55);
		EXPECT(m_registers.m_flags.H, 1);
		EXPECT(m_registers.m_flags.N, 1);

		CYCLES(4);
	});

	TEST("CCF", {
		m_registers.m_flags.C = 1;
		OPCODE(0x3F);
		EXPECT(m_registers.m_flags.C, 0);
		EXPECT(m_registers.m_flags.H, 1);
		EXPECT(m_registers.m_flags.N, 0);

		CYCLES(4);
	});

	for (int i = 0x80; i < 0xA0; i++) {
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		const char *sinsmap[] = {"ADD", "ADC", "SUB", "SBC"};
		const char *cmap[] = {"B","C","D","E","H","L","(HL)","A"};

		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t offset = lnib > 7; 

		std::stringstream stm;
		stm << sinsmap[((hnib - 0x8) * 2) + offset] << " A, " << cmap[lnib % 8];

		v[stm.str()] = [&,i,hnib,lnib,offset]() {
			uint8_t insidx = ((hnib - 0x8) * 2) + offset; 
			// do carry tests. result should be 1 higher/lower with carry tests 

			m_registers.A = 0x01; 
			m_registers.m_flags.C = 0x01;

			int expectCycles = 4; 

			if (map[lnib % 8]) {
				*map[lnib % 8] = 0x01;
			} else {
				m_registers.HL(0x8000);
				write_byte(0x8000, 0x01);
				expectCycles = 8;
			}

			OPCODE(i);

			if (insidx == 0)
				EXPECT(m_registers.A, 0x02);
			if (insidx == 1)
				EXPECT(m_registers.A, 0x03);
			if (insidx == 2)
				EXPECT(m_registers.A, 0x00);
			if (insidx == 3)
				EXPECT(m_registers.A, 0xFF);

			CYCLES(expectCycles);
		};
	}

	for (int i = 0xA0; i < 0xC0; i++) {
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		const char *sinsmap[] = {"AND", "XOR", "OR", "CP"};
		const char *cmap[] = {"B","C","D","E","H","L","(HL)","A"};

		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t offset = lnib > 7; 

		std::stringstream stm;
		stm << sinsmap[((hnib - 0xA) * 2) + offset] << " " << cmap[lnib % 8];			

		v[stm.str()] = [&,i,hnib,lnib,offset]() {
			uint8_t insidx = ((hnib - 0xA) * 2) + offset;

			m_registers.A = 0xAA;

			int expectCycles = 4; 

			if (map[lnib % 8]) {
				*map[lnib % 8] = 0x55;
			} else {
				m_registers.HL(0x8000);
				write_byte(0x8000, 0x55);
				expectCycles = 8;
			}

			OPCODE(i);

			if (insidx == 0) {
				bool selfcompare = (lnib % 8) == 7;
				uint8_t value = (selfcompare ? 0x55 : 0x00);
				EXPECT(m_registers.A, value);
				EXPECT(m_registers.m_flags.Z, value ? 0 : 1);
			}
			else if (insidx == 1) {
				EXPECT(m_registers.A, (lnib%8) == 7 ? 0x00 : 0xFF);
				EXPECT(m_registers.m_flags.Z, (lnib % 8) == 7); 
			}
			else if (insidx == 2) {
				EXPECT(m_registers.A, (lnib%8) == 7 ? 0x55 : 0xFF);
				EXPECT(m_registers.m_flags.Z, 0); 
			}
			else if (insidx == 3) {
				bool selfcompare = (lnib % 8) == 7;
				EXPECT(m_registers.A, selfcompare ? 0x55 : 0xAA);
				EXPECT(m_registers.m_flags.Z, selfcompare); 				
				EXPECT(m_registers.m_flags.C, 0); 				
				EXPECT(m_registers.m_flags.N, 1); 				
			}

			CYCLES(expectCycles);
		}; 
	}


	/*
	** Tests for LD between opcode $40 and < $80 
	** This range includes HALT at $76 so we check for that. 
	*/
	for (int i = 0x40; i < 0x80; i++) {
		if (i == 0x76) {
			TEST("HALT", {
				OPCODE(0x76);
				CYCLES(4);
			});
			continue; 
		}

		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 
		const char *cmap[] = {"B","C","D","E","H","L","(HL)","A"};

		uint8_t hnib = (i >> 4) - 4;
		uint8_t off  = (i & 0xF) > 7; 
		uint8_t lnib = (i & 0xF) % 8;

		std::stringstream stm; 
		stm << "LD " << cmap[(hnib * 2) + off] << ", " << cmap[lnib];

		v[stm.str()] = [&, lnib, hnib, off, i]() {
			uint8_t *target = map[(hnib * 2) + off];
			uint8_t *source = map[lnib];

			m_registers.HL(0xFFFF); 

			int expectCycles = 4; 

			if (!source) {
				write_byte(m_registers.HL(), 0xFF);
				expectCycles = 8;
			} else {
				*source = 0xFF; 
			}

			OPCODE(i);

			if (target) {
				std::stringstream stm; 
				stm << "Expected " << cmap[(hnib * 2) + off] << " to be $12";
				if (*target != 0xFF) return stm.str().c_str();
			} else {
				expectCycles = 8;
				if (read_byte(0xFFFF) != 0xFF) return "Expected (HL) to be $FF";
			}

			if (last_cycles != expectCycles) {
				std::stringstream stm;
				stm << "Expected last_cycles to be " << expectCycles;
				return stm.str().c_str(); 
			}

			return PASS;
		};
	}

	for (int i = 0; i < 6; i++) {
		const int opmap[] = {0xC0, 0xC8, 0xC9, 0xD0, 0xD8, 0xD9};
		const char *map[] = {" NZ", " Z", "", " NC", " C", "I"};
		uint8_t flags[] = {0x0, 0x1, 0x0, 0x0, 0x8, 0x0};


		std::stringstream stm;
		stm << "RET" << map[i]; 

		v[stm.str()] = [&, i]() {
			auto setupStack = [&]() {
				m_registers.SP = 0xFFFE;
				m_registers.PC = 0x1234; 
				m_registers.SP -= 2; 
				write_word(m_registers.SP, m_registers.PC);
				m_registers.PC = 0xFFFF;
			}; 

			uint8_t *flagsp = (uint8_t *)&m_registers.m_flags;

			// test jumping 
			setupStack();
			*flagsp = flags[i];
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x1234);

			if (opmap[i] == 0xC9 || opmap[i] == 0xD9) {
				CYCLES(8);
			}

			// test no jump 
			setupStack();
			*flagsp = flags[i + 1]; 
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0xFFFF);

			CYCLES(8);
		};
	}

	// JPs 
	for (int i = 0; i < 5; i++) {
		const int opmap[] = {0xC2, 0xC3, 0xCA, 0xD2, 0xDA, 0xE9};
		const char *map[] = {"NZ, nn", "nn", "Z, nn", "NC, nn", "C, nn", "(HL)"}; 
		uint8_t flags[] = {0x0, 0x1, 0x1, 0x0, 0x8, 0x0};

		std::stringstream stm; 
		stm << "JP " << map[i];

		v[stm.str()] = [&, i]() {
			if (opmap[i] == 0xC3) {
				loadValues(0x34, 0x12);
				OPCODE(opmap[i]);
				EXPECT(m_registers.PC, 0x1234);

				CYCLES(12);
			}

			uint8_t *loc = (uint8_t *)&m_registers.m_flags;
			*loc = 0x00;

			*loc = flags[i];  												
			loadValues(0x34, 0x12);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0x1234);

			*loc = flags[i+1];  
			m_registers.PC = 0xFFFF;												
			loadValues(0x34, 0x12);
			OPCODE(opmap[i]);
			EXPECT(m_registers.PC, 0xFFFF);

			CYCLES(12);
		}; 
	}

	TEST("LDH (n), A", {
		loadValues(0x80, 0xFF);
		m_registers.A = 0x12;
		OPCODE(0xE0);
		EXPECT(read_byte(0xFF80), 0x12);
		CYCLES(12);
	});

	TEST("LDH A, (n)", {
		write_byte(0xFF80, 0x12);
		loadValues(0x80, 0xFF);
		OPCODE(0xF0);
		EXPECT(m_registers.A, 0x12);
		CYCLES(12);
	});

	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"BC", "DE", "HL", "AF"};
		// c = 12 

		std::stringstream stm;
		stm << "POP " << cmap[i] << ""; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = ((i + 0xC) << 4) + 1; 

			m_registers.SP = 0xFFF0; 
			m_registers.SP -= 2;
			write_word(m_registers.SP, 0x1234);
			OPCODE(opcode);

			switch (i) {
				case 0:
					EXPECT(m_registers.BC(), 0x1234);
					break;
				case 1:
					EXPECT(m_registers.DE(), 0x1234);
					break;
				case 2:
					EXPECT(m_registers.HL(), 0x1234);
					break;
				case 3:
					EXPECT(m_registers.AF(), 0x1234);
					break;
			}

			CYCLES(12);
		};
	}

	TEST("LDH (C), A", {
		m_registers.C = 0x80;
		m_registers.A = 0x12;
		OPCODE(0xE2);
		EXPECT(read_byte(0xFF80), 0x12);
		CYCLES(8);
	});

	TEST("LDH A, (C)", {
		m_registers.C = 0x80;
		write_byte(0xFF80, 0x12);
		OPCODE(0xF2);
		EXPECT(m_registers.A, 0x12);	
		CYCLES(8);
	});

	TEST("DI", {
		OPCODE(0xF3);
		EXPECT(m_registers.toggleIME, 0x00);
		CYCLES(4);
	});
	
	TEST("EI", {
		OPCODE(0xFB);
		EXPECT(m_registers.toggleIME, 0x01);
		CYCLES(4);
	});


	for (int i = 0; i < 5; i++) {
		const int opmap[] = {0xC4, 0xCC, 0xCD, 0xD4, 0xDC};
		const char *map[] = {"NZ, nn", "Z, nn", "nn", "NC, nn", "C, nn"};
		uint8_t flags[] = {0x0, 0x1, 0x0, 0x0, 0x8, 0x0};

		std::stringstream stm;
		stm << "CALL " << map[i];

		v[stm.str()] = [&,i]() {
			if (opmap[i] == 0xCD) {
				loadValues(0x34, 0x12);
				m_registers.PC = 0x4321;
				m_registers.SP = 0xFFF0; 
				OPCODE(opmap[i]);
				EXPECT(read_word(m_registers.SP), 0x4321);
				EXPECT(m_registers.PC, 0x1234);

				CYCLES(12);
			}

			uint8_t *loc = (uint8_t *)&m_registers.m_flags;
			*loc = 0x00;

			*loc = flags[i];  												
			loadValues(0x34, 0x12);
			m_registers.PC = 0x4321;
			m_registers.SP = 0x9000; 
			OPCODE(opmap[i]);
			printf("%04X\n", read_word(0x9000));
			EXPECT(read_word(m_registers.SP), 0x4321);

			EXPECT(m_registers.PC, 0x1234);

			*loc = flags[i+1];  
			loadValues(0x34, 0x12);
			m_registers.PC = 0x4321;
			m_registers.SP = 0xFFF0; 
			OPCODE(opmap[i]);
			EXPECT(m_registers.SP, 0xFFF0);
			EXPECT(m_registers.PC, 0x4321);

			CYCLES(12);		
		};
	}

	for (int i = 0; i < 4; i++) {
		const char *cmap[] = {"BC", "DE", "HL", "AF"};
		// c = 12 

		std::stringstream stm;
		stm << "PUSH " << cmap[i] << ""; 

		v[stm.str()] = [&, i]() {
			uint8_t opcode = ((i + 0xC) << 4) + 5; 

			m_registers.SP = 0xFFF0; 

			switch (i) {
				case 0:
					m_registers.BC(0x1234);
					break;
				case 1:
					m_registers.DE(0x1234);
					break;
				case 2:
					m_registers.HL(0x1234);
					break;
				case 3:
					m_registers.AF(0x1234);
					break;
			}

			OPCODE(opcode);
			EXPECT(m_registers.SP, 0xFFEE); 
			EXPECT(read_word(m_registers.SP), 0x1234);

			CYCLES(12);
		};
	}

	TEST("ADD A, n", {
		loadValues(0x12, 0x34);
		m_registers.A = 0x12; 
		OPCODE(0xC6);
		EXPECT(m_registers.A, 0x24);
		CYCLES(8);
	});
	
	TEST("SUB A, n", {
		loadValues(0x12, 0x34);
		m_registers.A = 0x12; 
		OPCODE(0xD6);
		EXPECT(m_registers.A, 0x00);
		EXPECT(m_registers.m_flags.Z, 1);
		CYCLES(8);
	});

	TEST("AND n", {
		loadValues(0x00, 0x34);
		m_registers.A = 0x12; 
		OPCODE(0xE6);
		EXPECT(m_registers.A, 0);
		EXPECT(m_registers.m_flags.Z, 1);
		CYCLES(8);
	});	

	TEST("OR n", {
		loadValues(0x01, 0x34);
		m_registers.A = 0x12; 
		OPCODE(0xF6);
		EXPECT(m_registers.A, 0x13);
		CYCLES(8);
	});	

	for (int i = 0; i < 8; i++) {
		uint8_t opcodes[] = {0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF};
		std::stringstream stm;
		stm << "RST " << std::hex << (i * 8);

		v[stm.str()] = [&, i]() {
			m_registers.SP = 0xFFF0;
			m_registers.PC = 0x1234;
			OPCODE(opcodes[i]);
			EXPECT(m_registers.PC, (i * 8));
			EXPECT(m_registers.SP, 0xFFEE);
			EXPECT(read_word(m_registers.SP), 0x1234);
			CYCLES(32);
		};
	}

	TEST("ADD SP, d", {
		loadValues(0xFF, 0x01); 
		m_registers.SP = 0x1234;
		OPCODE(0xE8);
		EXPECT(m_registers.SP, 0x1233);
		OPCODE(0xE8);
		EXPECT(m_registers.SP, 0x1234);
		
		CYCLES(16);
	});

	TEST("LDHL SP, d", {
		loadValues(0xFF, 0x01); 
		m_registers.SP = 0x1234;
		OPCODE(0xF8);
		EXPECT(m_registers.HL(), 0x1233);
		OPCODE(0xF8);
		EXPECT(m_registers.HL(), 0x1235);
		
		CYCLES(12);
	})

	TEST("JP (HL)", {
		m_registers.HL(0x8000);
		write_word(m_registers.HL(), 0x1234);
		OPCODE(0xE9);
		EXPECT(m_registers.PC, 0x1234);
		CYCLES(4);
	});

	TEST("LD SP, HL", {
		m_registers.HL(0x1234);
		OPCODE(0xF9);
		EXPECT(m_registers.SP, 0x1234);
		CYCLES(8);
	});

	TEST("LD (nn), A", {
		loadValues(0x00, 0x80);
		m_registers.A = 0x43;  
		OPCODE(0xEA);
		EXPECT(read_byte(0x8000), 0x43);
		CYCLES(16);
	});

	TEST("LD A, (nn)", {
		loadValues(0x00, 0x80);
		write_byte(0x8000, 0x43);
		OPCODE(0xFA);
		EXPECT(m_registers.A, 0x43);
		CYCLES(16);
	});

	TEST("ADC A, n", {
		loadValues(0x01, 0x01); 
		m_registers.A = 0x00; 
		m_registers.m_flags.C = 0;
		OPCODE(0xCE);
		EXPECT(m_registers.A, 0x01);

		m_registers.m_flags.C = 1;
		OPCODE(0xCE);
		EXPECT(m_registers.A, 0x03);

		CYCLES(8); 
	});

	TEST("SBC A, n", {
		loadValues(0x01, 0x01); 
		m_registers.A = 0x00; 
		m_registers.m_flags.C = 0;
		OPCODE(0xDE);
		EXPECT(m_registers.A, 0xFF);

		m_registers.m_flags.C = 1;
		OPCODE(0xDE);
		EXPECT(m_registers.A, 0xFD);

		CYCLES(8); 
	});

	TEST("XOR n", {
		loadValues(0x55, 0x00);
		m_registers.A = 0xAA; 
		OPCODE(0xEE);
		EXPECT(m_registers.A, 0xFF);
		CYCLES(8);
	})

	TEST("CP n", {
		loadValues(0x55, 0x44);
		m_registers.A = 0x44;
		OPCODE(0xFE);
		EXPECT(m_registers.m_flags.C, 1);
		
		OPCODE(0xFE);
		EXPECT(m_registers.m_flags.Z, 1);
		
		CYCLES(8);
	}); 

	/*
	**
	** CB EXTENDED SECTION 
	**		
	**
	*/ 
	for (int i = 0x00; i < 0x40; i++) {
		const char *smap[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"};
		const char *pmap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 


		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t off  = lnib > 7; 

		std::stringstream stm; 
		stm << smap[(hnib * 2) + off] << " " << pmap[lnib % 8];

		v[stm.str()] = [&, i, hnib, lnib, off]() {
			uint8_t opn = (hnib * 2) + off; 
			if (opn == 6) {
				// SWAPS 
				uint8_t *l = map[lnib % 8]; 

				int expectCycles = 8; 

				if (l) {
					*l = 0xF0; 
				} else {
					m_registers.HL(0x8000); 
					write_byte(0x8000, 0xF0); 
					expectCycles = 16;
				}

				loadValues(i, i);
				OPCODE(0xCB);

				if (l) {
					EXPECT(*l, 0x0F);
				}
				else {
					EXPECT(read_byte(0x8000), 0x0F);
				}

				CYCLES(expectCycles);
			} 
			else if (opn == 0) { // RLC (bits go into the C flag and don't wrap)
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				// 1000 0001 
				// 0000 0010 1 
				setup(0x81);
				OPCODE(0xCB);
				check(0x2);
				EXPECT(m_registers.m_flags.C, 1);

				OPCODE(0xCB);
				check(0x5);
				EXPECT(m_registers.m_flags.C, 0);

				CYCLES(expectCycles);
			}
			else if (opn == 1) { // RRC 
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				// 1000 0001 
				// 0100 0000 1 
				// 1010 0000
				setup(0x81);
				OPCODE(0xCB);
				check(0x40);
				EXPECT(m_registers.m_flags.C, 1);

				OPCODE(0xCB);
				check(0xA0);
				EXPECT(m_registers.m_flags.C, 0);
				CYCLES(expectCycles);
			}
			else if (opn == 2) { // RL 
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				setup(0x3);
				OPCODE(0xCB);
				auto c = check(6);
				if (strcmp(c, "") != 0) 
					return c; 

				m_registers.m_flags.C = 1; 
				setup(0x3);
				OPCODE(0xCB);
				c = check(7);
				if (strcmp(c, "") != 0) 
					return c; 

				// 1000 0001 
				// 0000 0010 1 
				m_registers.m_flags.C = 0; 
				setup(0x81);
				OPCODE(0xCB);
				c = check(2);
				if (strcmp(c, "") != 0) 
					return c; 

				EXPECT(m_registers.m_flags.C, 1);

				CYCLES(expectCycles);
			} else if (opn == 0x3) { // RR 
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				setup(0x81);
				m_registers.m_flags.C = 0;
				OPCODE(0xCB);
				auto c = check(0x40);
				if (strcmp(c, "") != 0) 
					return c; 
				EXPECT(m_registers.m_flags.C, 1);

				OPCODE(0xCB);
				c = check(0xA0);
				if (strcmp(c, "") != 0) 
					return c; 
				EXPECT(m_registers.m_flags.C, 0);

				CYCLES(expectCycles);				
			} else if (opn == 0x4) { // SLA 
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	


				setup(0x81);
				OPCODE(0xCB);
				check(0x02);
				EXPECT(m_registers.m_flags.C, 1);

				// 0x02 
				OPCODE(0xCB);
				check(0x04);
				EXPECT(m_registers.m_flags.C, 0);
				CYCLES(expectCycles);
			} else if (opn == 0x5) { // SRA 
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				setup(0x81);
				OPCODE(0xCB);
				check(0xC0);
				EXPECT(m_registers.m_flags.C, 1);

				// 0x02 
				OPCODE(0xCB);
				check(0x0A);
				EXPECT(m_registers.m_flags.C, 0);
				CYCLES(expectCycles);
			} else if (opn == 0x7) { // SRL
				int expectCycles = 8;
				auto setup = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						*l = v; 
					} else {
						m_registers.HL(0x8000); 
						write_byte(0x8000, v); 
						expectCycles = 16;
					}
					loadValues(i, i);
				};
				auto check = [&](int v) {
					uint8_t *l = map[lnib % 8]; 
					if (l) {
						EXPECT(*l, v)
					} else {
						EXPECT(read_byte(0x8000), v);
					}				
					CYCLES(expectCycles);	
				};	

				setup(0x81);
				OPCODE(0xCB);
				check(0x40);
				EXPECT(m_registers.m_flags.C, 1);

				OPCODE(0xCB);
				check(0x20);
				EXPECT(m_registers.m_flags.C, 0);	
				CYCLES(expectCycles);	
			}

			return "NYI";
		};
	}

	for (int i = 0x40; i < 0x80; i++) {
		const char *pmap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 

		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t off  = lnib > 7; 

		std::stringstream stm;
		stm << "BIT " << (((hnib-4) * 2) + off) << ", " << pmap[lnib % 8]; 

		v[stm.str()] = [&, i, hnib, lnib, off]() {
			uint8_t testBit = ((hnib - 4) * 2) + off; 

			uint8_t *t = map[lnib % 8]; 

			int expectCycles = 8; 

			if (t) {
				*t = 0xF0;
			} else {
				m_registers.HL(0x8000);
				write_byte(0x8000, 0xF0);
				expectCycles = 16; 
			}

			int expectBit = (0xF0 & (1 << testBit)) >> testBit;

			// run opcode
			loadValues(i, i); 
			OPCODE(0xCB);
			if (expectBit == 0) {
				EXPECT(m_registers.m_flags.Z, 1);
			} else {
				EXPECT(m_registers.m_flags.Z, 0);				
			}

			CYCLES(expectCycles);
		};
	}

	for (int i = 0x80; i < 0xC0; i++) {
		const char *pmap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 

		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t off  = lnib > 7; 

		std::stringstream stm;
		stm << "RES " << (((hnib-8) * 2) + off) << ", " << pmap[lnib % 8]; 

		v[stm.str()] = [&, i, hnib, lnib, off]() {
			uint8_t testBit = ((hnib - 4) * 2) + off; 

			uint8_t *t = map[lnib % 8]; 

			int expectCycles = 8; 

			if (t) {
				*t = 0xF0;
			} else {
				m_registers.HL(0x8000);
				write_byte(0x8000, 0xF0);
				expectCycles = 16; 
			}

			// run opcode
			loadValues(i, i); 
			OPCODE(0xCB);
			uint8_t v = (t ? *t : read_byte(0x8000));
			EXPECT((v & (1 << testBit)), 0);
			CYCLES(expectCycles);
		};
	}

	for (int i = 0xC0; i < 0x100; i++) {
		const char *pmap[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
		uint8_t *map[] = {&m_registers.B, &m_registers.C, &m_registers.D, &m_registers.E, 
			&m_registers.H, &m_registers.L, nullptr, &m_registers.A}; 

		uint8_t hnib = i >> 4; 
		uint8_t lnib = i & 0xF; 
		uint8_t off  = lnib > 7; 

		std::stringstream stm;
		stm << "SET " << (((hnib-0xC) * 2) + off) << ", " << pmap[lnib % 8]; 
	
		v[stm.str()] = [&, i, hnib, lnib, off]() {
			uint8_t testBit = ((hnib - 0xC) * 2) + off; 
			uint8_t *t = map[lnib % 8]; 

			int expectCycles = 8; 

			if (t) {
				*t = 0xF0;
			} else {
				m_registers.HL(0x8000);
				write_byte(0x8000, 0xF0);
				expectCycles = 16; 
			}

			loadValues(i, i); 
			OPCODE(0xCB);
			uint8_t v = (t ? *t : read_byte(0x8000));
			if ((v & (1 << testBit)) == 0) 
				return "Expected bit to be 1";
			CYCLES(expectCycles);
		};
	}

	int passed = 0; 
	int failed = 0; 
	int pending = 0;

	for (auto i : v) {
		printf("%s", KNRM);

		// rewrite 0 -> 0x8000 with 0 
		for (int i = 0; i < 0x8000; i++)
			write_byte(i, 0);

		last_cycles = 0;
		m_registers.reset(); 
		std::string name = i.first;

		std::string r; 

		try { 
			r = i.second(); 
		} catch (std::exception& e) {
			r = "CAUGHT EXCEPTION.";
		} 

		if (r.length() == 0) {
			passed++;
		} else if (r == "NYI") {
			printf("%s", KYEL);
			std::cout << "PENDING: " << name << std::endl; 
			pending++;
		} else {
			printf("%s", KRED);
			std::cout << "FAIL: " << name << " (" << r << ")" << std::endl; 
			failed++;
		}
	}

	printf("%s", KNRM);
	printf("PASSED: %d/%d\nFAILED: %d/%d\nPENDING: %d\n\n", passed, (passed+failed), failed, (passed+failed), pending);
}
