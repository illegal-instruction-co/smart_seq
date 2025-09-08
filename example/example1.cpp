#include <machinetherapist/smart_seq.h>

#include <chrono>
#include <iostream>

using namespace std;

using namespace machinetherapist;

// ====================== Example Types ======================
struct Vec3 {
  float x, y, z;

  float &operator[](size_t index) {
    if (index == 0)
      return x;
    if (index == 1)
      return y;
    if (index == 2)
      return z;
    throw out_of_range("Vec3 index out of range");
  }

  const float &operator[](size_t index) const {
    if (index == 0)
      return x;
    if (index == 1)
      return y;
    if (index == 2)
      return z;
    throw out_of_range("Vec3 index out of range");
  }
};

struct Particle {
  Vec3 pos;
  int id;
};

// ====================== Primitive Type Full Test ======================
void primitive_test() {
  cout << "\n=== Primitive Type Full Test (int with SSO) ===\n";

  smart_seq<int> ints;
  cout << "Initially empty? " << boolalpha << ints.empty() << "\n";

  // Push elements: SSO triggers for first 8
  for (int i = 0; i < 10; i++) {
    ints.push_back(i);
    cout << "Pushed: " << i << ", size: " << ints.size() << "\n";
  }

  // Operator[] access
  cout << "Access via operator[]: ";
  for (size_t i = 0; i < ints.size(); i++)
    cout << ints[i] << " ";
  cout << "\n";

  // At() access with bounds check
  try {
    cout << "ints.at(3) = " << ints.at(3) << "\n";
    cout << "ints.at(20) = " << ints.at(20) << "\n"; // should throw
  } catch (const exception &e) {
    cout << "Caught exception: " << e.what() << "\n";
  }

  // Pop elements
  for (size_t i = 0; i < 12; i++) {
    ints.pop_back();
    cout << "Pop_back called, size: " << ints.size() << "\n";
  }

  cout << "Empty after pop? " << ints.empty() << "\n";

  // Span conversion
  ints.push_back(42);
  ints.push_back(99);
  span<int> s = static_cast<span<int>>(ints);
  cout << "Span iteration: ";
  for (auto &v : s)
    cout << v << " ";
  cout << "\n";
}

// ====================== Struct Type Full Test ======================
void struct_test() {
  cout << "\n=== Struct Type Full Test (Particle, SoA) ===\n";

  smart_seq<Particle> particles;

  // Push_back and emplace_back
  particles.push_back(Particle{Vec3{1.f, 2.f, 3.f}, 10});
  particles.emplace_back(Vec3{4.f, 5.f, 6.f}, 20);
  cout << "Size after push/emplace: " << particles.size() << "\n";

  // Access fields via SoA
  auto pos_storage = particles.field<0>();
  auto id_storage = particles.field<1>();
  cout << "Field<0> (pos.x) iteration: ";
  for (auto &v : pos_storage)
    cout << v.x << " ";
  cout << "\n";

  cout << "Field<1> (id) iteration: ";
  for (auto &v : id_storage)
    cout << v << " ";
  cout << "\n";

  // Proxy access
  for (size_t i = 0; i < particles.size(); i++) {
    auto p = particles.get_ref(i);
    cout << "Proxy Particle " << i << ": pos(" << p.pos().x << ", " << p.pos().y
         << ", " << p.pos().z << "), id(" << p.id() << ")\n";

    // Modify via proxy
    p.id() += 100;
    p.pos().x += 10;
  }

  cout << "After Proxy modification:\n";
  for (size_t i = 0; i < particles.size(); i++) {
    auto p = particles.get_ref(i);
    cout << "Particle " << i << ": pos(" << p.pos().x << ", " << p.pos().y
         << ", " << p.pos().z << "), id(" << p.id() << ")\n";
  }

  // Pop_back
  particles.pop_back();
  particles.pop_back(); // should be empty now
  cout << "Empty after pop_back calls? " << boolalpha << particles.empty()
       << "\n";
}

// ====================== Performance Test ======================
void performance_test() {
  cout << "\n=== Performance Test ===\n";
  const int N = 1000000;
  smart_seq<int> numbers;

  auto start = chrono::high_resolution_clock::now();
  for (int i = 0; i < N; i++)
    numbers.push_back(i);
  auto end = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
  cout << "Added " << N << " ints in " << duration.count() << " ms\n";

  // Summation via span
  start = chrono::high_resolution_clock::now();
  long long sum = 0;
  for (auto &v : static_cast<span<int>>(numbers))
    sum += v;
  end = chrono::high_resolution_clock::now();
  duration = chrono::duration_cast<chrono::milliseconds>(end - start);
  cout << "Sum: " << sum << " calculated in " << duration.count() << " ms\n";
}

// ====================== Main ======================
int main() {
  try {
    primitive_test();
    struct_test();
    performance_test();

    cout << "\n=== All tests completed successfully ===\n";
    return 0;
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
