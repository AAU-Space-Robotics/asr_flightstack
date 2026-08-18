#pragma once

// Mission plan upload/status protocol -- carried as MAVLINK_MSG_ID_V2_EXTENSION
// payloads sent WiFi-only (LinkTarget::WifiOnly, see transport.h), since a
// plan blob doesn't fit in one 249-byte V2_EXTENSION payload and must never
// compete with the low-bandwidth SiK telemetry radio.
//
// One MissionFragHeader-prefixed fragment per V2_EXTENSION message. Small
// payloads (the start command, a status update) still go through this same
// path as a single-fragment transfer, so both ends only need one reassembly
// routine regardless of message kind.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

#pragma pack(push, 1)
struct MissionFragHeader {
    uint32_t transfer_id;   // caller-assigned, incremented per logical send
    uint16_t frag_idx;      // 0-based
    uint16_t total_frags;
    uint32_t total_size;    // bytes in the full reassembled blob
    uint16_t payload_size;  // bytes carried in this fragment
    uint32_t crc32;         // over the full reassembled blob
};
#pragma pack(pop)

// Leaves this many bytes of the 249-byte V2_EXTENSION payload for data.
static constexpr size_t MISSION_MAX_FRAG_PAYLOAD = 249 - sizeof(MissionFragHeader);

// IEEE 802.3 CRC-32 (the common "zip/ethernet" polynomial).
inline uint32_t mission_crc32(const uint8_t* data, size_t len)
{
    static uint32_t table[256];
    static bool table_ready = false;
    if (!table_ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        table_ready = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

// Splits `data` into V2_EXTENSION-sized fragments, each ready to memcpy
// straight into a mavlink_v2_extension_t::payload. Always returns at least
// one fragment, even for an empty blob.
inline std::vector<std::vector<uint8_t>> mission_fragment(uint32_t transfer_id,
                                                           const std::vector<uint8_t>& data)
{
    const uint32_t crc = mission_crc32(data.data(), data.size());
    uint16_t total_frags = static_cast<uint16_t>(
        (data.size() + MISSION_MAX_FRAG_PAYLOAD - 1) / MISSION_MAX_FRAG_PAYLOAD);
    if (total_frags == 0) total_frags = 1;

    std::vector<std::vector<uint8_t>> out;
    out.reserve(total_frags);

    for (uint16_t i = 0; i < total_frags; ++i) {
        const size_t offset = static_cast<size_t>(i) * MISSION_MAX_FRAG_PAYLOAD;
        const size_t chunk  = std::min(MISSION_MAX_FRAG_PAYLOAD, data.size() - offset);

        MissionFragHeader hdr{};
        hdr.transfer_id  = transfer_id;
        hdr.frag_idx     = i;
        hdr.total_frags  = total_frags;
        hdr.total_size   = static_cast<uint32_t>(data.size());
        hdr.payload_size = static_cast<uint16_t>(chunk);
        hdr.crc32        = crc;

        std::vector<uint8_t> frag(sizeof(hdr) + chunk);
        std::memcpy(frag.data(), &hdr, sizeof(hdr));
        std::memcpy(frag.data() + sizeof(hdr), data.data() + offset, chunk);
        out.push_back(std::move(frag));
    }
    return out;
}

// Reassembles fragments for ONE in-flight transfer at a time -- a new
// transfer_id resets it, discarding whatever was in progress (mirrors
// comms_{uav,gcs}.cpp's camera_assembler_: a transfer either completes or is
// abandoned, never interleaved with another of the same kind). Use one
// instance per message kind you receive.
class MissionReassembler {
public:
    // Returns the reassembled blob once every fragment has arrived and its
    // CRC checks out; std::nullopt otherwise (still incomplete, or the
    // completed blob failed its CRC check and was dropped).
    std::optional<std::vector<uint8_t>> feed(const uint8_t* payload, size_t len)
    {
        if (len < sizeof(MissionFragHeader)) return std::nullopt;

        MissionFragHeader hdr{};
        std::memcpy(&hdr, payload, sizeof(hdr));

        const uint8_t* frag_data = payload + sizeof(MissionFragHeader);
        const size_t   frag_len  = len - sizeof(MissionFragHeader);
        if (hdr.payload_size > frag_len || hdr.total_frags == 0) return std::nullopt;

        if (hdr.transfer_id != transfer_id_) {
            transfer_id_ = hdr.transfer_id;
            total_size_  = hdr.total_size;
            crc32_       = hdr.crc32;
            received_    = 0;
            frags_.assign(hdr.total_frags, {});
        }

        if (hdr.frag_idx >= frags_.size()) return std::nullopt;
        auto& frag = frags_[hdr.frag_idx];
        if (frag.received) return std::nullopt;

        frag.data.assign(frag_data, frag_data + hdr.payload_size);
        frag.received = true;
        ++received_;

        if (received_ != frags_.size()) return std::nullopt;

        std::vector<uint8_t> blob;
        blob.reserve(total_size_);
        for (auto& f : frags_) blob.insert(blob.end(), f.data.begin(), f.data.end());

        // Reset so a stray duplicate of the last fragment can't re-trigger
        // completion; the caller has the blob now.
        frags_.clear();
        received_ = 0;

        if (mission_crc32(blob.data(), blob.size()) != crc32_) return std::nullopt;
        return blob;
    }

private:
    struct Frag { std::vector<uint8_t> data; bool received{false}; };

    uint32_t transfer_id_{UINT32_MAX};
    uint32_t total_size_{0};
    uint32_t crc32_{0};
    uint16_t received_{0};
    std::vector<Frag> frags_;
};
