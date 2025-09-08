#pragma once

#include <boost/pfr.hpp>

#include <array>
#include <span>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace machinetherapist {

constexpr size_t sso_threshold = 8;

template <typename T, typename Enable = void> class smart_seq;

template <typename T> struct storage_type {
  using type = std::conditional_t<
      std::is_class_v<T>, std::vector<T>,
      std::conditional_t<(sizeof(T) <= sizeof(void *) * sso_threshold),
                         std::array<T, sso_threshold>, std::vector<T>>>;
};

template <typename T> using storage_type_t = typename storage_type<T>::type;

template <typename T>
using storage_variant =
    std::variant<std::array<T, sso_threshold>, std::vector<T>>;

template <typename T>
class smart_seq<T, std::enable_if_t<!std::is_class_v<T>>> {
  storage_variant<T> _data;
  size_t _count = 0;

  auto &get_storage() noexcept {
    return std::visit([](auto &s) -> auto & { return s; }, _data);
  }
  auto const &get_storage() const noexcept {
    return std::visit([](auto const &s) -> auto const & { return s; }, _data);
  }

  template <typename Storage, typename U>
  void push_to_storage(Storage &storage, U &&v) {
    if (_count < sso_threshold)
      storage[_count++] = std::forward<U>(v);
    else {
      std::vector<T> vec;
      vec.reserve(_count + 1);
      vec.insert(vec.end(), storage.begin(), storage.end());
      vec.push_back(std::forward<U>(v));
      _data = std::move(vec);
      _count++;
    }
  }

  template <typename Storage, typename... Args>
  void emplace_to_storage(Storage &storage, Args &&...args) {
    if (_count < sso_threshold)
      storage[_count++] = T(std::forward<Args>(args)...);
    else {
      std::vector<T> vec;
      vec.reserve(_count + 1);
      vec.insert(vec.end(), storage.begin(), storage.end());
      vec.emplace_back(std::forward<Args>(args)...);
      _data = std::move(vec);
      _count++;
    }
  }

public:
  smart_seq() {
    if constexpr (std::is_same_v<storage_type_t<T>,
                                 std::array<T, sso_threshold>>)
      _data = std::array<T, sso_threshold>{};
    else {
      _data = std::vector<T>{};
      std::get<std::vector<T>>(_data).reserve(sso_threshold);
    }
  }

  smart_seq(const smart_seq &) = default;
  smart_seq(smart_seq &&) noexcept = default;
  smart_seq &operator=(const smart_seq &) = default;
  smart_seq &operator=(smart_seq &&) noexcept = default;

  template <typename U> void push_back(U &&v) {
    if (auto *arr = std::get_if<std::array<T, sso_threshold>>(&_data))
      push_to_storage(*arr, std::forward<U>(v));
    else {
      auto &vec = std::get<std::vector<T>>(_data);
      vec.push_back(std::forward<U>(v));
      _count = vec.size();
    }
  }

  template <typename... Args> void emplace_back(Args &&...args) {
    if (auto *arr = std::get_if<std::array<T, sso_threshold>>(&_data))
      emplace_to_storage(*arr, std::forward<Args>(args)...);
    else {
      auto &vec = std::get<std::vector<T>>(_data);
      vec.emplace_back(std::forward<Args>(args)...);
      _count = vec.size();
    }
  }

  [[nodiscard]] auto size() const noexcept -> size_t { return _count; }

  auto operator[](size_t i) noexcept -> T & {
    return std::visit([i](auto &s) -> T & { return s[i]; }, _data);
  }
  auto operator[](size_t i) const noexcept -> T const & {
    return std::visit([i](auto const &s) -> T const & { return s[i]; }, _data);
  }

  [[nodiscard]] auto at(size_t i) -> T & {
    if (i >= _count)
      throw std::out_of_range("smart_seq index out of range");
    return (*this)[i];
  }
  [[nodiscard]] auto at(size_t i) const -> T const & {
    if (i >= _count)
      throw std::out_of_range("smart_seq index out of range");
    return (*this)[i];
  }

  operator std::span<T>() & noexcept {
    return std::visit(
        [this](auto &s) -> std::span<T> {
          using StorageType = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<StorageType,
                                       std::array<T, sso_threshold>>)
            return std::span<T>(s.data(), _count);
          else
            return std::span<T>(s.data(), s.size());
        },
        _data);
  }

  operator std::span<T const>() const & noexcept {
    return std::visit(
        [this](auto const &s) -> std::span<T const> {
          using StorageType = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<StorageType,
                                       std::array<T, sso_threshold>>)
            return std::span<T const>(s.data(), _count);
          else
            return std::span<T const>(s.data(), s.size());
        },
        _data);
  }
};

template <typename T> class smart_seq<T, std::enable_if_t<std::is_class_v<T>>> {
  static constexpr size_t _n = boost::pfr::tuple_size_v<T>;

  template <size_t I>
  using field_type =
      std::decay_t<decltype(boost::pfr::get<I>(std::declval<T &>()))>;

  template <size_t I>
  using storage_variant_for_field = storage_variant<field_type<I>>;

  using data_tuple_t = decltype([]<size_t... I>(std::index_sequence<I...>) {
    return std::tuple<storage_variant_for_field<I>...>{};
  }(std::make_index_sequence<_n>{}));

  data_tuple_t _data;
  size_t _count = 0;

  template <size_t I> auto &get_storage() noexcept {
    return std::get<I>(_data);
  }

  template <size_t I> auto const &get_storage() const noexcept {
    return std::get<I>(_data);
  }

  template <size_t I, typename U> void push_to_storage(U &&value) {
    auto &storage = get_storage<I>();
    if (std::holds_alternative<std::array<field_type<I>, sso_threshold>>(
            storage)) {
      auto &array_storage =
          std::get<std::array<field_type<I>, sso_threshold>>(storage);
      if (_count < sso_threshold)
        array_storage[_count] = std::forward<U>(value);
      else {
        std::vector<field_type<I>> vec;
        vec.reserve(_count + 1);
        vec.insert(vec.end(), array_storage.begin(), array_storage.end());
        vec.push_back(std::forward<U>(value));
        storage = std::move(vec);
      }
    } else {
      auto &vector_storage = std::get<std::vector<field_type<I>>>(storage);
      vector_storage.push_back(std::forward<U>(value));
    }
  }

public:
  smart_seq() {
    [this]<size_t... I>(std::index_sequence<I...>) {
      (([this] {
         using FieldType = field_type<I>;
         if constexpr (std::is_class_v<FieldType> ||
                       !(sizeof(FieldType) <= sizeof(void *) * sso_threshold)) {
           std::get<I>(_data) = std::vector<FieldType>{};
           if constexpr (!std::is_class_v<FieldType>)
             std::get<std::vector<FieldType>>(std::get<I>(_data))
                 .reserve(sso_threshold);
         } else
           std::get<I>(_data) = std::array<FieldType, sso_threshold>{};
       }()),
       ...);
    }(std::make_index_sequence<_n>{});
  }

  smart_seq(const smart_seq &) = default;
  smart_seq(smart_seq &&) noexcept = default;
  smart_seq &operator=(const smart_seq &) = default;
  smart_seq &operator=(smart_seq &&) noexcept = default;

  void push_back(T const &obj) {
    [this, &obj]<size_t... I>(std::index_sequence<I...>) {
      ((push_to_storage<I>(boost::pfr::get<I>(obj))), ...);
    }(std::make_index_sequence<_n>{});
    _count++;
  }

  template <typename... Args> void emplace_back(Args &&...args) {
    push_back(T(std::forward<Args>(args)...));
  }

  [[nodiscard]] auto size() const noexcept -> size_t { return _count; }

  [[nodiscard]] auto operator[](size_t index) const -> T {
    T result;
    [this, index, &result]<size_t... I>(std::index_sequence<I...>) {
      ((std::visit(
           [index, &result](auto const &stor) {
             boost::pfr::get<I>(result) = stor[index];
           },
           get_storage<I>())),
       ...);
    }(std::make_index_sequence<_n>{});
    return result;
  }

  [[nodiscard]] auto at(size_t index) const -> T {
    if (index >= _count)
      throw std::out_of_range("smart_seq index out of range");
    return (*this)[index];
  }

  template <size_t I> auto &field() noexcept { return get_storage<I>(); }

  template <size_t I> auto const &field() const noexcept {
    return get_storage<I>();
  }
};

template <typename T, typename... Args>
auto make_smart_seq(Args &&...) -> smart_seq<T> {
  return smart_seq<T>();
}

} // namespace machinetherapist
