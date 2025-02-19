#include "common.h"

namespace po = boost::program_options;

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
    ("num_printed_blocks", po::value<int>(), 
     "the number of printed blocks")
    ("file_size", po::value<uint64_t>(), 
     "the file size")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (...) {
    COUT_ERR("Unrecognized parameters, please use --help");
  }
  po::notify(vm);

  check_options(vm, {"workload_path", "workload_type", "num_pages", 
                     "page_size"});

  std::string workload_path = vm["workload_path"].as<std::string>();
  std::string workload_type = vm["workload_type"].as<std::string>();
  int num_pages = vm["num_pages"].as<int>();
  int page_size_in_bytes = vm["page_size"].as<int>() * 1024;
  int64_t file_size = 100000000;
  if (vm.count("file_size")) {
    file_size = vm["file_size"].as<int64_t>();
  }
  int num_printed_blocks = 1;
  if (vm.count("num_printed_blocks")) {
    num_printed_blocks = vm["num_printed_blocks"].as<int>();
  }

  std::vector<Block> blocks;
  if (workload_type == "hex") {
    read_hex(workload_path, page_size_in_bytes, num_pages, blocks);
  } else if (workload_type == "binary") {
    file_size = read_binary(workload_path, page_size_in_bytes, 
                            num_pages, blocks, file_size);
  } else {
    std::cout << "Unsupport workload type [" << workload_type 
              << "]" << std::endl;
    exit(-1);
  }

  COUT_INFO("# blocks " << blocks.size() << ", # pages " << num_pages)
  for (uint32_t i = 0, t = 0; i < blocks.size() && i < num_printed_blocks; ++ i) {
    Block& block = blocks[i];
    for (uint32_t k = 0; k < num_pages; ++ k) {
      for (uint32_t j = 0; j < block.significant_bits[k].size(); ++ j) {
        COUT_INFO(binarize(block.significant_bits[k][j], BIT_SIZE))
      }
    }
  }
  return 0;
}