#include "MOS6532.h"
#include "EmulatorDebugger.h"
#include "glue.h"
#include "Board.h"

#define IO_PORT_A 0
#define IO_PORT_B 1
#define TIMER 4

// Initialize the MOS 6532
MOS6532* mos6532Create() {

   MOS6532* mos6532 = (MOS6532*)calloc(1, sizeof(MOS6532));

    // Clear RAM and ports
    for (int i = 0; i < MOS6532_RAM_SIZE; i++) {
        mos6532->ram[i] = 0;
    }
    mos6532->port_a = 0;
    mos6532->port_b = 0;
    mos6532->timer = 0;
    mos6532->timer_running = false;

    return mos6532;
}

// Emulate a read operation
UInt8 mos6532Read(MOS6532* mos6532, UInt16 port) {
    if (port < MOS6532_RAM_SIZE) {
        // Reading from RAM
        return mos6532->ram[port];
    } else if (port == IO_PORT_A) {
        // Reading from Port A
        return mos6532->port_a;
    } else if (port == IO_PORT_B) {
        // Reading from Port B
        return mos6532->port_b;
    } else if (port == TIMER) {
        // Reading the timer
        return mos6532->timer & 0xFF;
    }
    // Invalid read port
    return 0xFF;
}

// Emulate a write operation
void mos6532Write(MOS6532* mos6532, UInt16 port, UInt8 value) {
    if (port < MOS6532_RAM_SIZE) {
        // Writing to RAM
        mos6532->ram[port] = value;
    } else if (port == IO_PORT_A) {
        // Writing to Port A
        mos6532->port_a = value;
    } else if (port == IO_PORT_B) {
        // Writing to Port B
        mos6532->port_b = value;
    } else if (port == TIMER) {
        // Writing to timer, initialize and start it
        mos6532->timer = value;
        mos6532->timer_running = true;
    }
}

UInt8 mos6532Destroy(MOS6532* mos6532) {
  free(mos6532);
}

void mos6532Reset(MOS6532 *mos6532, UInt32 cpuTime) {

}


void mos6532Execute(MOS6532 *mos6532) {
  // Emulate the timer countdown
  if (mos6532->timer_running && mos6532->timer > 0) {
      mos6532->timer--;
      if (mos6532->timer == 0) {
          mos6532->timer_running = false;
          // Handle timer expiration event here if needed
          printf("Timer expired!\n");
      }
  }
}
