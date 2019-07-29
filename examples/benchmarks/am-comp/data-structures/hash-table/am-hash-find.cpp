#include <bcl/bcl.hpp>
#include <queue>

std::unordered_map<int, int> map;

int main(int argc, char** argv) {
  BCL::init();

  BCL::gas::init_am();
  BCL::gas::init_2wayam();

  auto insert = BCL::gas::register_am([](int key, int value) -> void {
    map[key] = value;
  }, int(), int());

  auto find = BCL::gas::register_2wayam([](int key) -> int {
    return map[key];
  }, int());

  size_t num_ams = 100000;
  size_t key_space = 1000000;
  size_t keys_per_n = (key_space + BCL::nprocs() - 1) / BCL::nprocs();

  srand48(BCL::rank());

  std::vector<int> random_numbers;

  for (size_t i = 0; i < num_ams; i++) {
    size_t value = lrand48() % key_space;
    random_numbers.push_back(value);
  }

  BCL::barrier();

  for (size_t i = 0; i < num_ams; i++) {
    size_t value = random_numbers[i];
    size_t remote_proc = value / keys_per_n;

    insert.launch(remote_proc, value, value);
    BCL::gas::flush_am();
  }
  BCL::barrier();

  auto begin = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_ams; i++) {
    size_t value = random_numbers[i];
    size_t remote_proc = value / keys_per_n;

    auto fut = find.launch(remote_proc, value);
    BCL::gas::flush_am();
  }

  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();
  double duration = std::chrono::duration<double>(end - begin).count();

  double duration_us = 1e6*duration;
  double latency_us = duration_us / num_ams;

  BCL::print("Latency is %lf us per AM. (Finished in %lf s)\n", latency_us, duration);

  BCL::finalize();
  return 0;
}
