#ifndef __GPU_H__
#define __GPU_H__

#include "CPU.h" 
#include "Graphics.h"

class gameBoy;
class Graphics;

class GPU {
	friend class gameBoy;
private:

	gameBoy *m_cpu; 

	unsigned int clocks = 0;
	unsigned int mode = 0; 

	struct {
		bool GBMode = 1;
		/*
			LCDC: bit 7 -> 0 
				7: LCD Display Enable (0=off)
				6: Window Tile Map Display select (0=9800,1=9c00)
				5: Window Display Enable (0=Off)
				4: BG & Window Tile Data Select (0=8800, 1=8000)
				3: BG Tile Map Display Select (0=9800, 1=9c00)
				2: Sprite size (0=8x8,1=8x16)
				1: Sprite Display Enable (0=Off,1=On)
				0: BG Display (0=off)
					when turned off, background becomes blank (white)
		*/
		uint8_t LCDC = 0; // FF40 (R/W) LCD Control 
		uint8_t STAT = 0; // FF41 (R/W) LCDC Status  

		uint8_t SCY = 0; // FF42 R/W: scroll Y 
		uint8_t SCX = 0; // FF43 R/W: scroll X 

		uint8_t LY = 0; // FF44 R: LCD y-coordinate (line, I think)
		uint8_t LYC = 0; // FF45 R/W: when LYC == LY, STAT interrupt 
			
		uint8_t WY = 0; // 44FA window Y position 
		uint8_t WX = 0; // 44FB window X position (minus 7)

		// 0 = 0%, 1 = 33%, 2 = 66%, 3 = 100% 
		uint8_t BGP = 0; // FF47 (R/W): BG Palette Data 
		uint8_t OBP0 = 0; // FF48 (R/W): Object Palette 0 Data 
		uint8_t OBP1 = 0; // FF49 (R/W): Object Palette 1 Data

		uint8_t colors[8][4][2]; // npalettes * ncolors * (rgb).length
		uint8_t coloridx = 0; 
		uint8_t colorinc = 0;   

		uint8_t scolors[8][4][2]; // npalettes * ncolors * (rgb).length
		uint8_t scoloridx = 0; 
		uint8_t scolorinc = 0;   
	} m_registers;

	void scanline();
	void debug_refresh();

	uint8_t *getColors(uint8_t color, uint8_t *palette);
public:
	Graphics *graphics; 

	void write(uint16_t addr, uint8_t value);
	uint8_t read(uint16_t addr);

	// increments clocks by 1, handles stuff. 
	void keyEvent(GLFWwindow *win, int key, int scancode, int action, int mods);
	void tick(); 

	GPU(gameBoy *CPU); 
}; 

#endif 