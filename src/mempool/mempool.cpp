// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "veriblock/mempool/mempool.hpp"

#include "veriblock/stateless_validation.hpp"

namespace altintegration {

bool MemPool::add(const VbkBlock& block, ValidationState& state) {
  block_index_[block.getShortHash()] = block;

  //FIXME: stateful Validation
  std::ignore = state;

  return true;
}

bool MemPool::add(const std::vector<VbkBlock>& blocks, ValidationState& state) {
  for (const auto& block : blocks) {
    add(block, state);
  }

  return true;
}

bool MemPool::add(const ATV& atv, ValidationState& state) {
  if (!checkATV(atv, state, alt_chain_params_, vbk_chain_params_)) {
    return state.Invalid("mempool-add-atv");
  }

  if (!add(atv.containingBlock, state)) {
    return state.Invalid("mempool-add-atv-containing-block");
  }

  if (!add(atv.context, state)) {
    return state.Invalid("mempool-add-atv-context");
  }

  auto pair = std::make_pair(atv.getId(), atv);
  // clear context
  pair.second.context.clear();

  stored_atvs_.insert(pair);

  return true;
}

bool MemPool::add(const std::vector<ATV>& atvs, ValidationState& state) {
  for (size_t i = 0; i < atvs.size(); ++i) {
    if (!add(atvs[i], state)) {
      return state.Invalid("mempool-add-atvs", i);
    }
  }

  return true;
}

bool MemPool::add(const VTB& vtb, ValidationState& state) {
  if (!checkVTB(vtb, state, vbk_chain_params_, btc_chain_params_)) {
    return state.Invalid("mempool-add-vtb");
  }

  if (!add(vtb.containingBlock, state)) {
      return state.Invalid("mempool-add-vtb-containing-block");
  };

  if (!add(vtb.context, state)) {
    return state.Invalid("mempool-add-vtb-context");
  };

  auto pair = std::make_pair(BtcEndorsement::getId(vtb), vtb);
  // clear context
  pair.second.context.clear();

  stored_vtbs_.insert(pair);

  return true;
}

bool MemPool::add(const std::vector<VTB>& vtbs, ValidationState& state) {
  for (size_t i = 0; i < vtbs.size(); ++i) {
    if (!add(vtbs[i], state)) {
      return state.Invalid("mempool-add-vtbs", i);
    }
  }

  return true;
}

void MemPool::removePayloads(const std::vector<PopData>& PopDatas) {
  for (const auto& tx : PopDatas) {
    // clear context
    for (const auto& b : tx.vbk_context) {
      block_index_.erase(b.getShortHash());
    }

    // clear atv
    if (tx.hasAtv) {
      stored_atvs_.erase(tx.atv.getId());
    }

    // clear vtbs
    for (const auto& vtb : tx.vtbs) {
      stored_vtbs_.erase(BtcEndorsement::getId(vtb));
    }
  }
}

}  // namespace altintegration
