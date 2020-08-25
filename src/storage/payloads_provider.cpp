// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <veriblock/blockchain/alt_block_tree.hpp>
#include <veriblock/storage/payloads_provider.hpp>

namespace altintegration {

bool PayloadsProvider::getCommands(AltTree& tree,
                                   const BlockIndex<AltBlock>& block,
                                   std::vector<CommandGroup>& out,
                                   ValidationState& state) {
  try {
    auto vbks = getVBKs(block.getPayloadIds<VbkBlock>());
    auto vtbs = getVTBs(block.getPayloadIds<VTB>());
    auto atvs = getATVs(block.getPayloadIds<ATV>());

    auto containingHash = block.getHash();

    vectorPopToCommandGroup<AltTree, VbkBlock>(tree, vbks, containingHash, out);
    vectorPopToCommandGroup<AltTree, VTB>(tree, vtbs, containingHash, out);
    vectorPopToCommandGroup<AltTree, ATV>(tree, atvs, containingHash, out);

  } catch (const std::exception& e) {
    return state.Error(
        fmt::format("{}: Can't get commands for block={}, reason={}",
                    __func__,
                    block.toPrettyString(),
                    e.what()));
  }

  return true;
}

bool PayloadsProvider::getCommands(VbkBlockTree& tree,
                                   const BlockIndex<VbkBlock>& block,
                                   std::vector<CommandGroup>& out,
                                   ValidationState& state) {
  try {
    auto vtbs = getVTBs(block.getPayloadIds<VTB>());
    auto containingHash = block.getHash().asVector();
    vectorPopToCommandGroup<VbkBlockTree, VTB>(tree, vtbs, containingHash, out);
  } catch (const std::exception& e) {
    return state.Error(
        fmt::format("{}: Can't get commands for block={}, reason={}",
                    __func__,
                    block.toPrettyString(),
                    e.what()));
  }

  return true;
}
}  // namespace altintegration
