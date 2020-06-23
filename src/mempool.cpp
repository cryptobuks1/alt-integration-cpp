// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "veriblock/mempool.hpp"

#include <deque>
#include <veriblock/reversed_range.hpp>

#include "veriblock/entities/vbkfullblock.hpp"
#include "veriblock/stateless_validation.hpp"

namespace altintegration {

namespace {

bool checkConnectivityWithBlock(const VbkBlock& check_block,
                                const VbkBlock& current_block) {
  return check_block.previousBlock == current_block.getShortHash();
}

bool checkConnectivityWithTree(const VbkBlock& check_block,
                               const VbkBlockTree& tree) {
  return tree.getBlockIndex(check_block.previousBlock) != nullptr;
}

std::vector<std::pair<ATV::id_t, std::shared_ptr<ATV>>> getSortedATVs(
    const MemPool::atv_map_t& map) {
  std::vector<std::pair<ATV::id_t, std::shared_ptr<ATV>>> sorted_atvs(
      map.begin(), map.end());

  auto atv_comparator =
      [](const std::pair<ATV::id_t, std::shared_ptr<ATV>>& el1,
         const std::pair<ATV::id_t, std::shared_ptr<ATV>>& el2) -> bool {
    return el1.second->containingBlock.height <
           el2.second->containingBlock.height;
  };

  std::sort(sorted_atvs.begin(), sorted_atvs.end(), atv_comparator);
  return sorted_atvs;
}

}  // namespace

bool MemPool::fillContext(VbkBlock first_block,
                          std::vector<VbkBlock>& context,
                          AltTree& tree) {
  while (!checkConnectivityWithTree(first_block, tree.vbk())) {
    auto el = vbkblocks_.find(first_block.previousBlock);
    if (el != vbkblocks_.end()) {
      first_block = *el->second;
      context.push_back(first_block);
    } else {
      return false;
    }
  }

  // reverse context
  std::reverse(context.begin(), context.end());

  return true;
}

void MemPool::fillVTBs(std::vector<VTB>& vtbs,
                       const std::vector<VbkBlock>& vbk_context) {
  for (const auto& b : vbk_context) {
    for (auto& vtb : stored_vtbs_) {
      if (vtb.second->containingBlock == b) {
        vtbs.push_back(*vtb.second);
      }
    }
  }
}

bool MemPool::applyPayloads(const AltBlock& hack_block,
                            PopData& popdata,
                            AltTree& tree,
                            ValidationState& state) {
  bool ret = tree.acceptBlock(hack_block, state);
  VBK_ASSERT(ret);

  // apply vbk_context
  for (const auto& b : popdata.context) {
    ret = tree.vbk().acceptBlock(b, state);
    VBK_ASSERT(ret);
  }

  auto genesis_height = tree.vbk().getParams().getGenesisBlock().height;
  auto settlement_interval =
      tree.vbk().getParams().getEndorsementSettlementInterval();
  // check VTB endorsements
  for (auto it = popdata.vtbs.begin(); it != popdata.vtbs.end();) {
    VTB& vtb = *it;
    auto* containing_block_index =
        tree.vbk().getBlockIndex(vtb.containingBlock.getHash());
    if (!containing_block_index) {
      throw std::logic_error("Mempool: containing block is not found: " +
                             HexStr(vtb.containingBlock.getHash()));
    }

    auto start_height = (std::max)(
        genesis_height, containing_block_index->height - settlement_interval);

    auto endorsement = VbkEndorsement::fromContainer(vtb);
    Chain<BlockIndex<VbkBlock>> chain(start_height, containing_block_index);
    auto duplicate =
        findBlockContainingEndorsement(chain, endorsement, settlement_interval);

    // invalid vtb
    if (duplicate) {
      // remove from storage
      stored_vtbs_.erase(vtb.getId());

      it = popdata.vtbs.erase(it);
      continue;
    }
    ++it;
  }

  for (const auto& b : reverse_iterate(popdata.context)) {
    tree.vbk().removeSubtree(b.getHash());
  }

  //  TODO validate payloads
  //  AltPayloads payloads;
  //  payloads.popData = popdata;
  //  payloads.containingBlock = hack_block;
  //
  //  // find endorsed block
  //  auto endorsed_hash =
  //  hasher(popdata.atv.transaction.publicationData.header); auto
  //  endorsed_block_index = tree.getBlockIndex(endorsed_hash); if
  //  (!endorsed_block_index) {
  //    return false;
  //  }
  //  payloads.endorsed = *endorsed_block_index->header;
  //

  //  if (!tree.validatePayloads(hack_block.getHash(), payloads, state)) {
  //    stored_atvs_.erase(popdata.atvs.getId());
  //    return false;
  //  }

  return true;
}

bool MemPool::submitATV(const std::vector<ATV>& atvs, ValidationState& state) {
  for (size_t i = 0; i < atvs.size(); ++i) {
    if (!submit<ATV>(atvs[i], state)) {
      return false;
    }
  }

  return true;
}

bool MemPool::submitVTB(const std::vector<VTB>& vtbs, ValidationState& state) {
  for (size_t i = 0; i < vtbs.size(); ++i) {
    if (!submit<VTB>(vtbs[i], state)) {
      return false;
    }
  }

  return true;
}

template <typename K, typename V>
std::vector<std::pair<K, V>> getSorted(
    std::unordered_map<K, V>& map,
    std::function<bool(const std::pair<K, V>& a, const std::pair<K, V>& b)>
        cmp) {
  std::vector<std::pair<K, V>> ret(map.begin(), map.end());
  std::sort(ret.begin(), ret.end(), cmp);
  return ret;
}

std::vector<PopData> MemPool::getPop(AltTree& tree) {
  AltBlock tmp;

  {
    auto& tip = *tree.getBestChain().tip();
    tmp.hash = std::vector<uint8_t>(32, 2);
    tmp.previousBlock = tip.getHash();
    tmp.timestamp = tip.getBlockTime() + 1;
    tmp.height = tip.height + 1;
    ValidationState state;
    bool ret = tree.acceptBlock(tmp, state);
    VBK_ASSERT(ret);
  }

  // size in bytes of pop data added to
  size_t popSize = 0;

  // sorted array of VBK blocks
  using P = std::pair<VbkBlock::id_t, std::shared_ptr<VbkPayloadsRelations>>;
  std::vector<P> blocks(relations_.begin(), relations_.end());
  std::sort(blocks.begin(), blocks.end(), [](const P& a, const P& b) {
    return a.second->header.height < b.second->header.height;
  });

  for (const P& block : blocks) {
    PopData pop = block.second->toPopData();
    ValidationState state;
    if (!tree.addPayloads(tmp, pop, state)) {
      // find payloads ids that are invalid
      
    }
  }

  // clear tree from temp endorsement
  tree.removeSubtree(tmp.getHash());

  return popTxs;
}

void MemPool::removePayloads(const std::vector<PopData>& PopDatas) {
  for (const auto& tx : PopDatas) {
    // clear context
    for (const auto& b : tx.context) {
      vbkblocks_.erase(b.getShortHash());
    }

    // clear atvs
    for (const auto& atv : tx.atvs) {
      stored_vtbs_.erase(atv.getId());
    }

    // clear vtbs
    for (const auto& vtb : tx.vtbs) {
      stored_vtbs_.erase(vtb.getId());
    }
  }
}
MemPool::VbkPayloadsRelations& MemPool::touchVbkBlock(const VbkBlock& block) {
  auto sh = block.getShortHash();
  vbkblocks_[sh] = std::make_shared<VbkBlock>(block);
  if (relations_[sh] == nullptr) {
    relations_[sh] = std::make_shared<VbkPayloadsRelations>(block);
  }

  return *relations_[sh];
}

template <>
bool MemPool::submit(const ATV& atv, ValidationState& state) {
  if (!checkATV(atv, state, *alt_chain_params_, *vbk_chain_params_)) {
    return state.Invalid("pop-mempool-submit-atv");
  }

  for (const auto& b : atv.context) {
    touchVbkBlock(b);
  }

  auto& rel = touchVbkBlock(atv.containingBlock);

  auto atvid = atv.getId();
  auto atvptr = std::make_shared<ATV>(atv);
  auto pair = std::make_pair(atvid, atvptr);
  rel.atvs.push_back(atvptr);

  // clear context
  pair.second->context.clear();

  // store atv id in containing block index
  stored_atvs_.insert(pair);

  return true;
}

template <>
bool MemPool::submit(const VTB& vtb, ValidationState& state) {
  if (!checkVTB(vtb, state, *vbk_chain_params_, *btc_chain_params_)) {
    return state.Invalid("pop-mempool-submit-vtb");
  }

  for (const auto& b : vtb.context) {
    touchVbkBlock(b);
  }

  auto& rel = touchVbkBlock(vtb.containingBlock);
  auto vtbid = vtb.getId();
  auto vtbptr = std::make_shared<VTB>(vtb);
  auto pair = std::make_pair(vtbid, vtbptr);
  rel.vtbs.push_back(vtbptr);

  // clear context
  pair.second->context.clear();

  stored_vtbs_.insert(pair);

  return true;
}

template <>
bool MemPool::submit(const VbkBlock& blk, ValidationState& state) {
  if (!checkBlock(blk, state, *vbk_chain_params_)) {
    return state.Invalid("pop-mempool-submit-vbkblock");
  }

  touchVbkBlock(blk);

  return true;
}

}  // namespace altintegration
