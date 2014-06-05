#include "GPU.h"
#include "helper.h"
#include <stdarg.h>

GPU::GPU(gameBoy *CPU) {
	m_cpu = CPU; 
	graphics = NULL;

	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 2; k++)
				m_registers.colors[i][j][k] = m_registers.scolors[i][j][k] = 0xFF; 


	mode = 0;
	m_registers.STAT = (m_registers.STAT & 0xFC) | 0x0; // set mode flag 

}

void GPU::write(uint16_t addr, uint8_t value) {
	switch (addr) {
		case 0xFF40:
			m_registers.LCDC = value; 
			break;
		case 0xFF41:
			m_registers.STAT = value & (0xFF ^ 3); 
			break; 
		case 0xFF42:
			m_registers.SCY = value; 
			break; 
		case 0xFF43:
			m_registers.SCX = value; 
			break; 
		case 0xFF45:
			if (value > 153) value = 153;
			m_registers.LYC = value; 
			break; 
		case 0xFF4A:
			m_registers.WY = value; 
			break; 
		case 0xFF4B:
			m_registers.WX = value; 
			break; 
		case 0xFF47:
			m_registers.BGP = value; 
			break; 
		case 0xFF48:
			m_registers.OBP0 = value & 0xFF; 
			break; 
		case 0xFF49:
			m_registers.OBP1 = value & 0xFF; 
			break; 
		case 0xFF4F:
			m_cpu->m_registers.VRAMBank = value;
			break;
		case 0xFF4C:
			if (m_cpu->m_registers.inBios == true) {
				m_registers.GBMode = (value == 0x4);
				m_cpu->m_registers.VRAMBank = 0;
				m_cpu->m_registers.WRAMBank = 1;
			}
		break;
	}

	if (m_registers.GBMode == 1) return;
	if (addr == 0xFF68) {
		uint8_t idx = value & 0x1F; 
		uint8_t inc = ((value & 0x80) > 0); 	
  	
  	m_registers.coloridx = idx; 
  	m_registers.colorinc = inc; 
	} else if (addr == 0xFF69) {
		uint8_t idx = m_registers.coloridx; 

		uint8_t paln = idx/8;
		uint8_t coln = (idx/2)%4;
		uint8_t byten = idx%2;  

		m_registers.colors[paln][coln][byten] = value & (byten ? 0x7F : 0xFF);
		if (m_registers.colorinc == 1) m_registers.coloridx++;
	} else if (addr == 0xFF6A) {
		uint8_t idx = value & 0x1F; 
		uint8_t inc = ((value & 0x80) > 0); 	
  	
  	m_registers.scoloridx = idx; 
  	m_registers.scolorinc = inc; 
	} else if (addr == 0xFF6B) {
		uint8_t idx = m_registers.scoloridx; 

		uint8_t paln = idx/8;
		uint8_t coln = (idx/2)%4;
		uint8_t byten = idx%2;  

		m_registers.scolors[paln][coln][byten] = value & (byten ? 0x7F : 0xFF);
		if (m_registers.scolorinc == 1) m_registers.scoloridx++;
	}
}

uint8_t GPU::read(uint16_t addr) {
	switch (addr) {
		case 0xFF40:
			return m_registers.LCDC; 
			break;
		case 0xFF41:
			return m_registers.STAT; 
			break; 
		case 0xFF42:
			return m_registers.SCY; 
			break; 
		case 0xFF43:
			return m_registers.SCX; 
			break; 
		case 0xFF44:
			return m_registers.LY; 
			break; 
		case 0xFF45:
			return m_registers.LYC; 
			break; 
		case 0xFF4A:
			return m_registers.WY; 
			break; 
		case 0xFF4B:
			return m_registers.WX; 
			break; 
		case 0xFF47:
			return m_registers.BGP; 
			break; 
		case 0xFF48:
			return m_registers.OBP0; 
			break; 
		case 0xFF49:
			return m_registers.OBP1; 
			break; 
		case 0xFF4F:
			return m_cpu->m_registers.VRAMBank;
			break;
	}

	return 0;
}

void GPU::scanline() {
	uint16_t BGMapLoc = ((m_registers.LCDC & (1 << 3)) > 0) ? 0x9c00 : 0x9800;
	uint16_t WinMapLoc = (bitSet(m_registers.LCDC, 6)) ? 0x9c00 : 0x9800;
	uint16_t TileLoc = ((m_registers.LCDC & (1 << 4)) > 0) ? 0x8000 : 0x8800;
	BGMapLoc -= 0x8000;
	WinMapLoc -= 0x8000;
	TileLoc -= 0x8000;

	uint8_t X = m_registers.SCX;
	uint8_t Y = (m_registers.SCY + m_registers.LY) % 256; 
	uint8_t spriteSize = ((m_registers.LCDC & 0x4) > 0);

	uint8_t WX = m_registers.WX - 7;
	uint8_t WY = m_registers.WY;

	uint8_t bg_colors[160];

	if (m_registers.GBMode == 0) {
		for (int i = 0; i < 160; i++) {
			uint8_t curX = (X + i) % 160;

			uint16_t tileoff = (32 * (Y/8)) + (curX/8);
			uint16_t tileidx = *(m_cpu->m_vram[0] + BGMapLoc + tileoff); 
			uint16_t tileattr = *(m_cpu->m_vram[1] + BGMapLoc + tileoff); 

			if (TileLoc + 0x8000 == 0x8800) {
				tileidx += 128; 
			}

			uint8_t bankNum = (tileattr >> 3) & 1; 
			uint8_t palNum = tileattr & 0x7;

			uint8_t *tile = (m_cpu->m_vram[bankNum] + TileLoc + (tileidx * 0x10)); 
			// next 16 bytes are for the tile 

			uint8_t tile_x = curX % 8;
			uint8_t tile_y = (Y % 8); 

			uint8_t tile_left = tile[(tile_y * 2) + 0]; 
			uint8_t tile_right = tile[(tile_y * 2) + 1]; 

			uint8_t pixel_left = (tile_left >> (7 - tile_x)) & 1; 
			uint8_t pixel_right = (tile_right >> (7 - tile_x)) & 1;

			uint8_t color = (pixel_right << 1) | pixel_left; 

			uint8_t *colors = m_registers.colors[palNum][color];
			uint16_t RGB = (colors[1] << 8) | colors[0]; 
			RGB &= 0x7FFF; 

			uint8_t red = RGB & 0x1F; 
			uint8_t green = (RGB >> 5) & 0x1F; 
			uint8_t blue = (RGB >> 10) & 0x1F; 

			uint8_t pcolor[3] = {(uint8_t)((red/31.0f)*255.0f), (uint8_t)((green/31.0)*255.0f), (uint8_t)((blue/31.0)*255.0f)};
			graphics->plotPoint(i, m_registers.LY, pcolor);		

		}
	} else {
		for (int i = 0; i < 160; i++) {
			uint16_t curX = (X + i) % 256;

			uint16_t tileoff = (32 * (Y/8)) + (curX/8);
			uint16_t tileidx = *(m_cpu->m_vram[0] + BGMapLoc + tileoff); 

			if (TileLoc + 0x8000 == 0x8800) {
				tileidx = (int8_t)tileidx + 128; 
			}

			uint8_t *tile = (m_cpu->m_vram[0] + TileLoc + (tileidx * 0x10)); 
			// next 16 bytes are for the tile 

			uint8_t tile_x = (curX % 8);
			uint8_t tile_y = (Y % 8); 

			uint8_t tile_left = tile[(tile_y * 2) + 0]; 
			uint8_t tile_right = tile[(tile_y * 2) + 1]; 

			uint8_t pixel_left = (tile_left >> (7 - tile_x)) & 1; 
			uint8_t pixel_right = (tile_right >> (7 - tile_x)) & 1;

			uint8_t color = (pixel_right << 1) | pixel_left; 
			bg_colors[i] = color;

			uint8_t *pcolor = getColors(color, &m_registers.BGP);
			graphics->plotPoint(i, m_registers.LY, pcolor);		
			delete pcolor;

			if (bitSet(m_registers.LCDC, 5)) { // Draw window 
				if (WY <= m_registers.LY && WX <= i) {
					uint8_t X = (i - WX);
					uint8_t Y = (m_registers.LY - WY);

					uint16_t tileoff = (32 * (Y/8)) + (X/8);
					uint16_t tileidx = *(m_cpu->m_vram[0] + WinMapLoc + tileoff); 

					if (TileLoc + 0x8000 == 0x8800) 
						tileidx = (int8_t)tileidx + 128; 

					uint8_t *tile = (m_cpu->m_vram[0] + TileLoc + (tileidx * 0x10)); 

					uint8_t tile_x = (X % 8);
					uint8_t tile_y = (Y % 8); 

					uint8_t tile_left = tile[(tile_y * 2) + 0]; 
					uint8_t tile_right = tile[(tile_y * 2) + 1]; 

					uint8_t pixel_left = (tile_left >> (7 - tile_x)) & 1; 
					uint8_t pixel_right = (tile_right >> (7 - tile_x)) & 1;

					uint8_t color = (pixel_right << 1) | pixel_left; 
					bg_colors[i] = color;

					uint8_t *pcolor = getColors(color, &m_registers.BGP);
					graphics->plotPoint(i, m_registers.LY, pcolor);		
					delete pcolor;
				} 
			}
		}

		/*
		**
		** draw sprites 
		**
		*/ 
		uint8_t spritesShown = 0;
		for (int j = 0; j < 40; j++) {
			if (spritesShown > 10) break; 
			uint8_t *sprite = (m_cpu->m_oam + (j * 4));
			// Y = 0 == 16, so add 16 

			uint8_t spriteY = sprite[0] - 16; 
			uint8_t spriteX = sprite[1] - 8; 
			uint8_t spriteTile = sprite[2]; 
			uint8_t spriteAttrib = sprite[3]; 
			bool behindBG = (spriteAttrib & (1 << 7)) > 0;
			uint8_t palNum = ((spriteAttrib & 0x80) == 1); 

			int height = (spriteSize == 1) ? 16 : 8;   

			if (spriteY <= m_registers.LY && (spriteY+height) > m_registers.LY) {
				spritesShown++;

				for (int k = 0; k < 8; k++) {
					// draw all X values 
					uint8_t *tile = (m_cpu->m_vram[0] + (spriteTile * 0x10)); 

					uint8_t tile_x = k;
					uint8_t tile_y = (m_registers.LY - spriteY);

					if ((spriteAttrib & (1 << 6)) > 0) { // vertical flip
						tile_y = 7 - tile_y;
					}

					uint8_t tile_left = tile[(tile_y * 2) + 0];
					uint8_t tile_right = tile[(tile_y * 2) + 1]; 

					uint8_t pixel_left = (tile_left >> (7 - tile_x)) & 1; 
					uint8_t pixel_right = (tile_right >> (7 - tile_x)) & 1;	

					if ((spriteAttrib & (1 << 5)) > 0) { // horizontal flip
						pixel_left = (tile_left >> tile_x) & 1; 
						pixel_right = (tile_right >> tile_x) & 1;	
					}

					uint8_t color = (pixel_right << 1) | pixel_left;
					if (color == 0) continue; // 0 is transparant 
					if (behindBG == true && bg_colors[spriteX + k] != 0) continue;

					uint8_t *pcolor = getColors(color, palNum ? &m_registers.OBP1 : &m_registers.OBP0);
					graphics->plotPoint(spriteX + k, m_registers.LY, pcolor);		
					delete pcolor;
				}
			}
		}
	}
}

uint8_t *GPU::getColors(uint8_t color, uint8_t *palette) {
	uint8_t *ret = new uint8_t[3]; 

	uint8_t shade = (*palette >> (color * 2)) & 3; 
	uint8_t value = 0;

	switch (shade) {
		case 0:
			value = 255;
			break;
		case 1:
			value = 168;
			break;
		case 2:
			value = 84;
			break;
		case 3:
			value = 0; 
			break;
	}

	ret[0] = ret[1] = ret[2] = value;
	return ret;
}

void GPU::tick() {
	clocks += (m_cpu->last_cycles ? m_cpu->last_cycles : 3);
	switch (mode) {
		case 2:
			// reading from OAM
			if (clocks >= 80) {
				clocks = 0;  
				mode = 3; 
				m_registers.STAT = (m_registers.STAT & 0xFC) | 0x3; // set mode flag 
			} 
			break; 
		case 3:
			// OAM and VRAM read. 
			if (clocks >= 172) {
				if (bitSet(m_registers.STAT, 3)) {
					m_cpu->m_registers.IF |= (1 << 1);
				}

				clocks = 0; 
				mode = 0; 
				m_registers.STAT = (m_registers.STAT & 0xFC) | 0x0; // set mode flag 

				scanline(); 
			}
			break;
		case 0:
			if (clocks >= 204) {
				clocks = 0;
				m_registers.LY++; 

				if (m_registers.LY == 144) { // enter vblank
					mode = 1; 

					if (bitSet(m_registers.STAT, 4)) {
						m_cpu->m_registers.IF |= (1 << 1);
					}

					// update data on screen 
					graphics->swapBuffers();
					m_cpu->m_registers.IF |= 1;
				} else {

					if (bitSet(m_registers.STAT, 5)) {
						m_cpu->m_registers.IF |= (1 << 1);
					}

					mode = 2; 
					m_registers.STAT = (m_registers.STAT & 0xFC) | 0x2; // set mode flag 
				}
			}
			break; 
		case 1: 
			if (clocks >= 456) {
				clocks = 0;
				m_registers.LY++; 

				if (m_registers.LY > 153) {
					if (bitSet(m_registers.STAT, 5)) {
						m_cpu->m_registers.IF |= (1 << 1);
					}

					mode = 2; 
					m_registers.LY = 0; 
					m_registers.STAT = (m_registers.STAT & 0xFC) | 0x2; // set mode flag 
				}
			}
			break; 
	}

	if (m_registers.LY == m_registers.LYC) {
		m_registers.STAT |= (1 << 2); // set bit 2 (coincidence flag)
		if ((m_registers.STAT & (1 << 6)) > 0) {
			m_cpu->m_registers.IF |= (1 << 1);
		}
	} else {
		m_registers.STAT &= (0xFF ^ (1 << 2)); // unset bit 2 (coincidence flag)
	}

}