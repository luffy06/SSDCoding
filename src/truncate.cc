#include "common.h"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("source_path", po::value<std::string>(), 
     "the path of source workload")
    ("target_path", po::value<std::string>(), 
     "the path of truncated workload")
    ("file_size", po::value<uint32_t>(), 
     "the truncated file size")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (...) {
    COUT_ERR("Unrecognized parameters, please use --help");
  }
  po::notify(vm);

  check_options(vm, {"source_path", "target_path", "file_size"});
  std::string source_path = vm["source_path"].as<std::string>();
  std::string target_path = vm["target_path"].as<std::string>();
  uint32_t file_size = vm["file_size"].as<uint32_t>();
  std::fstream in(source_path, std::ios::in);
  std::fstream out(target_path, std::ios::out | std::ios::binary);
  if (in && out) {
    in.seekg(0, in.end);
    uint32_t actual_file_size = in.tellg();
    in.seekg(0, in.beg);
    COUT_INFO("Actual file size is about " 
              << (actual_file_size / (1024 * 1024)) << " MB")
    file_size = std::min(file_size, actual_file_size);
    const uint32_t N = 100;
    char buffer[N];
    for (; file_size > N; file_size -= N) {
      in.read(buffer, sizeof(char) * N);
      out.write(buffer, sizeof(char) * N);
    }
    if (file_size > 0) {
      in.read(buffer, sizeof(char) * file_size);
      out.write(buffer, sizeof(char) * file_size);
    }
  }
  out.close();
  in.close();
  return 0;
}