// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_BASE_REPOSITORY_HPP
#define VERIBLOCK_POP_CPP_BASE_REPOSITORY_HPP

#include <string>

#include "cursor.hpp"
#include "serializers.hpp"
#include "write_batch.hpp"
#include "base_cursor.hpp"

namespace altintegration {

struct BaseRepository {
  using key_t = std::string;
  using value_t = std::string;
  using cursor_t = BaseCursor;
  using batch_t = WriteBatch<key_t, value_t>;

  virtual ~BaseRepository() = default;

  virtual bool remove(const key_t& key) = 0;

  virtual bool put(const key_t& key, const value_t& value) = 0;

  virtual bool get(const key_t& key, value_t* value = 0) const = 0;

  virtual size_t getMany(Slice<const key_t> keys,
                         std::vector<value_t>* values = 0) const = 0;

  virtual void clear() = 0;

  virtual std::unique_ptr<batch_t> newBatch() = 0;

  virtual std::shared_ptr<cursor_t> newCursor() const = 0;
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_BASE_REPOSITORY_HPP
