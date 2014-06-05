const uint8_t CART_TYPE_ROM_ONLY 							= 0;
const uint8_t CART_TYPE_ROM_MBC1 							= 1;
const uint8_t CART_TYPE_ROM_MBC1_RAM 					= 2;
const uint8_t CART_TYPE_ROM_MBC1_RAM_BATTERY 	= 3;
const uint8_t CART_TYPE_ROM_MBC2 							= 5;
const uint8_t CART_TYPE_ROM_MBC2_BATTERY 			= 6;

const uint8_t LANGUAGE_JAPANESE 							= 0;
const uint8_t LANGUAGE_ENGLISH 								= 1;

struct GBGame {
	uint8_t magic[4]; 
	uint8_t logo[48];
	uint8_t title[16];
	uint8_t manufacturer[2];
	uint8_t unused; 
	uint8_t type;
	uint8_t rom_size; 
	uint8_t ram_size;
	uint8_t language; 
	uint8_t manufacturer_ext; 
	uint8_t version;
	uint8_t compliment_check;
	uint8_t checksum;
} __attribute__ ((packed)); 