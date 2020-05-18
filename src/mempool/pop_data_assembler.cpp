// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "veriblock/mempool/pop_data_assembler.hpp"

namespace altintegration {

namespace {

bool connectsTo(const VbkBlock& check_block,
              const VbkBlock& current_block) {
  return check_block.previousBlock == current_block.getShortHash();
}

bool connectsTo(const VbkBlock& check_block,
                const VbkBlockTree& tree) {
  return tree.getBlockIndex(check_block.previousBlock) != nullptr;
}

std::vector<std::pair<ATV::id_t, ATV>> getSortedATVs(
    const std::unordered_map<ATV::id_t, ATV>& map) {
  std::vector<std::pair<ATV::id_t, ATV>> sorted_atvs(map.size());
  std::copy(map.begin(), map.end(), sorted_atvs.begin());

  auto atv_comparator = [](const std::pair<ATV::id_t, ATV>& el1,
                           const std::pair<ATV::id_t, ATV>& el2) -> bool {
    return el1.second.containingBlock.height <
           el2.second.containingBlock.height;
  };
  std::sort(sorted_atvs.begin(), sorted_atvs.end(), atv_comparator);
  return sorted_atvs;
}

}  // namespace

bool PopDataAssembler::fillContext(VbkBlock first_block,
                                   std::vector<VbkBlock>& context,
                                   AltTree& tree) {
  while (!connectsTo(first_block, tree.vbk())) {
    auto el = mempool_.getVbkBlockIndex().find(first_block.previousBlock);
    if (el != mempool_.getVbkBlockIndex().end()) {
      first_block = el->second;
      context.push_back(first_block);
    } else {
      return false;
    }
  }

  // reverse context
  std::reverse(context.begin(), context.end());

  return true;
}

void PopDataAssembler::fillVTBs(std::vector<VTB>& vtbs,
                                const std::vector<VbkBlock>& vbk_context) {
  for (const auto& b : vbk_context) {
    for (auto vtb_it = mempool_.getVTBIndex().begin();
         vtb_it != mempool_.getVTBIndex().end();
         ++vtb_it) {
      if (vtb_it->second.containingBlock == b) {
        vtbs.push_back(vtb_it->second);
      }
    }
  }
}

bool PopDataAssembler::applyPayloads(const AltBlock& hack_block,
                                     PopData& popdata,
                                     AltTree& tree,
                                     ValidationState& state) {
  bool ret = tree.acceptBlock(hack_block, state);
  assert(ret);

  // apply vbk_context
  for (const auto& b : popdata.vbk_context) {
    ret = tree.vbk().acceptBlock(b, state);
    assert(ret);
  }

  auto genesis_height = tree.vbk().getParams().getGenesisBlock().height;
  auto settlement_interval =
      tree.vbk().getParams().getEndorsementSettlementInterval();
  // check VTB endorsements
  for (auto it = popdata.vtbs.begin(); it != popdata.vtbs.end();) {
    VTB& vtb = *it;
    auto* containing_block_index =
        tree.vbk().getBlockIndex(vtb.containingBlock.getHash());

    auto start_height = std::max(
        genesis_height, containing_block_index->height - settlement_interval);

    auto endorsement = BtcEndorsement::fromContainer(vtb);
    Chain<BlockIndex<VbkBlock>> chain(start_height, containing_block_index);
    auto duplicate =
        chain.findBlockContainingEndorsement(endorsement, settlement_interval);

    // invalid vtb
    if (duplicate) {
      // remove from storage
      mempool_.getVTBIndex().erase(vtb.getId());

      it = popdata.vtbs.erase(it);
      continue;
    }
    ++it;
  }

  for (const auto& b : popdata.vbk_context) {
    tree.vbk().removeSubtree(b.getHash());
  }

  AltPayloads payloads;
  payloads.popData = popdata;
  payloads.containingBlock = hack_block;

  // find endorsed block
  auto endorsed_hash = hasher(popdata.atv.transaction.publicationData.header);
  auto endorsed_block_index = tree.getBlockIndex(endorsed_hash);
  if (!endorsed_block_index) {
    return false;
  }
  payloads.endorsed = *endorsed_block_index->header;

  if (!tree.validatePayloads(hack_block.getHash(), payloads, state)) {
    mempool_.getATVIndex().erase(popdata.atv.getId());
    return false;
  }

  return true;
}

std::vector<PopData> PopDataAssembler::getPop(const AltBlock& current_block,
                                              AltTree& tree) {
  ValidationState state;
  bool ret = tree.setState(current_block.getHash(), state);
  (void)ret;
  assert(ret);

  AltBlock hack_block;
  hack_block.hash = std::vector<uint8_t>(32, 0);
  hack_block.previousBlock = current_block.getHash();
  hack_block.timestamp = current_block.timestamp + 1;
  hack_block.height = current_block.height + 1;


  std::vector<std::pair<ATV::id_t, ATV>> sorted_atvs =
      getSortedATVs(mempool_.getATVIndex());

  std::vector<PopData> popTxs;
  for(size_t i = 0; i < sorted_atvs.size() && i < tree.getParams().getMaxPopDataPerBlock(); ++i) {
    auto& atv = sorted_atvs[i].second;
    PopData popTx;
    VbkBlock first_block =
        !atv.context.empty() ? atv.context[0] : atv.containingBlock;

    if (!connectsTo(first_block, tree.vbk())) {
      if (fillContext(first_block, popTx.vbk_context, tree)) {
        fillVTBs(popTx.vtbs, popTx.vbk_context);
        popTx.atv = atv;
        popTx.hasAtv = true;
        if (applyPayloads(hack_block, popTx, tree, state)) {
          popTxs.push_back(popTx);
        }
      }
    } else {
      popTx.atv = atv;
      popTx.hasAtv = true;
      if (applyPayloads(hack_block, popTx, tree, state)) {
        popTxs.push_back(popTx);
      }
    }
  }

  // clear tree from temp endorsement
  tree.removeSubtree(hack_block.getHash());

  return popTxs;
}

}  // namespace altintegration
