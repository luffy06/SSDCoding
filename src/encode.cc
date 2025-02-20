#include "common.h"

namespace po = boost::program_options;

std::vector<double> counting_time;
uint32_t num_heuristics = 0;
uint32_t num_huffman = 0;

void map_heuristics(std::vector<StateInfo>& grouped_states, int num_grouped, 
                     const std::vector<std::string>& maps, 
                     const std::set<char>& sign_states, 
                     std::map<std::string, std::string>& mapping_tables,
                     bool verbose=false) {
  std::set<std::string> remap;          // States need to be mapped
  std::vector<std::string> candidates;  // Other states can map to candidates

  std::sort(grouped_states.begin(), grouped_states.end(), 
    [](auto const& a, auto const& b) {
      if (a.count != b.count) {
        return a.count > b.count;
      } else if (!compare(a.significance.first, b.significance.first)) {
        return a.significance.first < b.significance.first;
      } else {
        return a.significance.second > b.significance.second;
      }
  });

  for (int i = 0, k = 0; i < grouped_states.size(); ++ i) {
    if (i < maps.size()) {
      // Map grouped_states[i].value to maps[i]
      mapping_tables[grouped_states[i].value] = maps[i];
      // Add grouped_states[i].value into the candiadate set
      candidates.push_back(grouped_states[i].value);
      // Add maps[i] into the remapping set waiting to be remapped
      remap.insert(maps[i]);
      // Check remap whether current grouped_states[i].value is in it
      // If grouped_states[i].value exists, remove it since it has been remapped
      auto it = remap.find(grouped_states[i].value);
      if (it != remap.end()) {
        remap.erase(it);
      }
    } else {
      // All items in the constructed maps have been used, so we only do the 
      // remapping
      auto it = remap.find(grouped_states[i].value);
      if (it != remap.end()) {
        if (k == 0) {
          // Preprocess for candidate set
          // Remove the states in the candidate set that have been mapped
          std::vector<std::string> candidates_after_erase;
          candidates_after_erase.reserve(candidates.size());
          for (int v = 0; v < maps.size(); ++ v) {
            for (int u = 0; u < candidates.size(); ++ u) {
              if (candidates[u] != maps[v]) {
                candidates_after_erase.push_back(candidates[u]);
              }
            }
          }
          candidates.clear();
          candidates.insert(candidates.begin(), candidates_after_erase.begin(), candidates_after_erase.end());
          // Sort all candidate based on frequency
          std::sort(candidates.begin(), candidates.end(), 
            [&sign_states](auto const& gs_a, auto const& gs_b) {
              auto sign_a = compute_signficance(gs_a, sign_states);
              auto sign_b = compute_signficance(gs_b, sign_states);
              if (sign_a.first != sign_b.first) {
                return sign_a.first > sign_b.first;
              } else {
                return sign_a.second < sign_b.second;
              }
          });
        }
        remap.erase(it);
        mapping_tables[grouped_states[i].value] = candidates[k];
        k ++;
      }
    }
  }
}

void map_huffman(std::vector<StateInfo>& grouped_states, int num_grouped, 
                 const std::vector<std::string>& maps, 
                 const std::set<char>& sign_states, 
                 std::map<std::string, std::string>& mapping_tables,
                 bool verbose=false) {
  std::sort(grouped_states.begin(), grouped_states.end(), 
    [](auto const& a, auto const& b) {
      if (!compare(a.significance.first, b.significance.first)) {
        return a.significance.first < b.significance.first;
      } else if (!compare(a.significance.second, b.significance.second)) {
        return a.significance.second > b.significance.second;
      } else if (a.count != b.count) {
        return a.count > b.count;
      } else {
        return a.id > b.id;
      }
  });
  
  for (int i = 0, k = 0; i < grouped_states.size(); ++ i) {
    std::string value = grouped_states[i].value;
    if (mapping_tables.find(value) == mapping_tables.end() && k < maps.size()) {
      std::string base_value = maps[k];
      std::string mapped_value = base_value + (*(sign_states.begin()));
      std::string mapped_value_base = base_value + (*(sign_states.rbegin()));
      mapping_tables[grouped_states[i].value] = mapped_value;
      mapping_tables[base_value] = mapped_value_base;
      ++ k;
    }
  }
}

void map_state(Block& block, int num_pages, int num_grouped, 
               const std::vector<std::string>& maps, 
               const std::set<char>& sign_states, 
               std::map<std::string, std::string>& mapping_tables,
               bool build_table=true,
               double upper_threshold=0.28,
               double lower_threshold=0.22,
               bool verbose=false, bool show_tables=false, 
               bool activate_map=true) {
  // Get single state.
  std::vector<char> single_states;
  single_states.reserve(block.page_size);
  {
    for (int i = 0; i < (block.page_size / sizeof(BIT_TYPE)); ++ i) {
      for (int j = 0; j < BIT_SIZE; ++ j) {
        uint32_t state = 0;
        for (int k = 0; k < num_pages; ++ k) {
          BIT_TYPE bits = block.significant_bits[k][i];
          state |= (GET_BIT(bits, BIT_SIZE - j - 1) ? 1 << (num_pages - k - 1) 
                                                    : 0);
        }
        single_states.push_back(itoc(state, 1 << num_pages));
      }
    }
  }

  double sign_ratio = 0;
  {
    std::map<char, uint32_t> state_count;
    for (const auto s: single_states) {
      if (state_count.find(s) == state_count.end()) {
        state_count[s] = 1;
      } else {
        state_count[s] ++;
      }
    }
    double sum = 0;
    double sum_sign = 0;
    for (const auto kv: state_count) {
      if (sign_states.find(kv.first) != sign_states.end()) {
        sum_sign += kv.second;
      }
      sum += kv.second;
    }
    sign_ratio = sum_sign / sum;
  }
  // Group sinlge states, and count their frequency.
  std::vector<StateInfo> grouped_states;
  {
    std::map<std::string, StateInfo> frequency;
    for (int i = 0, k = 0; i < single_states.size(); i += num_grouped) {
      std::string grouped_state = "";
      for (int j = 0; j < num_grouped; ++ j) {
        grouped_state += single_states[i + j];
      }
      if (frequency.find(grouped_state) != frequency.end()) {
        frequency[grouped_state].count ++;
      } else {
        frequency.insert({grouped_state, StateInfo(k, 1, grouped_state, 
                                                   num_grouped, sign_states)});
        ++ k;  
      }
    }
    grouped_states.reserve(frequency.size());
    for (auto state : frequency) {
      grouped_states.push_back(state.second);
    }
  }

  // Sort the grouped states based on the frequency.
  std::sort(grouped_states.begin(), grouped_states.end(), 
    [](auto const& a, auto const& b) {
      if (a.count != b.count) {
        return a.count > b.count;
      } else if (!compare(a.significance.first, b.significance.first)) {
        return a.significance.first > b.significance.first;
      } else {
        return a.significance.second < b.significance.second;
      }
  });

  if (verbose) {
    COUT_INFO("Start to map block-" << block.block_id)
  }
  // Generate mapping tables
  if (build_table) {
    mapping_tables.clear();
    if (activate_map) {
      if (sign_ratio < upper_threshold && sign_ratio > lower_threshold) {
        map_huffman(grouped_states, num_grouped, maps, sign_states, 
                    mapping_tables, verbose);
        num_huffman ++;
      } else {
        map_heuristics(grouped_states, num_grouped, maps, sign_states, 
                      mapping_tables, verbose);
        num_heuristics ++;
      }
    }
  }

  uint32_t table_size_in_bits = 0;
  for (auto kv: mapping_tables) {
    table_size_in_bits += (kv.first.size() + kv.second.size());
  }
  table_size_in_bits *= num_pages;

  if (show_tables) {
    for (auto kv: mapping_tables) {
      uint32_t count = 0;
      for (uint32_t i = 0; i < grouped_states.size(); ++ i) {
        if (grouped_states[i].value == kv.first) {
          count = grouped_states[i].count;
        }
      }
      COUT_INFO("Map [" << kv.first << ", " << count << "] to [" << kv.second << "]")
    }
    COUT_INFO("")
  }

  auto replace = [&mapping_tables](std::string grouped_state) {
    if (mapping_tables.find(grouped_state) != mapping_tables.end()) {
      return mapping_tables[grouped_state];
    }
    return grouped_state;
  };

  uint32_t origin_size = 0;
  uint32_t mapped_size = 0;
  uint32_t origin_sign_size = 0;
  uint32_t mapped_sign_size = 0;
  // Save mapped bits
  for (int i = 0; i < single_states.size(); i += num_grouped) {
    std::string grouped_state = "";
    for (int j = 0; j < num_grouped; ++ j) {
      grouped_state += single_states[i + j];
    }
    if (verbose) {
      origin_size += grouped_state.size();
      auto res = compute_signficance(grouped_state, sign_states);
      origin_sign_size += res.first;
    }
    grouped_state = replace(grouped_state);
    if (verbose) {
      mapped_size += grouped_state.size();
      auto res = compute_signficance(grouped_state, sign_states);
      mapped_sign_size += res.first;
    }
    for (auto state: grouped_state) {
      uint32_t state_dec = ctoi(state, 1 << num_pages);
      for (int k = 0; k < num_pages; ++ k) {
        BIT_TYPE bit = GET_BIT(state_dec, num_pages - k - 1);
        ASSERT_WITH_MSG(bit == 0 || bit == 1, "Wrong bit [" << int(bit) << "]");
        block.mapped_bits[k].push_back(bit);
      }
    }
  }
  if (verbose) {
    COUT_INFO("Origin size [" << origin_size << "], mapped size [" 
              << mapped_size << "], overhead ratio [" << std::setprecision(2)
              << (mapped_size - origin_size) * 1. / origin_size << "]\n" 
              << "Origin sigificance ratio [" << std::setprecision(2) 
              << origin_sign_size * 1. / origin_size << "], " 
              << "mapped significance ratio [" << std::setprecision(2) 
              << mapped_sign_size * 1. / mapped_size << "]\n"
              << "Table size [" << table_size_in_bits << "] bits")
  }
}

void dfs(int pos, std::string grouped_state, int dec, int num_grouped, 
         std::vector<std::string>& maps) {
  if (pos >= num_grouped) {
    maps.push_back(grouped_state);
    return ;
  }
  for (int i = 0; i < dec; ++ i) {
    dfs(pos + 1, grouped_state + itoc(i, dec), dec, num_grouped, maps);
  }
}

void build_mapping_states(int num_pages, int num_grouped, 
                          const std::set<char>& sign_states, 
                          std::vector<std::string>& maps) {
  dfs(0, "", (1 << num_pages), num_grouped, maps);
  std::sort(maps.begin(), maps.end(), 
    [&sign_states](auto const& gs_a, auto const& gs_b) {
      auto sign_a = compute_signficance(gs_a, sign_states);
      auto sign_b = compute_signficance(gs_b, sign_states);
      if (sign_a.first != sign_b.first) {
        return sign_a.first > sign_b.first;
      } else {
        return sign_a.second < sign_b.second;
      }
  });
}

int main(int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("workload_path", po::value<std::string>(), 
     "the path of workload")
    ("workload_type", po::value<std::string>(), 
     "the workload type")
    ("output_path", po::value<std::string>(), 
     "the output path")
    ("num_pages", po::value<int>(), 
     "the number of pages")
    ("page_size", po::value<int>(), 
     "the page size in KB")
    ("num_grouped", po::value<int>(), 
     "the number of grouped states")
    ("table_size", po::value<int>(), 
     "the number of mapping entries in mapping table")
    ("build_table_once", "build_table_once")
    ("max_file_size", po::value<int>(), 
     "the maximal file size in bytes to be mapped")
    ("upper_threshold", po::value<double>(), 
     "the upper threshold")
    ("lower_threshold", po::value<double>(), 
     "the lower threshold")
    ("verbose", "verbose")
    ("show_maps", "show_maps")
    ("show_tables", "show_tables")
    ("significant_states", po::value<std::string>(), 
     "the list of significant state")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (...) {
    COUT_ERR("Unrecognized parameters, please use --help");
  }
  po::notify(vm);

  check_options(vm, {"workload_path", "workload_type", "output_path", 
                     "num_pages", "page_size", "num_grouped", "table_size"});

  std::string workload_path = vm["workload_path"].as<std::string>();
  std::string workload_type = vm["workload_type"].as<std::string>();
  std::string output_path = vm["output_path"].as<std::string>();
  int num_pages = vm["num_pages"].as<int>();
  int page_size_in_bytes = vm["page_size"].as<int>() * 1024;
  int num_grouped = vm["num_grouped"].as<int>();
  int table_size = vm["table_size"].as<int>();
  bool build_table_once = vm.count("build_table_once");
  double upper_threshold = vm["upper_threshold"].as<double>();
  double lower_threshold = vm["lower_threshold"].as<double>();
  bool verbose = vm.count("verbose");
  bool show_maps = vm.count("show_maps");
  bool show_tables = vm.count("show_tables");
  int num_printed_blocks = 0;
  if (vm.count("num_printed_blocks")) {
    num_printed_blocks = vm["num_printed_blocks"].as<int>();
  }
  std::set<char> significant_states;
  if (vm.count("significant_states")) {
    std::string states = vm["significant_states"].as<std::string>();
    for (auto s: states) {
      significant_states.insert(s);
    }
  }
  bool activate_map = significant_states.size() > 0;
  int num_maps = table_size / 2;
  ASSERT_WITH_MSG(page_size_in_bytes % sizeof(BIT_TYPE) == 0, "Bad page size [" 
                  << page_size_in_bytes << "]")

  // Load data
  std::vector<Block> blocks;
  if (workload_type == "hex") {
    read_hex(workload_path, page_size_in_bytes, num_pages, blocks);
  } else if (workload_type == "binary") {
    if (verbose) {
      int64_t file_size = get_file_size(workload_path);
      COUT_INFO("Origin file size [" << file_size << "] bytes")
    }
    uint64_t max_file_size = (vm.count("max_file_size") ? vm["max_file_size"].as<int>() : 0);
    if (max_file_size) {
      read_binary(workload_path, page_size_in_bytes, num_pages, blocks, max_file_size);
    } else {
      read_binary(workload_path, page_size_in_bytes, num_pages, blocks);
    }
  } else {
    std::cout << "Unsupport workload type [" << workload_type 
              << "]" << std::endl;
    exit(-1);
  }

  // Construct candidate maps
  if (verbose) {
    for (auto s: significant_states) {
      std::cout << "Significant State [" << s << "]" << std::endl;
    }
  }
  std::vector<std::string> maps;
  build_mapping_states(num_pages, num_grouped, significant_states, maps);

  if (show_maps) {
    std::cout << "Constructed Maps [" << num_maps << "/" << maps.size() 
              << "]" << std::endl;
    for (int i = 0; i < num_maps && i < maps.size(); ++ i) {
      if (i > 0 && i % 6 == 0) {
        std::cout << std::endl;
      }
      std::cout << std::setw(num_grouped) << std::setfill('0') << maps[i] 
                << "\t";
    }
    std::cout << std::endl;
  }

  if (maps.size() > num_maps) {
    maps.resize(num_maps);
  }

  num_heuristics = 0;
  num_huffman = 0;
  std::map<std::string, std::string> mapping_tables;
  for (int i = 0; i < blocks.size(); ++ i) {
    if (build_table_once) {
      if (i == 0) {
        map_state(blocks[i], num_pages, num_grouped, maps, significant_states, 
                  mapping_tables, true, upper_threshold, lower_threshold, 
                  verbose, show_tables, activate_map);
      } else {
        map_state(blocks[i], num_pages, num_grouped, maps, significant_states, 
                  mapping_tables, false, upper_threshold, lower_threshold, 
                  verbose, show_tables, activate_map);
      }
    } else {
      map_state(blocks[i], num_pages, num_grouped, maps, significant_states, 
                mapping_tables, true, upper_threshold, lower_threshold, 
                verbose, show_tables, activate_map);
    }
  }
  COUT_INFO("# blocks " << blocks.size() << ", # mapping using huffman " 
            << num_huffman << ", # mapping using heuristics " << num_heuristics)

  // Write mapped data into files
  std::vector<BIT_TYPE> merged_data;
  BIT_TYPE merged = 0;
  uint32_t num_slots = BIT_SIZE;
  for (uint32_t i = 0; i < blocks.size(); ++ i) {
    Block& block = blocks[i];
    for (uint32_t k = 0; k < num_pages; ++ k) {
      for (uint32_t j = 0; j < block.mapped_bits[k].size() 
            && j < page_size_in_bytes * 8; ++ j) {
        if (num_slots == 0) {
          merged_data.push_back(merged);
          merged = 0;
          num_slots = BIT_SIZE;
        }
        merged = (merged << 1) | (block.mapped_bits[k][j] ? 1 : 0);
        num_slots --;
      }
    }
  }
  if (num_slots != BIT_SIZE) {
    merged_data.push_back(merged);
  }

  std::fstream out(output_path, std::ios::out | std::ios::binary);
  out.write(reinterpret_cast<const char*>(merged_data.data()), 
            sizeof(BIT_TYPE) * merged_data.size());
  out.close();

  if (verbose) {
    int64_t output_size = get_file_size(output_path);
    COUT_INFO("Mapped file size [" << output_size << "] bytes")
  }
  return 0;
}
