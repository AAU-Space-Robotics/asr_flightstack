// Standalone .ulg inspector — works on both asr_logger logs and PX4 flight-controller
// logs. Exists to validate the reader outside the GUI, and to answer "what is actually
// in this file and what clock is it on" without launching the GCS.

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "data_container.hpp"
#include "exception.hpp"
#include "reader.hpp"

namespace {

// Timestamps at or above this are unix-epoch microseconds (2017-07-14 onwards);
// anything below is time-since-boot. ULog itself does not record which is meant,
// so the magnitude is the only signal available.
constexpr uint64_t kEpochThresholdUs = 1500000000000000ULL;

std::string formatUtc(uint64_t us)
{
    const std::time_t secs = static_cast<std::time_t>(us / 1000000ULL);
    std::tm tm{};
    gmtime_r(&secs, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    char out[48];
    std::snprintf(out, sizeof(out), "%s.%03u", buf,
                  static_cast<unsigned>((us % 1000000ULL) / 1000ULL));
    return out;
}

std::shared_ptr<ulog_cpp::DataContainer> load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "error: cannot open " << path << "\n";
        return nullptr;
    }

    auto container =
        std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
    ulog_cpp::Reader reader(container);

    std::vector<uint8_t> buffer(64 * 1024);
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto got = file.gcount();
        if (got > 0) {
            reader.readChunk(buffer.data(), static_cast<int>(got));
        }
    }

    if (!container->isHeaderComplete()) {
        std::cerr << "error: header never completed — not a valid ULog file?\n";
        return nullptr;
    }
    return container;
}

// A topic's timestamp range, plus which clock its stamps appear to be on.
struct Span {
    uint64_t first{0};
    uint64_t last{0};
    size_t count{0};
    bool epoch{false};
    bool has_timestamp{false};
};

Span spanOf(const ulog_cpp::Subscription& sub)
{
    Span s;
    s.count = sub.size();
    if (s.count == 0) return s;
    if (sub.fieldMap().find("timestamp") == sub.fieldMap().end()) return s;

    s.has_timestamp = true;
    s.first = sub.at(0)["timestamp"].as<uint64_t>();
    s.last = sub.at(s.count - 1)["timestamp"].as<uint64_t>();
    s.epoch = s.first >= kEpochThresholdUs;
    return s;
}

void printSummary(const std::string& path, ulog_cpp::DataContainer& c)
{
    std::printf("file:   %s\n", path.c_str());

    const uint64_t hdr = c.fileHeader().header().timestamp;
    std::printf("header: %" PRIu64 " us  ", hdr);
    if (hdr >= kEpochThresholdUs) {
        std::printf("(%s UTC)\n", formatUtc(hdr).c_str());
    } else {
        std::printf("(%.1f s — time since boot, not wall clock)\n", static_cast<double>(hdr) / 1e6);
    }

    for (const auto& [key, info] : c.messageInfo()) {
        if (info.field().type().type == ulog_cpp::Field::BasicType::CHAR) {
            std::printf("info:   %s = %s\n", key.c_str(), info.value().as<std::string>().c_str());
        }
    }

    if (!c.logging().empty()) std::printf("logging messages: %zu\n", c.logging().size());
    if (!c.dropouts().empty()) {
        uint32_t total_ms = 0;
        for (const auto& d : c.dropouts()) total_ms += d.durationMs();
        std::printf("dropouts: %zu (%u ms total)\n", c.dropouts().size(), total_ms);
    }
    if (!c.initialParameters().empty()) {
        std::printf("parameters: %zu\n", c.initialParameters().size());
    }
    for (const auto& e : c.parsingErrors()) std::printf("parse warning: %s\n", e.c_str());

    // Topics, most samples first.
    std::vector<std::pair<std::string, Span>> rows;
    size_t epoch_topics = 0;
    size_t boot_topics = 0;
    for (const auto& [k, sub] : c.subscriptionsByNameAndMultiId()) {
        const std::string name = k.multi_id ? k.name + "/" + std::to_string(k.multi_id) : k.name;
        Span s = spanOf(*sub);
        if (s.has_timestamp && s.count > 0) (s.epoch ? epoch_topics : boot_topics)++;
        rows.emplace_back(name, s);
    }
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second.count > b.second.count; });

    std::printf("\n%-38s %9s %8s  %s\n", "topic", "samples", "rate", "time span");
    std::printf("%s\n", std::string(96, '-').c_str());
    for (const auto& [name, s] : rows) {
        if (s.count == 0) continue;
        std::printf("%-38s %9zu", name.c_str(), s.count);
        if (!s.has_timestamp) {
            std::printf(" %8s  (no timestamp field)\n", "-");
            continue;
        }
        const double dur = static_cast<double>(s.last - s.first) / 1e6;
        std::printf(" %7.1fHz  ", dur > 0 ? static_cast<double>(s.count - 1) / dur : 0.0);
        if (s.epoch) {
            std::printf("%s .. %s UTC\n", formatUtc(s.first).c_str(), formatUtc(s.last).substr(11).c_str());
        } else {
            std::printf("%.1f .. %.1f s since boot\n", static_cast<double>(s.first) / 1e6,
                        static_cast<double>(s.last) / 1e6);
        }
    }

    if (epoch_topics > 0 && boot_topics > 0) {
        std::printf(
            "\nWARNING: this file mixes clock bases — %zu topic(s) stamp unix-epoch time and\n"
            "         %zu stamp time-since-boot. Plotting them on one axis will place them\n"
            "         decades apart. Offending topics:\n",
            epoch_topics, boot_topics);
        for (const auto& [name, s] : rows) {
            if (s.count && s.has_timestamp && !s.epoch) {
                std::printf("           %s (boot-relative)\n", name.c_str());
            }
        }
    }
}

void printFields(ulog_cpp::DataContainer& c, const std::string& topic)
{
    auto sub = c.subscription(topic);
    std::printf("%s — %zu samples\n\n", topic.c_str(), sub->size());
    for (const auto& fname : sub->format()->fieldNames()) {
        const auto f = sub->field(fname);
        std::printf("  %-28s %s", fname.c_str(), f->type().name.c_str());
        if (f->arrayLength() > 0) std::printf("[%d]", f->arrayLength());
        std::printf("\n");
    }
}

void printCsv(ulog_cpp::DataContainer& c, const std::string& topic, const std::string& field_csv)
{
    auto sub = c.subscription(topic);

    std::vector<std::string> wanted;
    if (field_csv.empty()) {
        wanted = sub->format()->fieldNames();
    } else {
        size_t pos = 0;
        std::string rest = field_csv;
        while ((pos = rest.find(',')) != std::string::npos) {
            wanted.push_back(rest.substr(0, pos));
            rest.erase(0, pos + 1);
        }
        if (!rest.empty()) wanted.push_back(rest);
    }

    // Expand arrays into indexed columns; skip nested fields, which have no flat representation.
    struct Col {
        std::string header;
        std::shared_ptr<ulog_cpp::Field> field;
        int index;
    };
    std::vector<Col> cols;
    for (const auto& name : wanted) {
        const auto it = sub->fieldMap().find(name);
        if (it == sub->fieldMap().end()) {
            std::cerr << "warning: no field '" << name << "' in " << topic << "\n";
            continue;
        }
        const auto& f = it->second;
        if (f->type().type == ulog_cpp::Field::BasicType::NESTED) {
            std::cerr << "warning: skipping nested field '" << name << "'\n";
            continue;
        }
        const int len = f->arrayLength();
        if (len > 1) {
            for (int i = 0; i < len; ++i) {
                cols.push_back({name + "[" + std::to_string(i) + "]", f, i});
            }
        } else {
            cols.push_back({name, f, -1});
        }
    }
    if (cols.empty()) return;

    for (size_t i = 0; i < cols.size(); ++i) {
        std::printf("%s%s", cols[i].header.c_str(), i + 1 < cols.size() ? "," : "\n");
    }

    for (size_t n = 0; n < sub->size(); ++n) {
        const auto view = sub->at(n);
        for (size_t i = 0; i < cols.size(); ++i) {
            const ulog_cpp::Value v(*cols[i].field, view.rawData(), cols[i].index);
            // uint64 timestamps lose precision through double, so print them as integers.
            if (cols[i].field->type().type == ulog_cpp::Field::BasicType::UINT64) {
                std::printf("%" PRIu64, v.as<uint64_t>());
            } else {
                std::printf("%.9g", v.as<double>());
            }
            std::printf("%s", i + 1 < cols.size() ? "," : "\n");
        }
    }
}

void usage()
{
    std::cerr << "usage:\n"
              << "  ulog_dump <file.ulg>                        summary + topic list\n"
              << "  ulog_dump <file.ulg> --fields <topic>        field names and types\n"
              << "  ulog_dump <file.ulg> --csv <topic> [f1,f2]   sample values as CSV\n";
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    const std::string path = argv[1];
    std::shared_ptr<ulog_cpp::DataContainer> container;
    try {
        container = load(path);
    } catch (const ulog_cpp::ExceptionBase& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    if (!container) return 1;

    try {
        if (argc >= 4 && std::strcmp(argv[2], "--fields") == 0) {
            printFields(*container, argv[3]);
        } else if (argc >= 4 && std::strcmp(argv[2], "--csv") == 0) {
            printCsv(*container, argv[3], argc >= 5 ? argv[4] : "");
        } else if (argc == 2) {
            printSummary(path, *container);
        } else {
            usage();
            return 1;
        }
    } catch (const ulog_cpp::ExceptionBase& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
