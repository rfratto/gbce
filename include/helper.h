#ifndef __HELPER_H__ 
#define __HELPER_H__

#include <stdint.h>

#define TYPE_ROM_ONLY 0
#define TYPE_MBC1 		1
#define TYPE_MBC2 		2
#define TYPE_MMM01 		3
#define TYPE_MBC3 		4
#define TYPE_MBC4	 		5
#define TYPE_MBC5 		6

struct MBCInfo {
public:
	int MBCType; 
	bool hasRAM; 
	bool hasBattery;
	bool hasTimer; 
	bool hasRumble;
};

struct MBCInfo getMBCInfo(uint8_t cartType); 

bool bitSet(uint8_t value, uint8_t bit);

#endif 