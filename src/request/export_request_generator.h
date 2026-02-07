#ifndef WORKLOAD_EXPORT_REQUEST_GENERATOR_H
#define WORKLOAD_EXPORT_REQUEST_GENERATOR_H

#include <atomic>
#include <string>
#include <vector>
#include <utility>

#include "request_generation.h"
#include "acknowledged_counter.h"

namespace workload {

class RequestGenerator {
public:
    /// Constructor from a TOML config file path.
    RequestGenerator(const std::string& config_path);

    /// Constructor from explicit parameters (all entries of the TOML file).
    RequestGenerator(
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
    );

    ~RequestGenerator();

    /// Generate all operations and dump them into the export file.
    /// Equivalent to generate_export_requests. Inserts are auto-acknowledged.
    void generate_to_file();


    /// Get the next operation.
    /// @param[in] values       The operation types and their probabilities.
    /// @param[in] generator    The generator for the operation.
    /// @return The next operation.
    Type next_operation(
        std::vector<std::pair<Type,double>> values, 
        DoubleRandFunction *generator
    );


    /// Get the next operation.
    /// @param[out] type       The operation type (READ, WRITE, SCAN).
    /// @param[out] key        The key for the operation.
    /// @param[out] value      The value string (non-empty only for WRITEs when gen_values is on).
    /// @param[out] scan_size  The scan length (non-zero only for SCAN operations).
    /// @return true if the workload has ended (no operation produced),
    ///         false if a valid operation was returned.
    bool next(Type& type, long& key, std::string& value, long& scan_size);

    /// Increment the acknowledged counter for the given key.
    /// Must be called by the user after a WRITE/INSERT is confirmed.
    void acknowledge(long key);

private:
    void init();

    // ── Configuration ──────────────────────────────────────────────────
    std::string export_path_;
    bool gen_values_;
    long value_min_size_;
    long value_max_size_;
    long key_seed_;
    long operation_seed_;
    int n_records_;
    int n_operations_;
    std::string data_distribution_str_;
    double read_proportion_;
    double update_proportion_;
    double insert_proportion_;
    double scan_proportion_;
    long scan_seed_;
    std::string scan_length_distribution_str_;
    int min_scan_length_;
    int max_scan_length_;

    // ── Phase tracking ─────────────────────────────────────────────────
    enum class Phase { LOADING, OPERATIONS, DONE };
    Phase phase_;
    int loading_index_;
    int operations_index_;
    long long n_requests_;

    acknowledged_counter<long>* insert_key_sequence_;

    // ── Generators ─────────────────────────────────────────────────────
    std::vector<std::pair<Type, double>> operation_proportions_;
    rfunc::RandFunction data_generator_;
    rfunc::RandFunction scan_length_generator_;
    rfunc::DoubleRandFunction operation_generator_;
    CharGenerator char_generator_;
    rfunc::RandFunction len_generator_;

    static const size_t MAX_VALUE_LEN = 10240;
};

} // namespace workload

#endif
