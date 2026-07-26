#pragma once

#include <cstddef>
#include <sys/types.h>

struct ITransport {
    virtual ~ITransport() = default;
    virtual ssize_t recv(void* buf, size_t len) = 0;
    virtual ssize_t send(const void* buf, size_t len) = 0;
};

// Which physical link(s) send_mavlink() should use. Radio is the shared,
// low-bandwidth SiK link that also carries telemetry -- large or
// WiFi-specific payloads (e.g. mission plan uploads) should pass WifiOnly
// so they never compete with it.
enum class LinkTarget { Both, WifiOnly, RadioOnly };
