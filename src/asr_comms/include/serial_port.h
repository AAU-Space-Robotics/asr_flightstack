#pragma once

#include "transport.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

class SerialPort : public ITransport {
public:
    SerialPort(const std::string& device, int baud_rate)
    : speed_(to_speed(baud_rate))
    {
        fd_ = open_and_configure(device, speed_);
        if (fd_ < 0)
            throw std::runtime_error("Failed to open " + device + ": " + std::strerror(errno));
    }

    ~SerialPort() { if (fd_ >= 0) close(fd_); }
    SerialPort(const SerialPort&)            = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    ssize_t recv(void* buf, size_t len) override { return ::read(fd_, buf, len); }
    ssize_t send(const void* buf, size_t len) override { return ::write(fd_, buf, len); }

    // A USB unplug hangs up the tty: read() then returns 0 just like a normal
    // VMIN/VTIME timeout, but ioctls start failing — use that to tell a dead
    // link apart from a quiet one.
    bool alive() const
    {
        termios tty{};
        return tcgetattr(fd_, &tty) == 0;
    }

    // Re-point this port at a (re-)enumerated device after an unplug. dup2()
    // swaps the descriptor in place, so concurrent recv()/send() from other
    // threads migrate from the hung-up device to the new one without ever
    // seeing a closed fd. Returns false if the device is absent or not ready.
    bool reopen(const std::string& device)
    {
        const int nfd = open_and_configure(device, speed_);
        if (nfd < 0) return false;
        dup2(nfd, fd_);
        close(nfd);
        return true;
    }

private:
    int     fd_{-1};
    speed_t speed_;

    static int open_and_configure(const std::string& device, speed_t speed)
    {
        const int fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return -1;

        // Switch back to blocking after open to avoid ENXIO on some drivers
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        termios tty{};
        if (tcgetattr(fd, &tty) != 0) {
            close(fd);
            return -1;
        }

        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);
        cfmakeraw(&tty);
        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 1;  // 100 ms read timeout — lets recv_loop check running_ regularly

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            close(fd);
            return -1;
        }
        return fd;
    }

    static speed_t to_speed(int baud)
    {
        switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default: throw std::runtime_error("Unsupported baud rate: " + std::to_string(baud));
        }
    }
};
