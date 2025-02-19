#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <getopt.h>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <thread>
#include <vector>
#include <unistd.h>

#define COUT_INFO(this) std::cout << std::fixed << this << std::endl;
#define COUT_ERR(this) \
  std::cerr << std::fixed << this << std::endl; \
  assert(false);

#define UNUSED(var) ((void)var)
#define ASSERT_WITH_MSG(cond, msg) \
  { \
    if (!(cond)) { \
      COUT_ERR("Assertion at " << __FILE__ << ":" << __LINE__ << ", error: " << msg) \
    } \
  }

#define TIME_LOG (std::chrono::high_resolution_clock::now())
#define TIME_IN_NANO_SECOND(begin, end) (std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count())
#define TIME_IN_SECOND(begin, end) (TIME_IN_NANO_SECOND(begin, end) / 1e9)

#define BIT_TYPE uint8_t
#define BIT_SIZE (sizeof(BIT_TYPE) * 8)
#define GET_BIT(x, n) (((x) >> (n)) & 1)
#define BIT_NUM(n) (static_cast<int>(1) << (n))

const long long kSEED = 1e9 + 7;
const int kINF = 0x7fffffff;

// Common functions
void check_options(const boost::program_options::variables_map& vm, 
                   const std::vector<std::string>& options) {
  for (auto op : options) {
    if (!vm.count(op)) {
      std::cout << "--" << op << " option required" << std::endl;
    }
  }
}

template<typename T>
inline bool compare(const T& a, const T& b) {
  if (std::numeric_limits<T>::is_integer) {
    return a == b;
  } else {
    return std::fabs(a - b) < std::numeric_limits<T>::epsilon();
  }
}

template<typename T>
std::string str(T n) {
  std::stringstream ss;
  ss << std::setprecision(std::numeric_limits<T>::digits10) << std::fixed << n;
  std::string n_str = ss.str();
  if (n_str.find(".") != std::string::npos) {
    while (*(n_str.rbegin()) == '0') {
      n_str.pop_back();
    }
    if (*(n_str.rbegin()) == '.') {
      n_str.pop_back();
    }
  }
  return n_str;
}

template<typename T, typename P>
P ston(T s) {
  std::string ss = str<T>(s);
  P v = 0;
  int point = -1;
  bool negative = (ss[0] == '-');
  for (int i = (negative ? 1 : 0); i < ss.size(); ++ i) {
    if (ss[i] >= '0' && ss[i] <= '9') {
      v = v * 10 + (ss[i] - '0');
    } else if (point == -1 && ss[i] == '.') {
      point = ss.size() - i - 1;
    } else {
      break;
    }
  }
  for (int i = 0; i < point; ++ i) {
    v = v / 10.;
  }
  if (negative) {
    v = -v;
  }
  return v;
}

char itoc(int n, int dec=10) {
  ASSERT_WITH_MSG(n < dec, "Convert illegal value [" << n << "] in " << dec)
  if (n >= 10) {
    return 'A' + n % 10;
  } else {
    return '0' + n;
  }
}

int ctoi(char c, int dec=10) {
  if (dec <= 10) {
    ASSERT_WITH_MSG(c >= '0' && c <= '9', 
                    "Convert illegal char [" << c << "] in " << dec)
  } else {
    ASSERT_WITH_MSG((c >= '0' && c <= '9') || (c >= 'A' && c <= 'A' + ((dec - 1) % 10)), 
                    "Convert illegal char [" << c << "] in " << dec)
  }
  if (c >= 'A') {
    return c - 'A' + 10;
  } else {
    return c - '0';
  }
}

std::string strip(std::string s, char c=' ') {
  int l = 0;
  int r = s.size() - 1;
  while (l < r && s[l] == c) {
    l ++;
  }
  while (l < r && s[r] == c) {
    r --;
  }
  if (l > r) {
    return "";
  } else {
    return s.substr(l, r - l + 1);
  }
}

std::vector<std::string> split(std::string s, char sep) {
  std::vector<std::string> res;
  std::string tmp = "";
  for (uint32_t i = 0; i < s.size(); ++ i) {
    if (s[i] == sep) {
      res.push_back(tmp);
      tmp = "";
    } else {
      tmp += s[i];
    }
  }
  res.push_back(tmp);
  return res;
}

std::pair<int, double> compute_signficance(const std::string& gs, 
                                           const std::set<char>& sign_states) {
  int sum = 0;
  double var = 0;
  std::map<char, int> frequency;
  for (auto s: sign_states) {
    frequency[s] = 0;
  }
  for (int i = 0; i < gs.size(); ++ i) {
    if (sign_states.find(gs[i]) != sign_states.end()) {
      frequency[gs[i]] += 1;
      sum ++;
    }
  }
  for (auto x : frequency) {
    double diff = x.second - sum * 1. / frequency.size();
    var += diff * diff;
  }
  return {sum, var};
}

struct StateInfo {
  int id;
  int count;
  std::string value;
  std::map<char, int> num;
  std::pair<int, double> significance;

  StateInfo() : id(0), count(0), value(""), significance({0, 0}) { }

  StateInfo(int i, int c, std::string v, int num_grouped, 
            const std::set<char>& sign_states) 
            : id(i), value(v), count(c) {
    set_value(v, num_grouped, sign_states);
  }

  void set_value(std::string val, int num_grouped, 
                 const std::set<char>& sign_states) {
    value = val;
    num.clear();
    for (int i = 0; i < num_grouped; ++ i) {
      if (num.find(val[i]) != num.end()) {
        num[val[i]] ++;
      } else {
        num[val[i]] = 1;
      }
    }
    significance = compute_signficance(value, sign_states);    
  }

  StateInfo& operator=(const StateInfo& s) {
    this->id = s.id;
    this->count = s.count;
    this->value = s.value;
    this->num.clear();
    for (auto i: s.num) {
      this->num.insert(i);
    }
    this->significance = s.significance;
    return *this;
  }
};

struct HuffmanNode {
  int id;
  StateInfo* data;
  int freq;
  HuffmanNode* left;
  HuffmanNode* right;

  HuffmanNode(int i, StateInfo* state, int f, HuffmanNode* l, HuffmanNode* r) {
    id = i;
    data = state;
    freq = f;
    left = l;
    right = r;
  }
};

class CompareHuffmanNode {
public:
  bool operator() (HuffmanNode* a, HuffmanNode* b) {
    return a->freq > b->freq;
  }
};

struct Block {
  int block_id;
  int page_size;
  std::vector<std::vector<BIT_TYPE>> significant_bits;
  std::vector<std::vector<BIT_TYPE>> mapped_bits;

  explicit Block(int id, int p, int num_pages) 
                : block_id(id), page_size(p) {
    significant_bits.resize(num_pages);
    mapped_bits.resize(num_pages);
    for (int i = 0; i < num_pages; ++ i) {
      significant_bits[i].reserve(p / sizeof(BIT_TYPE));
      mapped_bits[i].reserve(p / sizeof(BIT_TYPE));
    }
  }
};

// Functions for loading/dumping data
void decode_hex(const std::vector<std::string>& data, int page_size_in_bytes, 
                int num_pages, Block& block) {
  std::map<char, uint8_t> hex_map = {{'0', 0}, {'1', 1}, {'2', 2}, 
                                      {'3', 3}, {'4', 4}, {'5', 5}, 
                                      {'6', 6}, {'7', 7}, {'8', 8}, 
                                      {'9', 9}, {'a', 10}, {'b', 11}, 
                                      {'c', 12}, {'d', 13}, {'e', 14}, 
                                      {'f', 15}};
  for (uint i = 0; i < num_pages; ++ i) {
    for (uint j = 0; j < page_size_in_bytes * 8 / 4; j += 2) {
      char hex1 = data[i][j];
      char hex2 = data[i][j + 1];
      block.significant_bits[i].push_back((hex_map[hex1] << 4) | hex_map[hex2]);
    }
  }
}

void read_hex(std::string workload_path, int page_size_in_bytes,  
              int num_pages, std::vector<Block>& blocks) {
  std::fstream in(workload_path, std::ios::in);
  int num_pages_total = 0;
  int block_id = 0;
  in >> num_pages_total;
  std::vector<std::string> data;
  data.resize(num_pages);
  blocks.reserve(num_pages_total / num_pages);
  for (int i = 0; i < num_pages_total; i += num_pages) {
    for (int k = 0; k < num_pages; ++ k) {
      in >> data[k];
    }
    blocks.emplace_back(block_id ++, page_size_in_bytes, num_pages);
    decode_hex(data, page_size_in_bytes, num_pages, *(blocks.rbegin()));
  }
  in.close();
}

int64_t get_file_size(std::string workload_path) {
  std::fstream in(workload_path, std::ios::in | std::ios::binary);
  in.seekg(0, in.end);
  int64_t file_size = in.tellg();
  in.close();
  return file_size;
}

int64_t read_binary(std::string workload_path, int page_size_in_bytes, 
                    int num_pages, std::vector<Block>& blocks, 
                    int64_t max_file_size=100000000) {
  int64_t actual_file_size = get_file_size(workload_path);
  ASSERT_WITH_MSG(actual_file_size > 0, "Error file size [" << actual_file_size << "]")
  // std::cout << "File size: " << actual_file_size << " Bytes" << std::endl;
  std::fstream in(workload_path, std::ios::in | std::ios::binary);
  int block_id = 0;
  int64_t file_size = std::min(actual_file_size, max_file_size);
  uint64_t num_blocks = file_size / (page_size_in_bytes * num_pages);
  blocks.reserve(num_blocks);
  for (int i = 0; i < num_blocks; ++ i) {
    blocks.emplace_back(block_id ++, page_size_in_bytes, num_pages);
    auto cur = blocks.rbegin();
    for (int j = 0; j < num_pages; ++ j) {
      cur->significant_bits[j].resize(page_size_in_bytes / sizeof(BIT_TYPE));
      in.read((char*)(cur->significant_bits[j].data()), page_size_in_bytes);
    }
  }
  in.close();
  return file_size;
}

std::string binarize(int value, int num_bits) {
  std::string res = "";
  for (int i = 0; i < num_bits; ++ i) {
    res += ('0' + (value & 1));
    value >>= 1;
  }
  for (int i = 0; i < res.size() / 2; ++ i) {
    std::swap(res[i], res[res.size() - i - 1]);
  }
  return res;
}

void load_config(std::string config_path, std::vector<uint32_t>& page_config) {
  std::fstream in(config_path, std::ios::in);
  if (!in.is_open()) {
    COUT_ERR("File [" << config_path << "] is not open");
  }
  while (in.good()) {
    std::string line = "";
    std::getline(in, line);
    if (line == "") {
      continue;
    }
    std::vector<std::string> items = split(line, ',');
    uint32_t p = 0;
    for (auto item : items) {
      p += (ston<std::string, int>(item) != -1);
    }
    page_config.push_back(p);
  }
  in.close();
}

#endif