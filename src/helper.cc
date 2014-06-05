#include "helper.h"

struct MBCInfo getMBCInfo(uint8_t cartType) {
	struct MBCInfo ret; 
	ret.MBCType = 0;
	ret.hasRAM = false;
	ret.hasBattery = false;
	ret.hasTimer = false;
	ret.hasRumble = false; 

	switch (cartType) {
		case 0x9:
			ret.hasBattery = true; 
		case 0x8:
			ret.hasRAM = true; 
		case 0x0:
			ret.MBCType = TYPE_ROM_ONLY; 
			break;
		
		case 0x3:
			ret.hasBattery = true; 
		case 0x2:
			ret.hasRAM = true; 
		case 0x1: 
			ret.MBCType = TYPE_MBC1; 
			break; 

		case 0x6:
			ret.hasBattery = true; 
		case 0x5: 
			ret.MBCType = TYPE_MBC2; 
			break; 

		case 0xD:
			ret.hasBattery = true; 
		case 0xC:
			ret.hasRAM = true; 
		case 0xB: 
			ret.MBCType = TYPE_MMM01; 
			break; 

		case 0x10:
			ret.hasRAM = true; 
		case 0x0F: 
			ret.hasBattery = true;
			ret.hasTimer = true;
			ret.MBCType = TYPE_MBC3; 
			break;

		case 0x13:
			ret.hasBattery = true;
		case 0x12:
			ret.hasRAM = true; 
		case 0x11:
			ret.MBCType = TYPE_MBC3;
			break;

		case 0x17:
			ret.hasBattery = true;
		case 0x16:
			ret.hasRAM = true; 
		case 0x15:
			ret.MBCType = TYPE_MBC4;
			break;

		case 0x1B:
			ret.hasBattery = true;
		case 0x1A:
			ret.hasRAM = true; 
		case 0x19:
			ret.MBCType = TYPE_MBC5;
			break;

		case 0x1E:
			ret.hasBattery = true;
		case 0x1D:
			ret.hasRAM = true; 
		case 0x1C:
			ret.hasRumble = true;
			ret.MBCType = TYPE_MBC3;
			break;
	}

	return ret;
}

bool bitSet(uint8_t value, uint8_t bit) {
	return ((value & (1 << bit)) > 0) ? 1 : 0;
}