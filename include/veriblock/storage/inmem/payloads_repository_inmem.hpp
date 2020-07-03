// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_INMEM_PAYLOADS_REPOSITORY_INMEM_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_INMEM_PAYLOADS_REPOSITORY_INMEM_HPP_

#include <string>
#include <unordered_map>
#include <veriblock/storage/payloads_repository.hpp>

namespace altintegration {

template <typename Payloads>
struct PayloadsRepositoryInmem : public PayloadsRepository<Payloads> {
  using payloads_t = Payloads;
  using eid_t = typename Payloads::id_t;
  using cursor_t = Cursor<eid_t, Payloads>;

  ~PayloadsRepositoryInmem() override = default;

  bool remove(const eid_t& id) override { return _p.erase(id) == 1; }

  bool put(const payloads_t& payload) override {
    auto id = payload.getId();
    _p[id] = payload;
    return true;
  }

  bool get(const eid_t& id, payloads_t* payload) const override {
    auto it = _p.find(id);
    if (it == _p.end()) {
      return false;
    }
    if (payload != nullptr) {
      *payload = _p.at(id);
    }
    return true;
  }

  void clear() override { _p.clear(); }

  std::unique_ptr<PayloadsWriteBatch<Payloads>> newBatch() override {
    return std::unique_ptr<PayloadsWriteBatchInmem<Payloads>>(
        new PayloadsWriteBatchInmem<Payloads>(this));
  }

  std::shared_ptr<cursor_t> newCursor() const override {
    return std::make_shared<PayloadsCursorInmem<Payloads>>(_p);
  }

 private:
  // [endorsement id] => payload
  std::unordered_map<eid_t, payloads_t> _p;
};

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_INMEM_PAYLOADS_REPOSITORY_INMEM_HPP_
