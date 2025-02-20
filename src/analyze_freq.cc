#include "common.h"

namespace po = boost::program_options;

void print_grouped_states(const std::vector<StateInfo>& grouped_states, 
                         int num_grouped, const std::set<char>& sign_states) {
  std::cout << "# of Grouped States [" << grouped_states.size() << "]" 
            << std::endl;
  for (int i = 0; i < grouped_states.size(); ++ i) {
    std::cout << std::setw(num_grouped) << std::setfill('0') 
              << "\tId [" << i << "], Grouped State [" << grouped_states[i].value << "], " 
              << "Count [" << grouped_states[i].count << "], "
              << "Each Significance [" << grouped_states[i].significance.first 
              << "], Sum Significance [" 
              << grouped_states[i].count * grouped_states[i].significance.first 
              << "]" << std::endl;
  }
}

void map_state(Block& block, int num_pages, int num_grouped, 
               const std::set<char>& sign_states, 
               std::map<std::string, int>& state_count_all) {
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

  // Sort the grouped states based on the state.
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

  // Merge the state count.
  for (const auto state_info: grouped_states) {
    if (state_count_all.find(state_info.value) != state_count_all.end()) {
      state_count_all[state_info.value] += state_info.count;
    } else {
      state_count_all[state_info.value] = state_info.count;
    }
  }

  // Print grouped states
  for (const auto state_info: grouped_states) {
    COUT_INFO("Block-State:[" << block.block_id << "," << state_info.value << "," << state_info.count << "]");
  }

  // Compute the entropy of the single states.
  double sum = 0;
  double sum_count = 0;
  for (const auto state_info: grouped_states) {
    sum_count += state_info.count;
  }
  for (const auto state_info: grouped_states) {
    double ratio = state_info.count * 1. / sum_count;
    sum += - ratio * log2(ratio);
  }
  COUT_INFO("Block-Entropy:[" << block.block_id << "," << sum << "]");
}

int main(int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("workload_path", po::value<std::string>(), 
     "the path of workload")
    ("workload_type", po::value<std::string>(), 
     "the workload type")
    ("num_pages", po::value<int>(), 
     "the number of pages")
    ("page_size", po::value<int>(), 
     "the page size in KB")
    ("num_grouped", po::value<int>(), 
     "the number of grouped states")
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

  check_options(vm, {"workload_path", "workload_type", "num_pages", "page_size", 
                     "num_grouped"});

  std::string workload_path = vm["workload_path"].as<std::string>();
  std::string workload_type = vm["workload_type"].as<std::string>();
  int num_pages = vm["num_pages"].as<int>();
  int page_size_in_bytes = vm["page_size"].as<int>() * 1024;
  int num_grouped = vm["num_grouped"].as<int>();
  std::set<char> significant_states;
  if (vm.count("significant_states")) {
    std::string states = vm["significant_states"].as<std::string>();
    for (auto s: states) {
      significant_states.insert(s);
    }
  }

  std::cout << "Significant Single State [";
  bool print_first = true;
  for (auto s: significant_states) {
    if (print_first) {
      print_first = false;
    } else {
      std::cout << ", ";
    }
    std::cout << s;
  }
  std::cout << "]" << std::endl;

  std::vector<Block> blocks;
  if (workload_type == "hex") {
    read_hex(workload_path, page_size_in_bytes, num_pages, blocks);
  } else if (workload_type == "binary") {
    int64_t file_size = get_file_size(workload_path);
    COUT_INFO("File size [" << file_size << "] bytes")
    read_binary(workload_path, page_size_in_bytes, num_pages, blocks, file_size);
  } else {
    std::cout << "Unsupport workload type [" << workload_type 
              << "]" << std::endl;
    exit(-1);
  }

  COUT_INFO("# blocks:" << blocks.size());
  std::map<std::string, int> state_count_all;
  uint32_t page_idx = 0;
  for (int i = 0; i < blocks.size(); ++ i, ++ page_idx) {
    map_state(blocks[i], num_pages, num_grouped, significant_states, state_count_all);
  }

  for (const auto kv: state_count_all) {
    COUT_INFO("\tOverall-State:[" << kv.first << "," << kv.second << "]");
  }
  return 0;
}