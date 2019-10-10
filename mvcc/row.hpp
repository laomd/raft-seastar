#pragma once

#include <seastar/core/future.hh>
#include <lao_utils/check.hh>

BEGIN_NAMESPACE(laomd)

using version_t = uint64_t;
#define UNSET_VERSION 0

template <typename V> 
class Record {
public:
  Record(version_t trx_id, const V& data) {
    DCHECK(trx_id != UNSET_VERSION);
    created_time_ = trx_id;
    deleted_time_ = UNSET_VERSION;
    data_ = data;
  }

  const V& data() const { return data_; }
  V& data() { return data_; }
  version_t created_time() const { return created_time_; }
  version_t deleted_time() const { return deleted_time_; }

  void set_deleted_time(version_t trx_id) {
    DCHECK(trx_id != UNSET_VERSION);
    deleted_time_ = trx_id;
  }
private:
  V data_;
  version_t created_time_;
  version_t deleted_time_;
};

END_NAMESPACE(laomd)