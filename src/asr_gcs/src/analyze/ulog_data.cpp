#include "analyze/ulog_data.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

#include "data_container.hpp"
#include "exception.hpp"
#include "reader.hpp"

namespace analyze {

namespace {

constexpr uint64_t kEpochThresholdUs = 1500000000000000ULL;

bool IsNumeric(ulog_cpp::Field::BasicType t)
{
    return t != ulog_cpp::Field::BasicType::NESTED && t != ulog_cpp::Field::BasicType::CHAR;
}


void SplitIndexed(const std::string &in, std::string &name, int &index)
{
    const auto open = in.find('[');
    if (open == std::string::npos || in.back() != ']') {
        name = in;
        index = -1;
        return;
    }
    name = in.substr(0, open);
    index = std::atoi(in.c_str() + open + 1);
}

Source ClassifySource(const std::string &sys_name)
{
    if (sys_name == "PX4") return Source::Fc;
    if (sys_name == "asr_gcs") return Source::Gcs;
    if (!sys_name.empty()) return Source::Uav; 
    return Source::Unknown;
}

bool LooksLikeAngle(const std::string &field)
{
    static const char *kNames[] = {"roll", "pitch", "yaw", "ref_yaw", "heading"};
    for (const char *n : kNames) {
        if (field == n) return true;
    }
    return false;
}

double WrapPi(double a)
{
    constexpr double kTwoPi = 2.0 * M_PI;
    a = std::fmod(a + M_PI, kTwoPi);
    if (a < 0.0) a += kTwoPi;
    return a - M_PI;
}

}  // namespace

LogFile::LogFile() = default;
LogFile::~LogFile() = default;

bool LogFile::Load(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        warnings_.push_back("cannot open file");
        return false;
    }

    container_ = std::make_shared<ulog_cpp::DataContainer>(
        ulog_cpp::DataContainer::StorageConfig::FullLog);

    try {
        ulog_cpp::Reader reader(container_);
        std::vector<uint8_t> buffer(256 * 1024);
        while (file) {
            file.read(reinterpret_cast<char *>(buffer.data()),
                      static_cast<std::streamsize>(buffer.size()));
            if (file.gcount() > 0) {
                reader.readChunk(buffer.data(), static_cast<int>(file.gcount()));
            }
        }
    } catch (const ulog_cpp::ExceptionBase &e) {
        warnings_.push_back(std::string("parse error: ") + e.what());
        return false;
    }

    if (!container_->isHeaderComplete()) {
        warnings_.push_back("not a valid ULog file (header never completed)");
        return false;
    }

    path_ = path;
    name_ = std::filesystem::path(path).filename().string();

    const auto &info = container_->messageInfo();
    if (const auto it = info.find("sys_name"); it != info.end()) {
        try {
            sys_name_ = it->second.value().as<std::string>();
        } catch (const ulog_cpp::ExceptionBase &) {
        }
    }
    is_px4_ = (sys_name_ == "PX4");
    source_ = ClassifySource(sys_name_);

    if (!BuildTimeMapping()) return false;
    BuildTopics();
    BuildTrack();

    for (const auto &e : container_->parsingErrors()) {
        warnings_.push_back("parse warning: " + e);
    }
    return true;
}


bool LogFile::BuildTimeMapping()
{
    const auto &subs = container_->subscriptionsByNameAndMultiId();

    size_t total_samples = 0;
    for (const auto &[key, sub] : subs) total_samples += sub->size();
    if (total_samples == 0) {
        warnings_.push_back("log contains no data samples (header only)");
        return false;
    }

    uint64_t first_epoch_us = 0;
    for (const auto &[key, sub] : subs) {
        if (sub->size() == 0) continue;
        if (sub->fieldMap().find("timestamp") == sub->fieldMap().end()) continue;
        const uint64_t first = sub->at(0)["timestamp"].as<uint64_t>();
        if (first >= kEpochThresholdUs && (first_epoch_us == 0 || first < first_epoch_us)) {
            first_epoch_us = first;
        }
    }

    if (first_epoch_us > 0) {
        // Already absolute (asr_logger).
        absolute_time_ = true;
        time_slope_ = 1.0;
        time_intercept_ = 0.0;
    } else {
        // Boot-relative (PX4): recover absolute time from the GPS records.
        std::vector<double> hrt, utc;
        const auto it = subs.find({"vehicle_gps_position", 0});
        if (it != subs.end() && it->second->size() > 0 &&
            it->second->fieldMap().count("time_utc_usec")) {
            for (const auto sample : *it->second) {
                const auto t_utc = sample["time_utc_usec"].as<uint64_t>();
                if (t_utc < kEpochThresholdUs) continue;   // no time lock yet
                hrt.push_back(static_cast<double>(sample["timestamp"].as<uint64_t>()));
                utc.push_back(static_cast<double>(t_utc));
            }
        }

        if (hrt.size() >= 2) {
            const double n = static_cast<double>(hrt.size());
            double sx = 0, sy = 0, sxx = 0, sxy = 0;

            const double x0 = hrt[0], y0 = utc[0];
            for (size_t i = 0; i < hrt.size(); ++i) {
                const double x = hrt[i] - x0, y = utc[i] - y0;
                sx += x; sy += y; sxx += x * x; sxy += x * y;
            }
            const double denom = n * sxx - sx * sx;
            if (std::abs(denom) > 1e-9) {
                time_slope_ = (n * sxy - sx * sy) / denom;
                time_intercept_ = y0 + (sy - time_slope_ * sx) / n - time_slope_ * x0;
                absolute_time_ = true;
            }
        }

        if (!absolute_time_) {
            time_slope_ = 1.0;
            time_intercept_ = 0.0;
            warnings_.push_back(
                "no GPS time lock in this log -- times are relative to flight-controller "
                "boot and cannot be aligned with another log");
        }
    }

    if (absolute_time_ && first_epoch_us > 0) {
        for (const auto &[key, sub] : subs) {
            if (sub->size() == 0) continue;
            if (sub->fieldMap().find("timestamp") == sub->fieldMap().end()) continue;
            const uint64_t first = sub->at(0)["timestamp"].as<uint64_t>();
            if (first < kEpochThresholdUs) {
                const std::string name =
                    key.multi_id ? key.name + "/" + std::to_string(key.multi_id) : key.name;
                boot_topic_shift_us_[name] =
                    static_cast<double>(first_epoch_us) - static_cast<double>(first);
                warnings_.push_back(
                    "'" + name +
                    "' is stamped on a different clock than the rest of the file; shifted to "
                    "line up with the log start (approximate)");
            }
        }
    }

    std::vector<double> starts;
    bool have_end = false;
    for (const auto &[key, sub] : subs) {
        if (sub->size() == 0) continue;
        if (sub->fieldMap().find("timestamp") == sub->fieldMap().end()) continue;
        const std::string name =
            key.multi_id ? key.name + "/" + std::to_string(key.multi_id) : key.name;
        if (boot_topic_shift_us_.count(name)) continue;
        const double a = ToSeconds(sub->at(0)["timestamp"].as<uint64_t>(), name);
        const double b = ToSeconds(sub->at(sub->size() - 1)["timestamp"].as<uint64_t>(), name);
        starts.push_back(a);
        t_end_ = have_end ? std::max(t_end_, b) : b;
        have_end = true;
    }
    if (!have_end) return false;

    const size_t mid = starts.size() / 2;
    std::nth_element(starts.begin(), starts.begin() + static_cast<long>(mid), starts.end());
    t_begin_ = starts[mid];
    if (t_begin_ > t_end_) t_begin_ = t_end_;   // degenerate single-sample logs
    return true;
}

double LogFile::ToSeconds(uint64_t raw_us, const std::string &topic) const
{
    double us = static_cast<double>(raw_us) * time_slope_ + time_intercept_;
    if (const auto it = boot_topic_shift_us_.find(topic); it != boot_topic_shift_us_.end()) {
        us += it->second;
    }
    return us / 1e6 + time_offset_;
}

void LogFile::set_time_offset(double seconds)
{
    if (seconds == time_offset_) return;
    time_offset_ = seconds;
    cache_.clear();
    track_ = Track{};
    BuildTrack();
}

void LogFile::BuildTopics()
{
    for (const auto &[key, sub] : container_->subscriptionsByNameAndMultiId()) {
        if (sub->size() == 0) continue;

        TopicInfo info;
        info.name = key.multi_id ? key.name + "/" + std::to_string(key.multi_id) : key.name;
        info.sample_count = sub->size();
        info.boot_clock = boot_topic_shift_us_.count(info.name) > 0;

        for (const auto &fname : sub->format()->fieldNames()) {
            if (fname == "timestamp") continue;
            const auto f = sub->field(fname);
            if (!IsNumeric(f->type().type)) continue;
            if (f->arrayLength() > 1) {
                for (int i = 0; i < f->arrayLength(); ++i) {
                    info.fields.push_back(fname + "[" + std::to_string(i) + "]");
                }
            } else {
                info.fields.push_back(fname);
            }
        }

        if (key.name == "vehicle_attitude" && sub->fieldMap().count("q")) {
            info.fields.insert(info.fields.begin(), {"roll", "pitch", "yaw"});
        }

        topics_.push_back(std::move(info));
    }

    std::sort(topics_.begin(), topics_.end(),
              [](const TopicInfo &a, const TopicInfo &b) { return a.name < b.name; });
}

void LogFile::BuildTrack()
{
    const auto &subs = container_->subscriptionsByNameAndMultiId();
    const auto it = subs.find({"vehicle_gps_position", 0});
    if (it == subs.end() || it->second->size() == 0) return;

    const auto &sub = it->second;
   
    const bool degrees = sub->fieldMap().count("latitude_deg") > 0;
    const char *lat_field = degrees ? "latitude_deg" : "lat";
    const char *lon_field = degrees ? "longitude_deg" : "lon";
    if (!sub->fieldMap().count(lat_field) || !sub->fieldMap().count(lon_field)) return;
    const double scale = degrees ? 1.0 : 1e-7;

    for (const auto sample : *sub) {
        const double lat = sample[lat_field].as<double>() * scale;
        const double lon = sample[lon_field].as<double>() * scale;
        if (std::abs(lat) < 1e-6 && std::abs(lon) < 1e-6) continue;  
        if (std::abs(lat) > 90.0 || std::abs(lon) > 180.0) continue;
        track_.t.push_back(ToSeconds(sample["timestamp"].as<uint64_t>(), "vehicle_gps_position"));
        track_.lat.push_back(lat);
        track_.lon.push_back(lon);
    }
}

const Series *LogFile::GetSeries(const std::string &topic, const std::string &field)
{
    const std::string key = topic + "|" + field;
    if (const auto it = cache_.find(key); it != cache_.end()) return &it->second;
    if (!container_) return nullptr;

    std::string base = topic;
    int multi = 0;
    if (const auto slash = topic.find('/'); slash != std::string::npos) {
        base = topic.substr(0, slash);
        multi = std::atoi(topic.c_str() + slash + 1);
    }

    std::shared_ptr<ulog_cpp::Subscription> sub;
    try {
        sub = container_->subscription(base, multi);
    } catch (const ulog_cpp::ExceptionBase &) {
        return nullptr;
    }
    if (!sub || sub->size() == 0) return nullptr;

    Series series;
    series.t.reserve(sub->size());
    series.v.reserve(sub->size());

    const bool derived_euler =
        base == "vehicle_attitude" && sub->fieldMap().count("q") && LooksLikeAngle(field) &&
        !sub->fieldMap().count(field);

    std::string fname;
    int index = -1;
    SplitIndexed(field, fname, index);

    if (!derived_euler && !sub->fieldMap().count(fname)) return nullptr;

    try {
        for (const auto sample : *sub) {
            const double t = ToSeconds(sample["timestamp"].as<uint64_t>(), topic);
            double value;
            if (derived_euler) {
                const double w = sample["q"][static_cast<size_t>(0)].as<double>();
                const double x = sample["q"][static_cast<size_t>(1)].as<double>();
                const double y = sample["q"][static_cast<size_t>(2)].as<double>();
                const double z = sample["q"][static_cast<size_t>(3)].as<double>();
                if (field == "roll") {
                    value = std::atan2(2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y));
                } else if (field == "pitch") {
                    value = std::asin(std::clamp(2.0 * (w * y - z * x), -1.0, 1.0));
                } else {
                    value = std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z));
                }
            } else if (index >= 0) {
                value = sample[fname][static_cast<size_t>(index)].as<double>();
            } else {
                value = sample[fname].as<double>();
            }
            series.t.push_back(t);
            series.v.push_back(value);
        }
    } catch (const ulog_cpp::ExceptionBase &) {
        return nullptr;
    }

    // Re-wrap [0, 2pi) angles so they overlay on the FC's [-pi, pi] convention.
    if (LooksLikeAngle(fname) && !derived_euler && !series.v.empty()) {
        const double hi = *std::max_element(series.v.begin(), series.v.end());
        if (hi > M_PI * 1.01) {
            for (double &v : series.v) v = WrapPi(v);
            series.note = "re-wrapped from [0, 2pi) to [-pi, pi]";
        }
    }
    series.angle = derived_euler || LooksLikeAngle(fname);

    return &(cache_[key] = std::move(series));
}

// ---------------------------------------------------------------------------

std::string ExpandUser(const std::string &path)
{
    if (path.empty() || path[0] != '~') return path;
    const char *home = std::getenv("HOME");
    return home ? std::string(home) + path.substr(1) : path;
}

const char *SourceName(Source s)
{
    switch (s) {
        case Source::Uav: return "uav";
        case Source::Fc: return "fc";
        case Source::Gcs: return "gcs";
        default: return "?";
    }
}

namespace {

std::string DateOf(double t_seconds, bool absolute)
{
    if (!absolute) return "undated";
    const std::time_t secs = static_cast<std::time_t>(t_seconds);
    std::tm tm{};
    gmtime_r(&secs, &tm);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

}  // namespace

std::vector<DateGroup> ScanGrouped(const std::string &root, size_t limit)
{
    namespace fs = std::filesystem;
    std::error_code ec;

    const std::string expanded = ExpandUser(root);
    if (!fs::is_directory(expanded, ec)) return {};

    std::vector<std::string> paths;
    auto it = fs::recursive_directory_iterator(
        expanded, fs::directory_options::skip_permission_denied, ec);
    if (ec) return {};
    for (const auto &e : it) {
        if (paths.size() >= limit) break;
        if (!e.is_regular_file(ec)) continue;
        if (e.path().extension() != ".ulg") continue;
        paths.push_back(e.path().string());
    }


    std::map<std::string, std::vector<LogEntry>> by_date;
    for (const auto &p : paths) {
        LogFile probe;
        if (!probe.Load(p)) continue;  
        LogEntry entry;
        entry.path = p;
        entry.name = probe.name();
        entry.source = probe.source();
        entry.t_begin = probe.t_begin();
        entry.t_end = probe.t_end();
        entry.absolute = probe.absolute_time();
        entry.date = DateOf(entry.t_begin, entry.absolute);
        by_date[entry.date].push_back(std::move(entry));
    }

    std::vector<DateGroup> groups;
    groups.reserve(by_date.size());
    for (auto &[date, entries] : by_date) {
        std::sort(entries.begin(), entries.end(),
                  [](const LogEntry &a, const LogEntry &b) { return a.t_begin < b.t_begin; });
        groups.push_back({date, std::move(entries)});
    }

    std::sort(groups.begin(), groups.end(), [](const DateGroup &a, const DateGroup &b) {
        if (a.date == "undated") return false;
        if (b.date == "undated") return true;
        return a.date > b.date;
    });
    return groups;
}

namespace {

struct SignalPair {
    Source source;
    const char *topic;
    const char *field;
};
const SignalPair kUavSignals[] = {
    {Source::Uav, "position", "pos_z"},
    {Source::Uav, "position", "pos_x"},
    {Source::Uav, "attitude", "pitch"},
};
const SignalPair kFcSignals[] = {
    {Source::Fc, "vehicle_local_position", "z"},
    {Source::Fc, "vehicle_local_position", "x"},
    {Source::Fc, "vehicle_attitude", "pitch"},
};

const Series *PickSignal(LogFile &log, size_t rank)
{
    const SignalPair *table = (log.source() == Source::Fc) ? kFcSignals : kUavSignals;
    if (rank >= 3) return nullptr;
    return log.GetSeries(table[rank].topic, table[rank].field);
}

double SampleLinear(const Series &s, double t)
{
    const auto it = std::lower_bound(s.t.begin(), s.t.end(), t);
    const size_t i = static_cast<size_t>(it - s.t.begin());
    if (i == 0) return s.v.front();
    if (i >= s.t.size()) return s.v.back();
    const double t0 = s.t[i - 1], t1 = s.t[i];
    const double f = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0;
    return s.v[i - 1] + f * (s.v[i] - s.v[i - 1]);
}

double CorrelationAtLag(const Series &a, const Series &b, double lag, double step)
{
    const double lo = std::max(a.t.front(), b.t.front() + lag);
    const double hi = std::min(a.t.back(), b.t.back() + lag);
    if (hi - lo < 10.0) return -2.0;   // need a decent stretch of common time

    double sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0;
    int n = 0;
    for (double t = lo; t <= hi; t += step) {
        const double va = SampleLinear(a, t);
        const double vb = SampleLinear(b, t - lag);
        sa += va; sb += vb; saa += va * va; sbb += vb * vb; sab += va * vb;
        ++n;
    }
    if (n < 20) return -2.0;
    const double cov = sab / n - (sa / n) * (sb / n);
    const double va = saa / n - (sa / n) * (sa / n);
    const double vb = sbb / n - (sb / n) * (sb / n);
    if (va < 1e-9 || vb < 1e-9) return -2.0;   // one side is flat; nothing to match
    return cov / std::sqrt(va * vb);
}

}  // namespace

TimeAlignment EstimateTimeOffset(LogFile &reference, LogFile &moving, double search_seconds)
{
    TimeAlignment result;
    if (!reference.absolute_time() || !moving.absolute_time()) return result;
    if (reference.source() == moving.source()) return result;

   
    const double previous = moving.time_offset();
    moving.set_time_offset(0.0);

    for (size_t rank = 0; rank < 3 && !result.ok; ++rank) {
        const Series *a = PickSignal(reference, rank);
        const Series *b = PickSignal(moving, rank);
        if (!a || !b || a->t.size() < 20 || b->t.size() < 20) continue;

        double best_lag = 0.0, best_corr = -2.0;
        for (double lag = -search_seconds; lag <= search_seconds; lag += 1.0) {
            const double c = CorrelationAtLag(*a, *b, lag, 0.2);
            if (c > best_corr) { best_corr = c; best_lag = lag; }
        }
        if (best_corr < -1.0) continue;

        for (double lag = best_lag - 1.5; lag <= best_lag + 1.5; lag += 0.02) {
            const double c = CorrelationAtLag(*a, *b, lag, 0.05);
            if (c > best_corr) { best_corr = c; best_lag = lag; }
        }

        if (best_corr > 0.8) {
            result.ok = true;
            result.offset = best_lag;
            result.correlation = best_corr;
            const SignalPair *table =
                (reference.source() == Source::Fc) ? kFcSignals : kUavSignals;
            result.signal = std::string(table[rank].topic) + "." + table[rank].field;
        }
    }

    moving.set_time_offset(result.ok ? result.offset : previous);
    return result;
}

std::vector<const LogEntry *> FindCompanions(const std::vector<DateGroup> &groups,
                                              const LogEntry &entry, size_t max_results)
{
    std::vector<std::pair<double, const LogEntry *>> hits;
    for (const auto &g : groups) {
        for (const auto &e : g.entries) {
            if (e.path == entry.path || e.source == entry.source) continue;
            if (!e.absolute || !entry.absolute) continue;   // can't compare clocks
            const double overlap =
                std::min(e.t_end, entry.t_end) - std::max(e.t_begin, entry.t_begin);
            if (overlap > 0.0) hits.emplace_back(overlap, &e);
        }
    }

    std::sort(hits.begin(), hits.end(),
              [](const auto &a, const auto &b) { return a.first > b.first; });
    if (hits.size() > max_results) hits.resize(max_results);

    std::vector<const LogEntry *> out;
    out.reserve(hits.size());
    for (const auto &h : hits) out.push_back(h.second);
    return out;
}

std::string FormatTime(double seconds, bool absolute)
{
    char buf[64];
    if (!absolute) {
        std::snprintf(buf, sizeof(buf), "%.2f s", seconds);
        return buf;
    }
    const std::time_t secs = static_cast<std::time_t>(seconds);
    std::tm tm{};
    gmtime_r(&secs, &tm);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tm);
    std::snprintf(buf, sizeof(buf), "%s.%02d", stamp,
                  static_cast<int>((seconds - std::floor(seconds)) * 100.0));
    return buf;
}

}  // namespace analyze
