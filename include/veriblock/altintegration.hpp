// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALTINTEGRATION_ALTINTEGRATION_HPP
#define ALTINTEGRATION_ALTINTEGRATION_HPP

#include <veriblock/alt-util.hpp>
#include <veriblock/blockchain/alt_block_tree.hpp>
#include <veriblock/config.hpp>
#include <veriblock/mempool.hpp>
#include <veriblock/storage/payloads_storage.hpp>

namespace altintegration {

struct Altintegration {
  static std::shared_ptr<Altintegration> create(
      const std::shared_ptr<Config>& config, std::shared_ptr<Repository>& db) {
    config->validate();

    std::shared_ptr<Altintegration> service =
        std::make_shared<Altintegration>();
    service->config = std::move(config);
    service->repo = std::move(db);
    service->store = std::make_shared<PayloadsStorage>(*service->repo);
    service->altTree = std::make_shared<AltTree>(*service->config->alt,
                                                 *service->config->vbk.params,
                                                 *service->config->btc.params,
                                                 *service->store);
    service->mempool =
        std::make_shared<altintegration::MemPool>(*service->altTree);

    ValidationState state;

    // first, bootstrap BTC
    if (service->config->btc.blocks.empty()) {
      service->altTree->btc().bootstrapWithGenesis(state);
      VBK_ASSERT(state.IsValid());
    } else {
      service->altTree->btc().bootstrapWithChain(
          service->config->btc.startHeight, service->config->btc.blocks, state);
      VBK_ASSERT(state.IsValid());
    }

    // then, bootstrap VBK
    if (service->config->vbk.blocks.empty()) {
      service->altTree->vbk().bootstrapWithGenesis(state);
      VBK_ASSERT(state.IsValid());
    } else {
      service->altTree->vbk().bootstrapWithChain(
          service->config->vbk.startHeight, service->config->vbk.blocks, state);
      VBK_ASSERT(state.IsValid());
    }

    service->altTree->bootstrap(state);
    return service;
  }

  bool checkPopData(const PopData& popData, ValidationState& state) {
    if (!checkVbkBlocks(popData.context, state, *config->vbk.params)) {
      return state.Invalid("pop-vbkblock-statelessly-invalid");
    }

    for (const auto& vtb : popData.vtbs) {
      if (!checkVTB(vtb, state, *config->btc.params)) {
        return state.Invalid("pop-vtb-statelessly-invalid");
      }
    }

    for (const auto& atv : popData.atvs) {
      if (!checkATV(atv, state, *config->alt)) {
        return state.Invalid("pop-atv-statelessly-invalid");
      }
    }

    return true;
  }

  std::shared_ptr<Config> config;
  std::shared_ptr<Repository> repo;
  std::shared_ptr<MemPool> mempool;
  std::shared_ptr<AltTree> altTree;
  std::shared_ptr<PayloadsStorage> store;
  std::vector<PopData> disconnected_popdata;
};

}  // namespace altintegration

#endif  // ALTINTEGRATION_ALTINTEGRATION_HPP
