// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_REPOSITORY_INMEM_HPP
#define VERIBLOCK_POP_CPP_REPOSITORY_INMEM_HPP

#include <veriblock/storage/base_repository.hpp>

#include "batch_inmem.hpp"
#include "cursor_inmem.hpp"

namespace altintegration {

struct RepositoryInmem : public BaseRepository {
  using key_t = BaseRepository::key_t;
  using value_t = BaseRepository::value_t;
  using base_cursor_t = typename BaseRepository::cursor_t;
  using cursor_t = CursorInmem<key_t, value_t>;
  using base_batch_t = typename BaseRepository::batch_t;
  using batch_t = WriteBatchInmem<key_t, value_t>;

  ~RepositoryInmem() override = default;

  bool get(const key_t& key, value_t* out) const override {
    auto it = _storage.find(key);
    if (it == _storage.end()) {
      return false;
    }

    if (out) {
      *out = *it->second;
    }

    return true;
  }

  size_t getMany(Slice<const key_t> keys,
                 std::vector<value_t>* out) const override {
    size_t totalFound = 0;
    for (const auto& h : keys) {
      value_t block;
      if (get(h, &block)) {
        ++totalFound;
        out->push_back(block);
      }
    }

    return totalFound;
  }

  bool put(const key_t& key, const value_t& value) override {
    bool res = _storage.find(key) != _storage.end();
    _storage[key] = std::make_shared<value_t>(value);
    return res;
  }

  bool remove(const key_t& key) override { return _storage.erase(key) == 1; }

  void clear() override { _storage.clear(); }

  std::unique_ptr<base_batch_t> newBatch() override {
    return std::unique_ptr<base_batch_t>(new batch_t(this));
  }

  std::shared_ptr<base_cursor_t> newCursor() const override {
    return std::make_shared<cursor_t>(_storage);
  }

 private:
  std::unordered_map<key_t, std::shared_ptr<value_t>> _storage;
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_REPOSITORY_INMEM_HPP
