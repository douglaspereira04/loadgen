#ifndef WORKLOAD_EXPORT_REQUEST_GENERATOR_H
#define WORKLOAD_EXPORT_REQUEST_GENERATOR_H

#include <atomic>
#include <string>
#include <vector>
#include <utility>

#include "char_generator.h"
#include "acknowledged_counter.h"
#include "../types/types.h"

namespace workload {

class RequestGenerator {
public:
    struct Configuration {
        std::string export_path;
        bool gen_values = false;
        long value_min_size = 0;
        long value_max_size = 0;
        long key_seed = 0;
        long operation_seed = 0;
        int n_records = 0;
        int n_operations = 0;
        std::string data_distribution = "UNIFORM";
        double read_proportion = 0.0;
        double update_proportion = 0.0;
        double insert_proportion = 0.0;
        double scan_proportion = 0.0;
        long scan_seed = 0;
        std::string scan_length_distribution = "UNIFORM";
        int min_scan_length = 1;
        int max_scan_length = 1000;
    };

    /// Constructor from a TOML config file path.
    RequestGenerator(const std::string &config_path,
                     bool initialize_immediately = true);

    /// Constructor from explicit parameters (all entries of the TOML file).
    RequestGenerator(const std::string &export_path, bool gen_values,
                     long value_min_size, long value_max_size, long key_seed,
                     long operation_seed, int n_records, int n_operations,
                     const std::string &data_distribution,
                     double read_proportion, double update_proportion,
                     double insert_proportion, double scan_proportion,
                     long scan_seed,
                     const std::string &scan_length_distribution,
                     int min_scan_length, int max_scan_length);

    ~RequestGenerator();

    /// Generate all operations and dump them into the export file.
    /// Equivalent to generate_export_requests. Inserts are auto-acknowledged.
    void generate_to_file();

    /// Get the next operation.
    /// @param[in] values       The operation types and their probabilities.
    /// @param[in] generator    The generator for the operation.
    /// @return The next operation.
    loadgen::types::Type
    next_operation(std::vector<std::pair<loadgen::types::Type, double>> values,
                   rfunc::DoubleRandFunction *generator);

    /// Get the next operation.
    /// @param[out] type       The operation type (READ, WRITE, SCAN).
    /// @param[out] key        The key for the operation.
    /// @param[out] value      The value string (non-empty only for WRITEs when
    /// gen_values is on).
    /// @param[out] scan_size  The scan length (non-zero only for SCAN
    /// operations).
    /// @return true if the workload has ended (no operation produced),
    ///         false if a valid operation was returned.
    bool next(loadgen::types::Type &type, long &key, std::string &value,
              long &scan_size);

    /// Increment the acknowledged counter for the given key.
    /// Must be called by the user after a WRITE/INSERT is confirmed.
    void acknowledge(long key);

    /// Reload configuration from TOML without instantiating generators.
    void load_config(const std::string &config_path);
    /// Finalize initialization after the configuration is ready.
    void initialize();
    /// Mutable view of the pending configuration.
    Configuration &config();
    /// Const view of the pending configuration.
    const Configuration &config() const;
    /// True once the generator has been initialized.
    bool is_initialized() const;

    /// Skip the current phase and move to the next one.
    void skip_current_phase();

private:
    void init();

    Configuration config_;
    bool initialized_ = false;

    // ── Phase tracking ─────────────────────────────────────────────────
    enum class Phase {
        LOADING,
        OPERATIONS,
        DONE
    };
    Phase phase_;
    int loading_index_;
    int operations_index_;
    long long n_requests_;

    acknowledged_counter<long> *insert_key_sequence_;

    // ── Generators ─────────────────────────────────────────────────────
    std::vector<std::pair<loadgen::types::Type, double>> operation_proportions_;
    rfunc::RandFunction data_generator_;
    rfunc::RandFunction scan_length_generator_;
    rfunc::DoubleRandFunction operation_generator_;
    CharGenerator char_generator_;
    rfunc::RandFunction len_generator_;

    static const size_t MAX_VALUE_LEN = 10240;
};

} // namespace workload

#endif
