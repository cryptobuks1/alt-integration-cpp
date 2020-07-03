// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_CURSOR_INMEM_HPP
#define VERIBLOCK_POP_CPP_CURSOR_INMEM_HPP

#include <veriblock/storage/cursor.hpp>

namespace altintegration {

template <typename Key, typename Value>
struct CursorInmem : public Cursor<Key, Value> {
  using key_t = Key;
  using value_t = Value;

  using umap = std::unordered_map<key_t, std::shared_ptr<value_t>>;
  using pair = std::pair<key_t, std::shared_ptr<value_t>>;

  CursorInmem(const umap& map) {
    for (const pair& m : map) {
      _etl.push_back(m);
    }
  }
  ~CursorInmem() override = default;
  void seekToFirst() override {
    if (_etl.empty()) {
      _it = _etl.cend();
    } else {
      _it = _etl.cbegin();
    }
  }
  void seek(const key_t& key) override {
    _it = std::find_if(_etl.cbegin(), _etl.cend(), [&key](const pair& p) {
      return p.first == key;
    });
  }
  void seekToLast() override { _it = --_etl.cend(); }
  bool isValid() const override {
    bool a = _it != _etl.cend();
    bool b = _it >= _etl.cbegin();
    bool c = _it < _etl.cend();
    return a && b && c;
  }
  void next() override {
    if (_it < _etl.cend()) {
      ++_it;
    }
  }
  void prev() override {
    if (_it == _etl.cbegin()) {
      _it = _etl.cend();
    } else {
      --_it;
    }
  }
  key_t key() const override {
    if (!isValid()) {
      throw std::out_of_range("invalid cursor");
    }

    return _it->first;
  }
  value_t value() const override {
    if (!isValid()) {
      throw std::out_of_range("invalid cursor");
    }

    return *_it->second;
  }

 private:
  std::vector<pair> _etl;
  typename std::vector<pair>::const_iterator _it;
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_CURSOR_INMEM_HPP
