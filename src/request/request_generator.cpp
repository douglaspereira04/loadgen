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
RequestGenerator::RequestGenerator(const std::string& config_path)
    : insert_key_sequence_(nullptr),
      phase_(Phase::LOADING),
      loading_index_(0),
      operations_index_(0),
      n_requests_(0)
{
    const auto config = toml::parse(config_path);

    export_path_ = toml::find<string>(config, "output", "requests", "export_path");
    gen_values_  = toml::find<bool>(config, "workload", "gen_values");
    value_min_size_ = toml::find<long>(config, "workload", "value_min_size");
    value_max_size_ = toml::find<long>(config, "workload", "value_max_size");
    key_seed_       = toml::find<long>(config, "workload", "key_seed");
    operation_seed_ = toml::find<long>(config, "workload", "operation_seed");
    n_records_      = toml::find<int>(config, "workload", "n_records");
    n_operations_   = toml::find<int>(config, "workload", "n_operations");
    data_distribution_str_ = toml::find<string>(config, "workload", "data_distribution");
    read_proportion_   = toml::find<double>(config, "workload", "read_proportion");
    update_proportion_ = toml::find<double>(config, "workload", "update_proportion");
    insert_proportion_ = toml::find<double>(config, "workload", "insert_proportion");
    scan_proportion_   = toml::find<double>(config, "workload", "scan_proportion");

    scan_seed_ = 0;
    scan_length_distribution_str_ = "UNIFORM";
    min_scan_length_ = 1;
    max_scan_length_ = 1000;

    if (scan_proportion_ > 0) {
        scan_seed_ = toml::find<long>(config, "workload", "scan_seed");
        scan_length_distribution_str_ = toml::find<string>(
            config, "workload", "scan_length_distribution"
        );
        min_scan_length_ = toml::find<int>(config, "workload", "min_scan_length");
        max_scan_length_ = toml::find<int>(config, "workload", "max_scan_length");
    }

    n_requests_ = n_operations_;
    init();
}

// ────────────────────────────────────────────────────────────────────────
// Constructor from explicit parameters
// ────────────────────────────────────────────────────────────────────────
RequestGenerator::RequestGenerator(
    const std::string& export_path,
    bool gen_values,
    long value_min_size,
    long value_max_size,
    long key_seed,
    long operation_seed,
    int n_records,
    int n_operations,
    const std::string& data_distribution,
    double read_proportion,
    double update_proportion,
    double insert_proportion,
    double scan_proportion,
    long scan_seed,
    const std::string& scan_length_distribution,
    int min_scan_length,
    int max_scan_length
)
    : export_path_(export_path),
      gen_values_(gen_values),
      value_min_size_(value_min_size),
      value_max_size_(value_max_size),
      key_seed_(key_seed),
      operation_seed_(operation_seed),
      n_records_(n_records),
      n_operations_(n_operations),
      data_distribution_str_(data_distribution),
      read_proportion_(read_proportion),
      update_proportion_(update_proportion),
      insert_proportion_(insert_proportion),
      scan_proportion_(scan_proportion),
      scan_seed_(scan_seed),
      scan_length_distribution_str_(scan_length_distribution),
      min_scan_length_(min_scan_length),
      max_scan_length_(max_scan_length),
      insert_key_sequence_(nullptr),
      phase_(Phase::LOADING),
      loading_index_(0),
      operations_index_(0),
      n_requests_(n_operations)
{
    init();
}

// ────────────────────────────────────────────────────────────────────────
// Destructor
// ────────────────────────────────────────────────────────────────────────
RequestGenerator::~RequestGenerator() {
    delete insert_key_sequence_;
}

// ────────────────────────────────────────────────────────────────────────
// Shared initialisation (called from both constructors)
// ────────────────────────────────────────────────────────────────────────
void RequestGenerator::init() {

    // Internal acknowledged_counter for LATEST distribution compatibility
    insert_key_sequence_ = new acknowledged_counter<long>(n_records_);

    // Operation proportions
    if (read_proportion_ > 0) {
        operation_proportions_.push_back(make_pair(Type::READ, read_proportion_));
    }
    if (update_proportion_ > 0) {
        operation_proportions_.push_back(make_pair(Type::UPDATE, update_proportion_));
    }
    if (insert_proportion_ > 0) {
        operation_proportions_.push_back(make_pair(Type::WRITE, insert_proportion_));
    }

    // Data distribution
    Distribution data_distribution = str_to_dist(data_distribution_str_);

    if (data_distribution == UNIFORM) {
        data_generator_ = uniform_distribution_rand(0, n_records_, key_seed_);
    } else if (data_distribution == ZIPFIAN) {
        int expectednewkeys = (int)((n_operations_) * insert_proportion_ * 2.0);
        data_generator_ = scrambled_zipfian_distribution(
            0, n_records_ + expectednewkeys, key_seed_
        );
    } else if (data_distribution == LATEST) {
        zipfian_int_distribution<long>* zip =
            new zipfian_int_distribution<long>(0, insert_key_sequence_->last_value());
        data_generator_ = skewed_latest_distribution(
            insert_key_sequence_, zip, key_seed_
        );
        //leaking
    }

    // Scan generators
    if (scan_proportion_ > 0) {
        operation_proportions_.push_back(make_pair(Type::SCAN, scan_proportion_));

        Distribution scan_len_dist = str_to_dist(scan_length_distribution_str_);
        if (scan_len_dist == UNIFORM) {
            scan_length_generator_ = uniform_distribution_rand(
                min_scan_length_, max_scan_length_, scan_seed_
            );
        } else if (scan_len_dist == ZIPFIAN) {
            scan_length_generator_ = scrambled_zipfian_distribution(
                0, n_records_, scan_seed_
            );
        }
    }

    // Operation type generator
    operation_generator_ = uniform_double_distribution_rand(0.0, 1.0, operation_seed_);

    // Value generators
    if (gen_values_) {
        char_generator_ = CharGenerator();
        len_generator_  = uniform_distribution_rand(value_min_size_, value_max_size_);
    }

    // State
    phase_            = Phase::LOADING;
    loading_index_    = 0;
    operations_index_ = 0;
}


Type RequestGenerator::next_operation(
    std::vector<std::pair<Type,double>> values, 
    rfunc::DoubleRandFunction *generator
) {
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

// ────────────────────────────────────────────────────────────────────────
// next()  –  returns true when the workload has ended
// ────────────────────────────────────────────────────────────────────────
bool RequestGenerator::next(
    Type& type, long& key, std::string& value, long& scan_size
) {
    value.clear();
    scan_size = 0;

    if (phase_ == Phase::DONE) {
        return true;
    }

    // ── Loading phase: emit initial records (WRITE keys 0 … n_records-1) ──
    if (phase_ == Phase::LOADING) {
        if (loading_index_ < n_records_) {
            type = Type::WRITE;
            key  = loading_index_;

            if (gen_values_) {
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
        if (operations_index_ < n_operations_) {
            type = next_operation(operation_proportions_, &operation_generator_);

            if (type == Type::READ || type == Type::UPDATE) {
                do {
                    key = data_generator_();
                } while (key >= insert_key_sequence_->last_value());

                if (type == Type::UPDATE) {
                    type = Type::WRITE;
                }
            } else if (type == Type::SCAN) {
                long size = scan_length_generator_();
                scan_size = size;
                n_requests_ += (size - 1);
                do {
                    key = data_generator_();
                } while (key + size >= insert_key_sequence_->last_value());
            } else if (type == Type::WRITE) {
                key = insert_key_sequence_->next();
            }

            // Generate a value for WRITE operations when gen_values is on
            if (type == Type::WRITE && gen_values_) {
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
    cout << "Generating " << export_path_ << " ..." << endl;

    float total = static_cast<float>(n_records_ + n_operations_);
    double progress = 0;
    auto progress_thread = std::thread(export_print_progress, &progress);

    ofstream ofs(export_path_, ofstream::out);

    Type type;
    long key;
    std::string value;
    long scan_size;
    long count = 0;

    while (!next(type, key, value, scan_size)) {
        if (type == Type::READ) {
            ofs << static_cast<int>(type) << ","
                << setfill('0') << setw(10) << key << endl;
        } else if (type == Type::WRITE) {
            ofs << static_cast<int>(type) << ","
                << setfill('0') << setw(10) << key;
            if (!value.empty()) {
                ofs << "," << value;
            }
            ofs << endl;
            acknowledge(key);
        } else if (type == Type::SCAN) {
            ofs << static_cast<int>(type) << ","
                << setfill('0') << setw(10) << key
                << "," << scan_size << endl;
        }

        count++;
        progress = count / total;
    }

    progress = 1.0;
    progress_thread.join();

    cout << "number of writes/reads to keys: " << n_requests_ << endl;
    cout << "Generated into " << export_path_ << endl;
    ofs.flush();
    ofs.close();
}

} // namespace workload
