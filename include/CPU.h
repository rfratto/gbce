#ifndef __CPU_H__
#define __CPU_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <string>
#include "GPU.h"

class GPU;

class gameBoy {
	friend class GPU;
private:
	uint8_t *m_bios; 
	uint8_t *m_rom; 
	uint8_t m_vram[2][0x2000];
	uint8_t *m_eram;
	uint8_t m_wram[0x1000];
	uint8_t m_wramb[0x1000][8];
	uint8_t m_oam[0xA0];
	uint8_t m_hram[0x7F]; 


	uint8_t io_port(uint16_t addr, bool write, uint8_t value);
	
	void write_byte(uint16_t addr, uint8_t value);
	void write_word(uint16_t addr, uint16_t value);

	uint8_t get_byte();
	uint16_t get_word();

	uint8_t getParam(uint8_t opcode, uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t H, 
									 uint8_t L, uint8_t HL, uint8_t n);

	void STOP(uint8_t opcode);
	void DI(uint8_t opcode);
	void EI(uint8_t opcode);
	void SCF(uint8_t opcode);
	void DAA(uint8_t opcode);
	void POP(uint8_t opcode);
	void PUSH(uint8_t opcode);
	void RET(uint8_t opcode);
	void INC(uint8_t opcode); 
	void DEC(uint8_t opcode); 
	void CPL(uint8_t opcode);
	void ROT(uint8_t opcode);
	void CB(uint8_t opcode); 
	void XOR(uint8_t opcode);
	void NOP(uint8_t opcode);
	void RST(uint8_t opcode);
	void ADD(uint8_t opcode);
	void SUB(uint8_t opcode);
	void CP(uint8_t opcode);
	void CCF(uint8_t opcode);
	void ADC(uint8_t opcode);
	void SBC(uint8_t opcode);
	void AND(uint8_t opcode);
	void OR(uint8_t opcode);
	void CALL(uint8_t opcode);
	void JR(uint8_t opcode);
	void JP(uint8_t opcode);
	void LD(uint8_t opcode);

	void interrupt(uint16_t addr); 

	std::vector<std::function<void(gameBoy*,uint8_t)>> insmap;

	std::string saveFileName; 
	uint32_t saveBytes;
	bool shouldSave = false;
public:
	bool debug = true, bugfix = false;
	bool finish = false;
	struct {
	private: 
		uint8_t m_flags_v() {
			return *((uint8_t *)&m_flags) & 0xF0;
		}
		void m_flags_v(uint8_t v) {
			*((uint8_t *)&m_flags) = v & 0xF0;
		}
	public: 
		bool inBios;

		uint8_t IE = 0;
		uint8_t IF = 0;
		uint8_t IME; 
		uint8_t toggleIME = 0xFF;
		uint8_t joypad, joypadDir, joypadButt; 
		bool halted = false;

		// Timer registers 
		uint8_t DIV = 0;

		uint8_t A, B, C, D, E, H, L;
		uint16_t SP, PC; 

		uint8_t WRAMBank, VRAMBank;
		uint8_t ROMBank = 1, BankMode = 0, RAMBank = 0, RAMEnabled = 0; 
		uint8_t ClockRegister = 0;

		struct {
			uint8_t R : 4; // reserved (bits 0-4)
			uint8_t C : 1; 
			uint8_t H : 1; 
			uint8_t N : 1; 
			uint8_t Z : 1; 
		} m_flags; 

		struct {
			uint8_t S;
			uint8_t M; 
			uint8_t H;
			uint8_t DL; 
			uint8_t DH;
		} Clock, LatchedClock;

		uint8_t shades[4]; 

		void BC(uint16_t v) { B = (v >> 8); C = v & 0xFF; } 
		void DE(uint16_t v) { D = (v >> 8); E = v & 0xFF; } 
		void HL(uint16_t v) { H = (v >> 8); L = v & 0xFF; } 
		void AF(uint16_t v) { A = (v >> 8); m_flags_v(v & 0xFF); }

		uint16_t AF() { return (A << 8) | m_flags_v(); }
		uint16_t BC() { return (B << 8) | C; }
		uint16_t DE() { return (D << 8) | E; }
		uint16_t HL() { return (H << 8) | L; }

		void reset() {
			joypad = joypadDir = joypadButt = 0x0F;
			WRAMBank = 1;
			VRAMBank = 0;
			inBios = true; 
			toggleIME = 0xFF;
			IE = 0;

			A = 0; B = 0; C = 0; D = 0; E = 0; H = 0; L = 0;
			SP = 0; PC = 0;
			IME = 1; 
			IF = 0;

			m_flags.Z = 0; m_flags.N = 0; m_flags.H = 0; m_flags.C = 0; m_flags.R = 0;
			testingOffset = 0;
		}

		uint8_t type = 0;
		uint8_t romSz = 0;
		uint8_t ramSz = 0;  

		uint8_t testingOffset = 0; 
		uint8_t tbytes[2];  
	} m_registers; 
	
	std::string last = "";
	int last_cycles; 
	GPU *gpu;

	void test();
	void reset(const char *bios_file, const char *rom_file);
	void save();
	void ins();
	void dump();
	void keyEvent(int key, int action);
	void tickTimer();

	uint8_t read_byte(uint16_t addr);
	uint16_t read_word(uint16_t addr);

	~gameBoy();
	gameBoy();
};

#endif 