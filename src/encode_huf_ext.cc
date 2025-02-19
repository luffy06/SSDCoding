#include "common.h"

namespace po = boost::program_options;

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
      } else {
        return a.count > b.count;
      }
  });
  
  for (int i = 0, k = 0; i < grouped_states.size(); ++ i) {
    std::string value = grouped_states[i].value;
    if (mapping_tables.find(value) != mapping_tables.end()) {
      std::string mapped_value = mapping_tables[value];
      grouped_states[i].set_map_value(mapped_value, num_grouped + 1, sign_states);
    } else {
      if (k < maps.size()) {
        std::string base_value = maps[k];
        std::string mapped_value = base_value + (*(sign_states.begin()));
        std::string mapped_value_base = base_value + (*(sign_states.rbegin()));
        grouped_states[i].set_map_value(mapped_value, num_grouped + 1, sign_states);
        mapping_tables[grouped_states[i].value] = mapped_value;
        mapping_tables[base_value] = mapped_value_base;
        ++ k;
      } else {
        grouped_states[i].set_map_value(value, num_grouped, sign_states);
      }
    }
  }
}

void show_grouped_states(const std::vector<StateInfo>& grouped_states, 
                         int num_grouped) {
  bool print_first = true;
  std::cout << "# of Grouped States [" << grouped_states.size() << "]" 
            << std::endl;
  for (int i = 0; i < grouped_states.size(); ++ i) {
    std::cout << std::setw(num_grouped) << std::setfill('0') 
              << "\tGrouped State [" << grouped_states[i].value << "], " 
              << "Count [" << grouped_states[i].count << "], " 
              << "Mapped State [" << grouped_states[i].map_value << "]"
              << std::endl;
  }
}

void map_state(Block& block, int num_pages, int num_grouped, 
               const std::vector<std::string>& maps, 
               const std::set<char>& sign_states, 
               bool verbose=false) {  
  // Get single state.
  std::vector<char> single_states;
  single_states.reserve(block.page_size);
  {
    bool verbose_old = verbose;
    for (int i = 0; i < (block.page_size / sizeof(BIT_TYPE)); ++ i) {
      if (verbose) {
        std::cout << "Bits of Each Page" << std::endl;
        for (int k = 0; k < num_pages; ++ k) {
          std::cout << binarize(block.significant_bits[k][i], BIT_SIZE) 
                    << std::endl;
        }
      }
      for (int j = 0; j < BIT_SIZE; ++ j) {
        uint32_t state = 0;
        for (int k = 0; k < num_pages; ++ k) {
          BIT_TYPE bits = block.significant_bits[k][i];
          state |= (GET_BIT(bits, BIT_SIZE - j - 1) ? 1 << (num_pages - k - 1) 
                                                    : 0);
        }
        single_states.push_back(itoc(state, 1 << num_pages));
      }
      if (verbose) {
        std::cout << "Single States" << std::endl;
        for (int j = single_states.size() - BIT_SIZE; j < single_states.size(); 
             ++ j) {
          std::cout << single_states[j];
        }
        std::cout << std::endl;
        verbose = false;
      }
    }
    verbose = verbose_old;
  }
  
  // Group sinlge states, and count their frequency.
  std::vector<StateInfo> grouped_states;
  {
    std::map<std::string, StateInfo> frequency;
    for (int i = 0; i < single_states.size(); i += num_grouped) {
      std::string grouped_state = "";
      for (int j = 0; j < num_grouped; ++ j) {
        grouped_state += single_states[i + j];
      }
      if (frequency.find(grouped_state) != frequency.end()) {
        frequency[grouped_state].count ++;
      } else {
        frequency.insert({grouped_state, StateInfo(1, grouped_state, 
                                                   num_grouped, {})});
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
  
  std::map<std::string, std::string> mapping_tables;
  // Map the grouped states
  map_huffman(grouped_states, num_grouped, maps, sign_states, mapping_tables, 
              verbose);

  if (verbose) {
    std::cout << "Block [" << block.block_id << "] # Single States [" 
              << single_states.size() << "]" << std::endl;
    show_grouped_states(grouped_states, num_grouped);
  }

  auto replace = [&grouped_states](std::string grouped_state) {
    for (int k = 0; k < grouped_states.size(); ++ k) {
      if (grouped_states[k].value == grouped_state) {
        grouped_state = grouped_states[k].map_value;
        break;
      }
    }
    return grouped_state;
  };

  // Save mapped bits
  std::string grouped_state = "";
  for (int i = 0; i < (block.page_size / sizeof(BIT_TYPE)); ++ i) {
    for (int j = 0; j < BIT_SIZE; ++ j) {
      uint32_t state = 0;
      for (int k = 0; k < num_pages; ++ k) {
        BIT_TYPE bits = block.significant_bits[k][i];
        state |= (GET_BIT(bits, BIT_SIZE - j - 1) ? 1 << (num_pages - k - 1) 
                                                  : 0);
      }
      grouped_state += itoc(state, 1 << num_pages);
      if (grouped_state.size() == num_grouped) {
        grouped_state = replace(grouped_state);
        for (int k = 0; k < num_pages; ++ k) {
          block.mapped_bits_str[k].push_back("");
        }
        for (auto s : grouped_state) {
          int int_s = ctoi(s, 1 << num_pages);
          for (int k = 0; k < num_pages; ++ k) {
            *(block.mapped_bits_str[k].rbegin()) += itoc(GET_BIT(int_s, num_pages - k - 1));
          }
        }
        grouped_state = "";
      }
    }
  }
  if (grouped_state.size() > 0) {
    grouped_state = replace(grouped_state);
    for (int k = 0; k < num_pages; ++ k) {
      block.mapped_bits_str[k].push_back(grouped_state);
    }
  }
  // if (verbose) {
  //   for (int k = 0; k < num_pages; ++ k) {
  //     for (auto s : block.mapped_bits_str[k]) {
  //       std::cout << s;
  //     }
  //     std::cout << std::endl;
  //   }
  // }
}

void check_options(const po::variables_map& vm, 
                   const std::vector<std::string>& options) {
  for (auto op : options) {
    if (!vm.count(op)) {
      std::cout << "--" << op << " option required" << std::endl;
    }
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
    ("config_path", po::value<std::string>(), 
     "the config path")
    ("num_pages", po::value<int>(), 
     "the number of pages")
    ("page_size", po::value<int>(), 
     "the page size in KB")
    ("num_grouped", po::value<int>(), 
     "the number of grouped states")
    ("verbose", "verbose")
    ("significant_states", po::value<std::string>(), 
     "the list of significant state")
    ("num_maps", po::value<int>(), 
     "the number of mapping")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (...) {
    COUT_ERR("Unrecognized parameters, please use --help");
  }
  po::notify(vm);

  check_options(vm, {"workload_path", "workload_type", "output_path", "page_size", "num_grouped"});

  std::string workload_path = vm["workload_path"].as<std::string>();
  std::string workload_type = vm["workload_type"].as<std::string>();
  std::string output_path = vm["output_path"].as<std::string>();
  int num_pages = vm["num_pages"].as<int>();
  int page_size_in_bytes = vm["page_size"].as<int>() * 1024;
  int num_grouped = vm["num_grouped"].as<int>();
  bool verbose = vm.count("verbose");
  std::set<char> significant_states;
  if (vm.count("significant_states")) {
    std::string states = vm["significant_states"].as<std::string>();
    for (auto s: states) {
      significant_states.insert(s);
    }
  }
  std::vector<uint32_t> page_config = {};
  if (vm.count("config_path")) {
    std::string config_path = vm["config_path"].as<std::string>();
    load_config(config_path, page_config);
  }
  int num_maps = -1;
  if (vm.count("num_maps")) {
    num_maps = vm["num_maps"].as<int>();
  }

  std::vector<Block> blocks;
  uint64_t file_size = 0;
  if (workload_type == "hex") {
    read_hex(workload_path, page_size_in_bytes, num_pages, blocks);
  } else if (workload_type == "binary") {
    file_size = read_binary(workload_path, page_size_in_bytes, 
                            num_pages, blocks);
  } else {
    std::cout << "Unsupport workload type [" << workload_type 
              << "]" << std::endl;
    exit(-1);
  }
  std::vector<std::string> maps;
  build_mapping_states(num_pages, num_grouped, significant_states, maps);
  if (num_maps != -1) {
    maps.resize(num_maps);
  }
  if (verbose) {
    bool first = true;
    std::cout << significant_states.size() << " significant states" << std::endl;
    for (auto s: significant_states) {
      if (first) {
        first = false;
      } else {
        std::cout << ", ";
      }
      std::cout << s;
    }
    std::cout << std::endl;
    first = true;
    std::cout << maps.size() << " candidate mapped value" << std::endl;
    for (auto s: maps) {
      if (first) {
        first = false;
      } else {
        std::cout << ", ";
      }
      std::cout << s;
    }
    std::cout << std::endl;
  }

  for (int i = 0; i < blocks.size(); ++ i) {
    map_state(blocks[i], num_pages, num_grouped, maps, significant_states, verbose);
    if (verbose) {
      break;
    }
  }
  std::cout << std::fixed << std::setprecision(6) << file_size << std::endl;

  std::fstream out(output_path, std::ios::out | std::ios::binary);
  uint32_t page_idx = 0;
  std::vector<BIT_TYPE> buffer;
  int buffer_size = 10000;
  BIT_TYPE bit = 0;
  int num_used = sizeof(BIT_TYPE) * 8;
  for (uint32_t i = 0; i < blocks.size(); ++ i, ++ page_idx) {
    Block& block = blocks[i];
    for (int j = 0; j < num_pages; ++ j) {
      for (auto state : block.mapped_bits_str[j]) {
        for (auto s : state) {
          bit <<= 1;
          bit |= (s == '0' ? 0 : 1);
          num_used --;
          if (num_used == 0) {
            buffer.push_back(bit);
            bit = 0;
            num_used = sizeof(BIT_TYPE) * 8;
            if (buffer.size() > buffer_size) {
              out.write(reinterpret_cast<const char*>(buffer.data()), 
                        buffer.size());
              buffer.clear();
            }
          }
        }
      }
    }
  }
  if (num_used < sizeof(BIT_TYPE) * 8) {
    buffer.push_back(bit);
  }
  if (buffer.size() > 0) {
    out.write(reinterpret_cast<const char*>(buffer.data()), 
              buffer.size());
    buffer.clear();
  }
  out.close();
  uint64_t output_file_size = get_file_size(output_path);
  std::cout << "File size after using huffman coding " <<  output_file_size 
            << " B, approximate " << std::fixed << std::setprecision(2) 
            << output_file_size / (1024. * 1024) << " MB" << std::endl;
  return 0;
}