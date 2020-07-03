// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_BATCH_INMEM_HPP
#define VERIBLOCK_POP_CPP_BATCH_INMEM_HPP

#include <veriblock/storage/base_repository.hpp>
#include <veriblock/storage/serializers.hpp>
#include <veriblock/storage/write_batch.hpp>

namespace altintegration {

template <typename Key, typename Value>
struct WriteBatchInmem : public WriteBatch<Key, Value> {
  using key_t = Key;
  using value_t = Value;

  enum class Operation { PUT, REMOVE };

  WriteBatchInmem(BaseRepository* repo) : _repo(repo) {}

  void put(const key_t& key, const value_t& value) override {
    _ops.push_back(Operation::PUT);
    _puts.push_back({key, value});
  };

  void remove(const key_t& key) override {
    _ops.push_back(Operation::REMOVE);
    _removes.push_back(key);
  };

  void clear() override {
    _ops.clear();
    _removes.clear();
    _puts.clear();
  };

  void commit() override {
    auto puts_begin = this->_puts.begin();
    auto removes_begin = this->_removes.begin();
    for (const auto& op : this->_ops) {
      switch (op) {
        case WriteBatchInmem<key_t, value_t>::Operation::PUT: {
          auto& pair = *puts_begin;
          auto k = db::Serialize(pair.first);
          auto v = db::Serialize(pair.second);
          _repo->put(k, v);
          ++puts_begin;
          break;
        }
        case WriteBatchInmem<key_t, value_t>::Operation::REMOVE: {
          auto& key = *removes_begin;
          auto k = db::Serialize(key);
          _repo->remove(k);
          ++removes_begin;
          break;
        }
        default:
          VBK_ASSERT(false && "should never happen");
      }
    }
    clear();
  }

 private:
  BaseRepository* _repo;
  std::vector<std::pair<key_t, value_t>> _puts;
  std::vector<key_t> _removes;
  std::vector<Operation> _ops;
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_BATCH_INMEM_HPP
