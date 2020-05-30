#include <iostream>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()


// Reference: https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/

int main() {

    int serial_port = open("/dev/ttyUSB0", O_RDWR);

    // Create new termios struc, we call it 'tty' for convention
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    // Read in existing settings, and handle any error
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }

    // Check for errors
    if (serial_port < 0) {
        // errno 2 = File not exists
        // errno 13 = Permission denied
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag |= PARENB;  // Set parity bit, enabling parity

    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication

    tty.c_cflag |= CS5; // 5 bits per byte
    tty.c_cflag |= CS6; // 6 bits per byte
    tty.c_cflag |= CS7; // 7 bits per byte
    tty.c_cflag |= CS8; // 8 bits per byte (most common)

    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control

    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON; // Disabling canonical mode

    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo

    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    /**
     * Input modes
     */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

    /**
     * Clearing all of the following bits disables any special handling of the bytes as they are received by the serial port,
     * before they are passed to the application. We just want the raw data thanks!
     */
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    /**
     * Output Modes (c_oflag)
     *
     * The c_oflag member of the termios struct contains low-level settings for output processing. When configuring a serial port,
     * we want to disable any special handling of output chars/bytes, so do the following:
     */
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)
    /**
     * Both OXTABS and ONOEOT are not defined in Linux. Linux however does have the XTABS field which seems to be related.
     * When compiling for Linux, I just exclude these two fields and the serial port still works fine.
     */

    /**
     * VMIN and VTIME (c_cc)
     *
     * An important point to note is that VTIME means slightly different things depending on what VMIN is. When VMIN is 0,
     * VTIME specifies a time-out from the start of the read() call. But when VMIN is > 0,
     * VTIME specifies the time-out from the start of the first received character.
     *
     * VMIN = 0, VTIME = 0: No blocking, return immediately with what is available
     *
     * VMIN > 0, VTIME = 0: This will make read() always wait for bytes (exactly how many is determined by VMIN),
     * so read() could block indefinitely.
     *
     * VMIN = 0, VTIME > 0: This is a blocking read of any number chars with a maximum timeout (given by VTIME).
     * read() will block until either any amount of data is available, or the timeout occurs. This happens to be my
     * favourite mode (and the one I use the most).
     *
     * VMIN > 0, VTIME > 0: Block until either VMIN characters have been received, or VTIME after first character has elapsed.
     * Note that the timeout for VTIME does not begin until the first character is received.
     */
    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    /**
     * Baud Rate
     * Available baud rates:
     * B0,  B50,  B75,  B110,  B134,  B150,  B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800
     */

    // the most common baudrates are: B9600, B57600 and B115200
    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }
}
