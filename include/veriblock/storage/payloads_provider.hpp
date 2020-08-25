// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_PAYLOADS_PROVIDER_HPP
#define VERIBLOCK_POP_CPP_PAYLOADS_PROVIDER_HPP

#include <veriblock/blockchain/blockchain_util.hpp>
#include <veriblock/entities/altblock.hpp>
#include <veriblock/entities/popdata.hpp>

namespace altintegration {

struct AltTree;
struct VbkBlockTree;

struct PayloadsProvider {
  virtual ~PayloadsProvider() = default;

  virtual std::vector<ATV> getATVs(const std::vector<ATV::id_t>& id) = 0;
  virtual std::vector<VTB> getVTBs(const std::vector<VTB::id_t>& id) = 0;
  virtual std::vector<VbkBlock> getVBKs(
      const std::vector<VbkBlock::id_t>& id) = 0;

  /**
   * Load commands from a particular block.
   * @param[in] tree load from this tree
   * @param[in] block load from this block
   * @param[out] out output vector of commands
   * @param[out] state if commands can't be loaded, will be set to Error
   * @return true if loaded successfully, false otherwise
   */
  virtual bool getCommands(AltTree& tree,
                           const BlockIndex<AltBlock>& block,
                           std::vector<CommandGroup>& out,
                           ValidationState& state);

  /**
   * Load commands from a particular block.
   * @param[in] tree load from this tree
   * @param[in] block load from this block
   * @param[out] out output vector of commands
   * @param[out] state if commands can't be loaded, will be set to Error
   * @return true if loaded successfully, false otherwise
   */
  virtual bool getCommands(VbkBlockTree& tree,
                           const BlockIndex<VbkBlock>& block,
                           std::vector<CommandGroup>& out,
                           ValidationState& state);
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_PAYLOADS_PROVIDER_HPP
