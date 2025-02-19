#include "common.h"
#include "linear_model.h"

template<typename T>
void ReadBinary(std::string workload_path, std::vector<T>& keys) {
  std::fstream in(workload_path, std::ios::in | std::ios::binary);
  uint64_t num_keys;
  in.read((char*)&num_keys, sizeof(T));

  keys.resize(num_keys);
  in.read((char*)keys.data(), num_keys * sizeof(T));
  in.close();
}

template<typename T>
void WriteBinary(std::string workload_path, const std::vector<T>& keys) {
  std::fstream in(workload_path, std::ios::out | std::ios::binary);
  uint64_t num_keys = keys.size();
  in.write((char*)&num_keys, sizeof(T));
  in.write((char*)keys.data(), num_keys * sizeof(T));
  in.close();
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "No enough parameters" << std::endl;
    std::cout << "Please input: format (workload path) "
              << "(key type) (output path) " << std::endl;
    exit(-1);
  }
  std::string workload_path = argv[1];
  std::string key_type = argv[2];
  std::string output_path = argv[3];
  if (key_type == "uint64") {
    std::vector<uint64_t> keys;
    ReadBinary<uint64_t>(workload_path, keys);
    uint64_t max_key = keys[0];
    uint64_t min_key = keys[0];
    for (int i = 0; i < keys.size(); ++ i) {
      max_key = std::max(max_key, keys[i]);
      min_key = std::min(min_key, keys[i]);
    }
    std::cout << "Range [" << min_key << ", " << max_key << "]" << std::endl;
    int num_bits = 0;
    while (max_key > 0) {
      max_key >>= 1;
      num_bits ++;
    }
    std::cout << "Required bits " << num_bits << std::endl;
    // if (num_bits < 64) {
    //   WriteBinary<uint64_t>(output_path, keys);
    // }
  } else {
    std::cout << "Unsupport key type [" << key_type 
              << "]" << std::endl;
    exit(-1);
  }
  return 0;
}