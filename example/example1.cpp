#include <machinetherapist/smart_seq.h>

#include <chrono>
#include <iostream>

using namespace std;

using namespace machinetherapist;

struct Vec3 {
  float x, y, z;

  float &operator[](size_t index) {
    if (index == 0)
      return x;
    if (index == 1)
      return y;
    if (index == 2)
      return z;
    throw std::out_of_range("Vec3 index out of range");
  }

  const float &operator[](size_t index) const {
    if (index == 0)
      return x;
    if (index == 1)
      return y;
    if (index == 2)
      return z;
    throw std::out_of_range("Vec3 index out of range");
  }
};

struct Particle {
  Vec3 pos;
  int id;
};

void basic_test() {
  cout << "=== Basic Test ===" << endl;

  smart_seq<int> ints;
  for (int i = 0; i < 10; i++) {
    ints.push_back(i);
  }

  cout << "Ints size: " << ints.size() << endl;
  for (size_t i = 0; i < ints.size(); i++) {
    cout << ints[i] << " ";
  }
  cout << endl;

  auto ints_span = static_cast<std::span<int>>(ints);
  cout << "Span size: " << ints_span.size() << endl;
}

void soa_test() {
  cout << "\n=== SoA Test ===" << endl;

  smart_seq<Particle> particles;
  const int count = 5;

  for (int i = 0; i < count; i++)
    particles.push_back(Particle{Vec3{1.0f * i, 2.0f * i, 3.0f * i}, i});

  cout << "Particles size: " << particles.size() << endl;

  float sum = 0;

  auto pos_storage = particles.field<0>();
  auto id_storage = particles.field<1>();

  for (size_t i = 0; i < particles.size(); i++) {
    sum += pos_storage[i].x + pos_storage[i].y + pos_storage[i].z;
    cout << "Particle " << i << ": pos(" << pos_storage[i].x << ", "
         << pos_storage[i].y << ", " << pos_storage[i].z << ")" << endl;
  }

  for (size_t i = 0; i < particles.size(); i++) {
    cout << "Particle " << i << ": id(" << id_storage[i] << ")" << endl;
  }

  cout << "Smart_seq SoA sum: " << sum << " (count: " << particles.size() << ")"
       << endl;

  cout << "\nOperator[] test:" << endl;
  for (size_t i = 0; i < particles.size(); i++) {
    auto p = particles.get_ref(i);
    cout << "Particle " << i << ": pos(" << p.pos().x << ", " << p.pos().y
         << ", " << p.pos().z << "), id(" << p.id() << ")" << endl;
  }
}

void performance_test() {
  cout << "\n=== Performance Test ===" << endl;

  const int large_count = 1000;
  smart_seq<int> numbers;

  auto start = chrono::high_resolution_clock::now();

  for (int i = 0; i < large_count; i++) {
    numbers.push_back(i);
  }

  auto end = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

  cout << "Added " << large_count << " elements in " << duration.count()
       << " microseconds" << endl;
  cout << "Final size: " << numbers.size() << endl;

  start = chrono::high_resolution_clock::now();
  long long sum = 0;
  for (size_t i = 0; i < numbers.size(); i++) {
    sum += numbers[i];
  }
  end = chrono::high_resolution_clock::now();
  duration = chrono::duration_cast<chrono::microseconds>(end - start);

  cout << "Sum: " << sum << " calculated in " << duration.count()
       << " microseconds" << endl;
}

int main() {
  try {
    basic_test();
    soa_test();
    performance_test();

    cout << "\n=== All tests completed successfully ===" << endl;
    return 0;

  } catch (const exception &e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
}