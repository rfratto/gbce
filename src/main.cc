/*
** gbce - Game Boy Color Emulator
** main.cc
**		provides entry function
** Copyright Robert Fratto 2014.
*/
#include <iostream>
#include "CPU.h"
#include "Graphics.h"
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <sstream>

gameBoy *gb;

void leave() { delete gb; }

void sig_handler(int signo) {
	 void* callstack[128];
   int i, frames = backtrace(callstack, 128);
   char** strs = backtrace_symbols(callstack, frames);
   for (i = 0; i < frames; ++i) {
       printf("%s\n", strs[i]);
   }
   free(strs);

	printf("EXCEPTION!\n");
	leave();
	_exit(1);
}

void keyEvent(GLFWwindow *win, int key, int scancode, int action, int mods) {
  gb->keyEvent(key, action);
}

int main(int argc, char **argv) {
	atexit(leave);
	signal(SIGSEGV, sig_handler);
	signal(SIGINT, sig_handler);

	gameBoy *GameBoy = new gameBoy;
	gb = GameBoy;
	GameBoy->reset("files/gb_bios.bin", argv[1]);

	#if TESTF
		gb->test();
		return 0;
	#endif

	Graphics graphics;
	graphics.setKeyCB(keyEvent);
	GameBoy->gpu->graphics = &graphics;

	GameBoy->debug = false;

	int oinst = 50000;
	int instructions = oinst;
	int breakpoint = -1;
	bool mcondition = true;
	graphics.run([&GameBoy, &instructions, &graphics, &oinst, &breakpoint, &mcondition]() {
    int repeats = 8825;
    repeats = 88250;
    for (int i = 0; i < repeats; i++) {
			if (GameBoy->last_cycles > 0) {
				GameBoy->last_cycles--;
				continue;
			}

			if (breakpoint > 0 && GameBoy->m_registers.PC == breakpoint) {
				printf("BREAKPOINT! %04X\n", GameBoy->m_registers.DE());
				GameBoy->debug = true;
			}

			GameBoy->ins();

			if (GameBoy->debug == true) {
				std::cout << GameBoy->last;
				bool acceptedInput = false;
				while (acceptedInput == false) {
					std::string input;
					std::cout << "AWAITING INPUT: ";
					getline(std::cin, input);

					if (input == "c") {
						acceptedInput = true;
						GameBoy->debug = false;
						std::cout << "Continuing...\n";
					}
					else if (input.substr(0,1) == "b") {
						uint16_t v;
						std::stringstream ss;
						ss << std::hex << input.substr(1);
						ss >> v;
						breakpoint = v;
						std::cout << "Setting breakpoint to $" << std::hex << v << std::endl;
					}
					else if (input == "i") { // prints out last instruction
						std::cout << GameBoy->last;
					}
					else if (input == "p") { // print out status
						GameBoy->dump();
					}
					else if (input == "s") {
						acceptedInput = true;
					} else if (input == "q") {
						std::cout << "Exiting...\n";
						exit(1);
					} else if (input.substr(0,2) == "xb") {
						uint16_t v;
						std::stringstream ss;
						ss << std::hex << input.substr(2);
						ss >> v;
						std::cout << "Value of $" << input.substr(3) << ": ";
						printf("$%04X\n", GameBoy->read_byte(v));
					} else if (input.substr(0,2) == "xw") {
						uint16_t v;
						std::stringstream ss;
						ss << std::hex << input.substr(2);
						ss >> v;
						std::cout << "Value of $" << input.substr(3) << ": ";
						printf("$%04X\n", GameBoy->read_word(v));
					} else {
						std::cout << "Unknown command \"" << input << "\"\n";
					}
				}
			}
			

			if (GameBoy->finish == true)
				graphics.stop();

			GameBoy->gpu->tick();
		}

		GameBoy->save();
	}, [&]() {
		GameBoy->tickTimer();
	});

	return 0;
}
