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

  auto &get_storage() {
    return std::visit([](auto &storage) -> auto & { return storage; }, _data);
  }

  const auto &get_storage() const {
    return std::visit(
        [](const auto &storage) -> const auto & { return storage; }, _data);
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

  template <typename Storage, typename U>
  void push_to_storage(Storage &storage, U &&v) {
    if (_count < sso_threshold)
      storage[_count++] = std::forward<U>(v);
    else {
      std::vector<T> vec(storage.begin(), storage.end());
      vec.push_back(std::forward<U>(v));
      _data = std::move(vec);
      _count = std::get<std::vector<T>>(_data).size();
    }
  }

  template <typename U> void push_back(U &&v) {
    if (auto *arr = std::get_if<std::array<T, sso_threshold>>(&_data))
      push_to_storage(*arr, std::forward<U>(v));
    else {
      auto &vec = std::get<std::vector<T>>(_data);
      vec.push_back(std::forward<U>(v));
      _count = vec.size();
    }
  }

  size_t size() const { return _count; }

  T &operator[](size_t i) {
    return std::visit([i](auto &storage) -> T & { return storage[i]; }, _data);
  }

  const T &operator[](size_t i) const {
    return std::visit(
        [i](const auto &storage) -> const T & { return storage[i]; }, _data);
  }

  operator std::span<T>() & {
    return std::visit(
        [this](auto &storage) -> std::span<T> {
          using StorageType = std::decay_t<decltype(storage)>;
          if constexpr (std::is_same_v<StorageType,
                                       std::array<T, sso_threshold>>)
            return std::span<T>(storage.data(), _count);
          else
            return std::span<T>(storage.data(), storage.size());
        },
        _data);
  }

  operator std::span<const T>() const & {
    return std::visit(
        [this](const auto &storage) -> std::span<const T> {
          using StorageType = std::decay_t<decltype(storage)>;
          if constexpr (std::is_same_v<StorageType,
                                       std::array<T, sso_threshold>>)
            return std::span<const T>(storage.data(), _count);
          else
            return std::span<const T>(storage.data(), storage.size());
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

  template <size_t I> auto &get_storage() { return std::get<I>(_data); }

  template <size_t I> const auto &get_storage() const {
    return std::get<I>(_data);
  }

  template <size_t I> void push_to_storage(const field_type<I> &value) {
    auto &storage = get_storage<I>();

    if (std::holds_alternative<std::array<field_type<I>, sso_threshold>>(
            storage)) {
      auto &array_storage =
          std::get<std::array<field_type<I>, sso_threshold>>(storage);
      if (_count < sso_threshold)
        array_storage[_count] = value;
      else {
        std::vector<field_type<I>> vec(array_storage.begin(),
                                       array_storage.end());
        vec.push_back(value);
        storage = std::move(vec);
      }
    } else {
      auto &vector_storage = std::get<std::vector<field_type<I>>>(storage);
      vector_storage.push_back(value);
    }
  }

public:
  smart_seq() {
    [this]<size_t... I>(std::index_sequence<I...>) {
      (([this]() {
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

  void push_back(const T &obj) {
    [this, &obj]<size_t... I>(std::index_sequence<I...>) {
      (([this, &obj] { push_to_storage<I>(boost::pfr::get<I>(obj)); }()), ...);
    }(std::make_index_sequence<_n>{});
    _count++;
  }

  size_t size() const { return _count; }

  T operator[](size_t index) const {
    T result;
    [this, index, &result]<size_t... I>(std::index_sequence<I...>) {
      (([this, index, &result] {
         const auto &storage = get_storage<I>();
         std::visit(
             [index, &result](const auto &stor) {
               boost::pfr::get<I>(result) = stor[index];
             },
             storage);
       }()),
       ...);
    }(std::make_index_sequence<_n>{});
    return result;
  }

  template <size_t I> auto &field() { return get_storage<I>(); }

  template <size_t I> const auto &field() const { return get_storage<I>(); }
};

template <typename T, typename... Args> auto make_smart_seq(Args &&...args) {
  return smart_seq<T>();
}

} // namespace machinetherapist