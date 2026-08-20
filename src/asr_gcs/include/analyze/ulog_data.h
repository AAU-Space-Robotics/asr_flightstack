#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ulog_cpp { class DataContainer; }

namespace analyze {


struct Series {
    std::vector<double> t;
    std::vector<double> v;
    std::string note;  
    bool angle = false;
};

struct TopicInfo {
    std::string name;                       
    size_t sample_count = 0;
    std::vector<std::string> fields;        
    bool boot_clock = false;                
};

enum class Source { Uav, Fc, Gcs, Unknown };
const char *SourceName(Source s);

// Flight path for the map view.
struct Track {
    std::vector<double> t, lat, lon;
    bool valid() const { return lat.size() > 1; }
};

class LogFile {
public:
    LogFile();
    ~LogFile();

    bool Load(const std::string &path);

    const std::string &path() const { return path_; }
    const std::string &name() const { return name_; }
    const std::string &sys_name() const { return sys_name_; }
    Source source() const { return source_; }
    bool is_px4() const { return is_px4_; }

   
    bool absolute_time() const { return absolute_time_; }
    double t_begin() const { return t_begin_ + time_offset_; }
    double t_end() const { return t_end_ + time_offset_; }

    double time_offset() const { return time_offset_; }
    void set_time_offset(double seconds);

    const std::vector<TopicInfo> &topics() const { return topics_; }
    const Track &track() const { return track_; }
    const std::vector<std::string> &warnings() const { return warnings_; }

    // Returns nullptr if the field doesn't exist or isn't numeric.
    const Series *GetSeries(const std::string &topic, const std::string &field);

private:
    bool BuildTimeMapping();
    void BuildTopics();
    void BuildTrack();
    double ToSeconds(uint64_t raw_us, const std::string &topic) const;

    std::shared_ptr<ulog_cpp::DataContainer> container_;
    std::string path_, name_, sys_name_;
    Source source_ = Source::Unknown;
    bool is_px4_ = false;
    bool absolute_time_ = false;
    double t_begin_ = 0.0, t_end_ = 0.0;

   
    double time_slope_ = 1.0, time_intercept_ = 0.0;
    double time_offset_ = 0.0;
    std::map<std::string, double> boot_topic_shift_us_;   

    std::vector<TopicInfo> topics_;
    Track track_;
    std::vector<std::string> warnings_;
    std::map<std::string, Series> cache_;
};

struct LogEntry {
    std::string path;
    std::string name;
    Source source = Source::Unknown;
    double t_begin = 0.0, t_end = 0.0;
    bool absolute = false;
    std::string date;   
};

struct DateGroup {
    std::string date;
    std::vector<LogEntry> entries;  
};

std::vector<DateGroup> ScanGrouped(const std::string &root, size_t limit = 400);

// Result of lining one log up against another by the shape of a shared signal.
struct TimeAlignment {
    bool ok = false;
    double offset = 0.0;       
    double correlation = 0.0;  
    std::string signal;       
};

TimeAlignment EstimateTimeOffset(LogFile &reference, LogFile &moving,
                                  double search_seconds = 600.0);

std::vector<const LogEntry *> FindCompanions(const std::vector<DateGroup> &groups,
                                              const LogEntry &entry, size_t max_results = 4);

std::string ExpandUser(const std::string &path);

std::string FormatTime(double seconds, bool absolute);

}  // namespace analyze
