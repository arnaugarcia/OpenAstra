#include <SPI.h>
#include <MCP2515_sw_can.h>

// Pin definitions specific to how the MCP2515 is wired up.
#ifdef MACCHINA_M2
#define CS_PIN  SPI0_CS3
#define INT_PIN SWC_INT
#else
#define CS_PIN  34
#define INT_PIN 38
#endif

// Create CAN object with pins as defined
SWcan SCAN(CS_PIN, INT_PIN);

void CANHandler() {
	SCAN.intHandler();
}

void setup() {
	SerialUSB.begin(115200);

	SerialUSB.println("Initializing ...");

	// Set up SPI Communication
	// dataMode can be SPI_MODE0 or SPI_MODE3 only for MCP2515
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);
	SPI.begin();

	// Initialize MCP2515 CAN controller at the specified speed and clock frequency
	// (Note:  This is the oscillator attached to the MCP2515, not the Arduino oscillator)
	// speed in KHz, clock in MHz
	SCAN.setupSW(33);

	attachInterrupt(INT_PIN, CANHandler, FALLING);
	SCAN.InitFilters(true);
  SCAN.mode(3);

	SerialUSB.println("Ready ...");
}

String serialData; // for incoming serial data

// CAN message frame (actually just the parts that are exposed by the MCP2515 RX/TX buffers)
CAN_FRAME message;

void loop() {

	// send data only when you receive data:
  if (SerialUSB.available() > 0) {
    // read the incoming byte:
    serialData = SerialUSB.readString();
    serialData.trim();

    SerialUSB.println(serialData);

    // SCAN.mode(2); //use HV Wakeup sending
    // delay(5);

    if (serialData == String("unlock")) {
        unlockdoors();
    } else if (serialData == String("lock")) {
      lockdoors();
    }

  }
  enableFilter();

}

void enableFilter() {
  if (SCAN.GetRXFrame(message)) {
    // Print message
    if (message.id == 0x160) {
      printFrame(message);
    }
    if (message.id == 0x230) {
      if (message.data.bytes[0] == 0x05 && message.data.bytes[5] == 0x09) {
        SerialUSB.println("Car locked");
      }
      if (message.data.bytes[0] == 0x00 && message.data.bytes[0] == 0x00) {
        SerialUSB.println("Car unlocked");
      }
    }
  }
}

void printFrame(CAN_FRAME message) {
      SerialUSB.print("#");
      SerialUSB.print(message.id, HEX);
      SerialUSB.print(" -> ");
      SerialUSB.print("(");
      SerialUSB.print(message.length, DEC);
      SerialUSB.print(") ");
      for(int i=0;i<message.length;i++) {
        SerialUSB.print(message.data.byte[i], HEX);
        SerialUSB.print(" ");
      }
      SerialUSB.println("");
}

void unlockdoors() {
  SerialUSB.println("Unlocking ...");
  message.id = 0x160;
  message.rtr = 0;
  message.extended = 0;
  message.length = 4;
  message.data.byte[0] = 0x02;
  message.data.byte[1] = 0x10;
  message.data.byte[2] = 0xEA;
  message.data.byte[3] = 0xE4;
  printFrame(message);
  sendMessage(message);
}

// 305 -> 0x00 0x00 0x00 0x00 0x10 0x80 0x00 -> not pressed
// 305 -> 0x00 0x00 0x00 0x00 0x10 0x80 0x01 -> not pressed

// 160 0x02 0x00 0xEA 0xE4 -> default state

// 160 0x02 0x80 0xEA 0xE4 -> right click
// 160 0x02 0xC0 0xEA 0xE4 -> right click (long press)

// 160 0x02 0x40 0xEA 0xE4 -> right click (lock car) ??

// 160 0x02 0x10 0xEA 0xE4 -> left click (unlock)
// 160 0x02 0x30 0xEA 0xE4 -> left click (long press)


void lockdoors() {
  SerialUSB.println("Locking ...");
  message.id = 0x160;
  message.rtr = 0;
  message.extended = 0;
  message.length = 4;
  message.data.byte[0] = 0x02;
  message.data.byte[1] = 0x40;
  message.data.byte[2] = 0xEA;
  message.data.byte[3] = 0xE4;
  sendMessage(message);
}

void sendMessage(CAN_FRAME message) {
  SCAN.mode(2); //use HV Wakeup sending
  SCAN.EnqueueTX(message);
  delay(5);
}