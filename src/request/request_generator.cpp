#include "request_generator.h"
#include "../../external/toml11/include/toml.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <cassert>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

static void export_print_progress(double *percentage) {
    while ((*percentage) < 1.0) {
        double val = (*percentage) * 100;
        int lpad = (int)((*percentage) * PBWIDTH);
        int rpad = PBWIDTH - lpad;
        printf("\r%.2f%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
        fflush(stdout);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
}

namespace workload {
using namespace std;
using namespace rfunc;

// ────────────────────────────────────────────────────────────────────────
// Constructor from TOML file
// ────────────────────────────────────────────────────────────────────────
RequestGenerator::RequestGenerator(const std::string &config_path,
                                   bool initialize_immediately) :
    config_(), initialized_(false), phase_(Phase::LOADING), loading_index_(0),
    operations_index_(0), n_requests_(0), insert_key_sequence_(nullptr) {
    load_config(config_path);
    if (initialize_immediately) {
        initialize();
    }
}

// ────────────────────────────────────────────────────────────────────────
// Constructor from explicit parameters
// ────────────────────────────────────────────────────────────────────────
RequestGenerator::RequestGenerator(
    const std::string &export_path, bool gen_values, long value_min_size,
    long value_max_size, long key_seed, long operation_seed, int n_records,
    int n_operations, const std::string &data_distribution,
    double read_proportion, double update_proportion, double insert_proportion,
    double scan_proportion, long scan_seed,
    const std::string &scan_length_distribution, int min_scan_length,
    int max_scan_length) :
    config_(), initialized_(false), phase_(Phase::LOADING), loading_index_(0),
    operations_index_(0), n_requests_(0), insert_key_sequence_(nullptr) {
    config_.export_path = export_path;
    config_.gen_values = gen_values;
    config_.value_min_size = value_min_size;
    config_.value_max_size = value_max_size;
    config_.key_seed = key_seed;
    config_.operation_seed = operation_seed;
    config_.n_records = n_records;
    config_.n_operations = n_operations;
    config_.data_distribution = data_distribution;
    config_.read_proportion = read_proportion;
    config_.update_proportion = update_proportion;
    config_.insert_proportion = insert_proportion;
    config_.scan_proportion = scan_proportion;
    config_.scan_seed = scan_seed;
    config_.scan_length_distribution = scan_length_distribution;
    config_.min_scan_length = min_scan_length;
    config_.max_scan_length = max_scan_length;
    initialize();
}

// ────────────────────────────────────────────────────────────────────────
// Public helpers
// ────────────────────────────────────────────────────────────────────────
void RequestGenerator::load_config(const std::string &config_path) {
    if (insert_key_sequence_) {
        delete insert_key_sequence_;
        insert_key_sequence_ = nullptr;
    }

    initialized_ = false;
    phase_ = Phase::LOADING;
    loading_index_ = 0;
    operations_index_ = 0;
    n_requests_ = 0;

    const auto config = toml::parse(config_path);

    config_.export_path =
        toml::find<string>(config, "output", "requests", "export_path");
    config_.gen_values = toml::find<bool>(config, "workload", "gen_values");
    config_.value_min_size =
        toml::find<long>(config, "workload", "value_min_size");
    config_.value_max_size =
        toml::find<long>(config, "workload", "value_max_size");
    config_.key_seed = toml::find<long>(config, "workload", "key_seed");
    config_.operation_seed =
        toml::find<long>(config, "workload", "operation_seed");
    config_.n_records = toml::find<int>(config, "workload", "n_records");
    config_.n_operations = toml::find<int>(config, "workload", "n_operations");
    config_.data_distribution =
        toml::find<string>(config, "workload", "data_distribution");
    config_.read_proportion =
        toml::find<double>(config, "workload", "read_proportion");
    config_.update_proportion =
        toml::find<double>(config, "workload", "update_proportion");
    config_.insert_proportion =
        toml::find<double>(config, "workload", "insert_proportion");
    config_.scan_proportion =
        toml::find<double>(config, "workload", "scan_proportion");

    config_.scan_seed = 0;
    config_.scan_length_distribution = "UNIFORM";
    config_.min_scan_length = 1;
    config_.max_scan_length = 1000;

    if (config_.scan_proportion > 0) {
        config_.scan_seed = toml::find<long>(config, "workload", "scan_seed");
        config_.scan_length_distribution =
            toml::find<string>(config, "workload", "scan_length_distribution");
        config_.min_scan_length =
            toml::find<int>(config, "workload", "min_scan_length");
        config_.max_scan_length =
            toml::find<int>(config, "workload", "max_scan_length");
    }
}

void RequestGenerator::initialize() {
    if (initialized_) {
        return;
    }
    init();
    initialized_ = true;
}

RequestGenerator::Configuration &RequestGenerator::config() { return config_; }

const RequestGenerator::Configuration &RequestGenerator::config() const {
    return config_;
}

bool RequestGenerator::is_initialized() const { return initialized_; }

// ────────────────────────────────────────────────────────────────────────
// Destructor
// ────────────────────────────────────────────────────────────────────────
RequestGenerator::~RequestGenerator() { delete insert_key_sequence_; }

// ────────────────────────────────────────────────────────────────────────
// Shared initialisation (called from both constructors)
// ────────────────────────────────────────────────────────────────────────
void RequestGenerator::init() {
    operation_proportions_.clear();

    insert_key_sequence_ = new acknowledged_counter<long>(config_.n_records);

    if (config_.read_proportion > 0) {
        operation_proportions_.push_back(
            make_pair(loadgen::types::Type::READ, config_.read_proportion));
    }
    if (config_.update_proportion > 0) {
        operation_proportions_.push_back(
            make_pair(loadgen::types::Type::UPDATE, config_.update_proportion));
    }
    if (config_.insert_proportion > 0) {
        operation_proportions_.push_back(
            make_pair(loadgen::types::Type::WRITE, config_.insert_proportion));
    }

    Distribution data_distribution = str_to_dist(config_.data_distribution);

    if (data_distribution == UNIFORM) {
        data_generator_ =
            uniform_distribution_rand(0, config_.n_records, config_.key_seed);
    } else if (data_distribution == ZIPFIAN) {
        int expectednewkeys =
            (int)((config_.n_operations) * config_.insert_proportion * 2.0);
        data_generator_ = scrambled_zipfian_distribution(
            0, config_.n_records + expectednewkeys, config_.key_seed);
    } else if (data_distribution == LATEST) {
        zipfian_int_distribution<long> *zip =
            new zipfian_int_distribution<long>(
                0, insert_key_sequence_->last_value());
        data_generator_ = skewed_latest_distribution(insert_key_sequence_, zip,
                                                     config_.key_seed);
        // leaking
    }

    if (config_.scan_proportion > 0) {
        operation_proportions_.push_back(
            make_pair(loadgen::types::Type::SCAN, config_.scan_proportion));

        Distribution scan_len_dist =
            str_to_dist(config_.scan_length_distribution);
        if (scan_len_dist == UNIFORM) {
            scan_length_generator_ = uniform_distribution_rand(
                config_.min_scan_length, config_.max_scan_length,
                config_.scan_seed);
        } else if (scan_len_dist == ZIPFIAN) {
            scan_length_generator_ = scrambled_zipfian_distribution(
                0, config_.n_records, config_.scan_seed);
        }
    }

    operation_generator_ =
        uniform_double_distribution_rand(0.0, 1.0, config_.operation_seed);

    if (config_.gen_values) {
        char_generator_ = CharGenerator();
        len_generator_ = uniform_distribution_rand(config_.value_min_size,
                                                   config_.value_max_size);
    }

    phase_ = Phase::LOADING;
    loading_index_ = 0;
    operations_index_ = 0;
    n_requests_ = config_.n_operations;
}

loadgen::types::Type RequestGenerator::next_operation(
    std::vector<std::pair<loadgen::types::Type, double>> values,
    rfunc::DoubleRandFunction *generator) {
    double sum = 0;

    for (size_t i = 0; i < values.size(); i++) {
        sum += values[i].second;
    }

    double val = (*generator)();

    for (size_t i = 0; i < values.size(); i++) {
        double vw = values[i].second / sum;
        if (val < vw) {
            return values[i].first;
        }

        val -= vw;
    }

    throw invalid_argument("Something went wrong");
}

RequestGenerator::Phase RequestGenerator::current_phase() const {
    return phase_;
}

void RequestGenerator::skip_current_phase() {
    if (phase_ == Phase::LOADING) {
        phase_ = Phase::OPERATIONS;
    } else if (phase_ == Phase::OPERATIONS) {
        phase_ = Phase::DONE;
    }
}

// ────────────────────────────────────────────────────────────────────────
// next()  –  returns true when the workload has ended
// ────────────────────────────────────────────────────────────────────────
bool RequestGenerator::next(loadgen::types::Type &type, long &key,
                            std::string &value, long &scan_size) {
    value.clear();
    scan_size = 0;

    if (phase_ == Phase::DONE) {
        return true;
    }

    // ── Loading phase: emit initial records (WRITE keys 0 … n_records-1) ──
    if (phase_ == Phase::LOADING) {
        if (loading_index_ < config_.n_records) {
            type = loadgen::types::Type::WRITE;
            key = loading_index_;

            if (config_.gen_values) {
                char buf[MAX_VALUE_LEN + 1];
                long length = len_generator_();
                for (long i = 0; i < length; i++) {
                    buf[i] = char_generator_();
                }
                buf[length] = '\0';
                value.assign(buf, static_cast<size_t>(length));
            }

            loading_index_++;
            return false;
        }
        // Loading finished → move to operations
        phase_ = Phase::OPERATIONS;
    }

    // ── Operations phase ──────────────────────────────────────────────
    if (phase_ == Phase::OPERATIONS) {
        if (operations_index_ < config_.n_operations) {
            type =
                next_operation(operation_proportions_, &operation_generator_);

            if (type == loadgen::types::Type::READ ||
                type == loadgen::types::Type::UPDATE) {
                do {
                    key = data_generator_();
                } while (key >= insert_key_sequence_->last_value());

                if (type == loadgen::types::Type::UPDATE) {
                    type = loadgen::types::Type::WRITE;
                }
            } else if (type == loadgen::types::Type::SCAN) {
                long size = scan_length_generator_();
                scan_size = size;
                n_requests_ += (size - 1);
                do {
                    key = data_generator_();
                } while (key + size >= insert_key_sequence_->last_value());
            } else if (type == loadgen::types::Type::WRITE) {
                key = insert_key_sequence_->next();
            }

            // Generate a value for WRITE operations when gen_values is on
            if (type == loadgen::types::Type::WRITE && config_.gen_values) {
                char buf[MAX_VALUE_LEN + 1];
                long length = len_generator_();
                for (long i = 0; i < length; i++) {
                    buf[i] = char_generator_();
                }
                buf[length] = '\0';
                value.assign(buf, static_cast<size_t>(length));
            }

            operations_index_++;
            return false;
        }

        phase_ = Phase::DONE;
    }

    return true; // workload ended
}

// ────────────────────────────────────────────────────────────────────────
// acknowledge()  –  update the atomic acknowledged counter
// ────────────────────────────────────────────────────────────────────────
void RequestGenerator::acknowledge(long key) {
    insert_key_sequence_->acknowledge(key);
}

// ────────────────────────────────────────────────────────────────────────
// generate_to_file()  –  dump full workload to the export file
// ────────────────────────────────────────────────────────────────────────
void RequestGenerator::generate_to_file() {
    cout << "Generating " << config_.export_path << " ..." << endl;

    float total = static_cast<float>(config_.n_records + config_.n_operations);
    double progress = 0;
    auto progress_thread = std::thread(export_print_progress, &progress);

    ofstream ofs(config_.export_path, ofstream::out);

    loadgen::types::Type type;
    long key;
    std::string value;
    long scan_size;
    long count = 0;

    while (!next(type, key, value, scan_size)) {
        if (type == loadgen::types::Type::READ) {
            ofs << static_cast<int>(type) << "," << setfill('0') << setw(10)
                << key << endl;
        } else if (type == loadgen::types::Type::WRITE) {
            ofs << static_cast<int>(type) << "," << setfill('0') << setw(10)
                << key;
            if (!value.empty()) {
                ofs << "," << value;
            }
            ofs << endl;
            acknowledge(key);
        } else if (type == loadgen::types::Type::SCAN) {
            ofs << static_cast<int>(type) << "," << setfill('0') << setw(10)
                << key << "," << scan_size << endl;
        }

        count++;
        progress = count / total;
    }

    progress = 1.0;
    progress_thread.join();

    cout << "number of writes/reads to keys: " << n_requests_ << endl;
    cout << "Generated into " << config_.export_path << endl;
    ofs.flush();
    ofs.close();
}

} // namespace workload
