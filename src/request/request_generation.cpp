#include "request_generation.h"
#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <random>
#include <algorithm>
#include <assert.h>

#include "types.h"
#include "scrambled_zipfian_int_distribution.h"
#include "acknowledged_counter.h"
#include "skewed_latest_int_distribution.h"

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void print_progress(float percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%.2f%% [%.*s%*s]", percentage*100, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

namespace workload {
using namespace std;
using namespace rfunc;

static const size_t MAX_VALUE_LEN = 10240;

const char CharGenerator::__CHARSET[] ="     ,;:.!?0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

const size_t CharGenerator::__CHARSET_LEN = 73;


long gen_value(char* out_str, CharGenerator *char_generator, RandFunction *len_generator) {
    long length = (*len_generator)();
    for (long i = 0; i < length; i++)
    {
        out_str[i] = (*char_generator)();
    }
    out_str[length] = '\0';
    return length;
}

Type next_operation(
    vector<pair<Type,double>> values, 
    DoubleRandFunction *generator
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

void generate_export_requests(
    string config_path, const toml::value& config
) {
    cout << "Generating " << config_path << " ..."  << endl; 
    vector<pair<Type, double>> operation_proportions;
    long long n_requests = 0;
    string export_path = toml::find<string>(
        config, "output", "requests", "export_path"
    );

    bool gen_values = toml::find<bool>(
        config, "workload", "gen_values"
    );

    long value_min_size  = toml::find<long>(
        config, "workload", "value_min_size"
    );

    long value_max_size  = toml::find<long>(
        config, "workload", "value_max_size"
    );

    const long key_seed = toml::find<long>(
        config, "workload", "key_seed"
    );

    const long operation_seed = toml::find<long>(
        config, "workload", "operation_seed"
    );

    const int n_records = toml::find<int>(
        config, "workload", "n_records"
    );
    acknowledged_counter<long> *insertkeysequence = new acknowledged_counter<long>(n_records);
    
    const int n_operations = toml::find<int>(
        config, "workload", "n_operations"
    );
    n_requests = n_operations;

    const string data_distribution_str = toml::find<string>(
        config, "workload", "data_distribution"
    );

    const double read_proportion = toml::find<double>(
        config, "workload", "read_proportion"
    );
    if(read_proportion>0){
        operation_proportions.push_back(make_pair(Type::READ,read_proportion));
    }

    const double scan_proportion = toml::find<double>(
        config, "workload", "scan_proportion"
    );

    const double update_proportion = toml::find<double>(
        config, "workload", "update_proportion"
    );
    if(update_proportion>0){
        operation_proportions.push_back(make_pair(Type::UPDATE,update_proportion));
    }

    const double insert_proportion = toml::find<double>(
        config, "workload", "insert_proportion"
    );
    if(insert_proportion>0){
        operation_proportions.push_back(make_pair(Type::WRITE,insert_proportion));
    }

    Distribution data_distribution = str_to_dist(data_distribution_str);

    RandFunction data_generator;
    if (data_distribution == UNIFORM) {
        data_generator = uniform_distribution_rand(
            0, n_records, key_seed
        );
    } else if (data_distribution == ZIPFIAN) {
        int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
        data_generator = scrambled_zipfian_distribution(0, n_records + expectednewkeys, key_seed);
    }  else if (data_distribution == LATEST) {
        zipfian_int_distribution<long>* zip = new zipfian_int_distribution<long>(0, insertkeysequence->last_value());
        data_generator = skewed_latest_distribution(insertkeysequence, zip, key_seed);
        //leaking
    }

    RandFunction scan_length_generator;
    if(scan_proportion > 0){

        const long scan_seed = toml::find<long>(
            config, "workload", "scan_seed"
        );

        operation_proportions.push_back(make_pair(Type::SCAN,scan_proportion));
    
        const string scan_length_distribution_str = toml::find<string>(
            config, "workload", "scan_length_distribution"
        );
        
        const int min_scan_length = toml::find<int>(
            config, "workload", "min_scan_length"
        );

        const int max_scan_length = toml::find<int>(
            config, "workload", "max_scan_length"
        );
        Distribution scan_length_distribution = str_to_dist(scan_length_distribution_str);

        if (scan_length_distribution == UNIFORM) {
            scan_length_generator = uniform_distribution_rand(
                min_scan_length, max_scan_length, scan_seed
            );
        } else if (scan_length_distribution == ZIPFIAN) {
            int expectednewkeys = (int) ((n_operations) * insert_proportion * 2.0);
            scan_length_generator = scrambled_zipfian_distribution(0, n_records, scan_seed);
        }

    }

    DoubleRandFunction operation_generator = uniform_double_distribution_rand(
        0.0, 1.0, operation_seed
    );

    CharGenerator char_generator;
    RandFunction len_generator;
    if (gen_values){
        char_generator = CharGenerator();

        len_generator = uniform_distribution_rand(value_min_size, value_max_size);
        
    }

    char value[MAX_VALUE_LEN+1];
    float total = n_records + n_operations;
    float progress = 0;

    ofstream ofs(export_path, ofstream::out);
    for (size_t i = 0; i < n_records; i++)
    {
        ofs << static_cast<int>(WRITE) << "," << i;
        if (gen_values) {
            gen_value(value, &char_generator, &len_generator);
            ofs << "," << value;
        }
        ofs << endl;
        progress = i/total;
        print_progress(progress);
    }
    
    for (int i = 0; i < n_operations; i++) {
        Type type = next_operation(operation_proportions, &operation_generator);
        int key, size;
        if(type == Type::READ || type == Type::UPDATE){
            do{
                key = data_generator();
            } while(key >= insertkeysequence->last_value());
            if(type == Type::UPDATE){
                type = Type::WRITE;
            }
        }else if(type == Type::SCAN){
            size = scan_length_generator();
            n_requests += (size-1);
            do{
                key = data_generator();
            } while(key+size >= insertkeysequence->last_value());
        } else if(type == Type::WRITE){
            key = insertkeysequence->next();
            insertkeysequence->acknowledge(key);
        }

        if (type == Type::READ) {
            ofs << type << "," << key << endl;
        } else if (type == Type::WRITE) {
            ofs << type << "," << key;
            if (gen_values) {
                gen_value(value, &char_generator, &len_generator);
                ofs << "," << value;
            }
            ofs << endl;
        } else if (type == Type::SCAN) {
            ofs << type << "," << key << "," << size << endl;
        }


        progress = (i+n_records)/total;
        print_progress(progress);

       
    }
    cout << "number of writes/reads to keys: " << n_requests << endl; 
    cout << "Generated into " << export_path  << endl; 
    ofs.flush();
    ofs.close();
}

void create_requests(
    string config_path
) {
    const auto config = toml::parse(config_path);

    const bool is_one_distribution = toml::find<bool>(
        config, "workload", "single_distribution"
    );

    if(is_one_distribution){
        generate_export_requests(config_path, config);
    }else{
    }
}


}
