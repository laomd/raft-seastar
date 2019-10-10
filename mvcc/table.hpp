#pragma once

#include <list>
#include <seastar/core/future.hh>
#include "row.hpp"

BEGIN_NAMESPACE(laomd)

template<typename V>
class MVCCTable {
  using row_type = Record<V>;
public:
  seastar::future<std::list<V>> select(version_t trx_id, const std::function<bool(const V&)>& pred, size_t limit=-1) const {
    std::list<V> res;
    for (const row_type& record: datas_) {
      if (limit <= 0) {
        break;
      }
      version_t d_time = record.deleted_time();
      if (trx_id >= record.created_time() && \
          (d_time == UNSET_VERSION || d_time > trx_id) && \
          pred(record.data())) {
        res.emplace_back(record.data());
        limit--;
      }
    }
    return seastar::make_ready_future<std::list<V>>(res);
  }
  seastar::future<bool> insert(version_t trx_id, const V& value) {
    auto&& res = select(trx_id, [&value](const V& other) { return value == other; }, 1).get0();
    if (res.empty()) {
      datas_.emplace_back(Record<V>(trx_id, value));
      return seastar::make_ready_future<bool>(true);
    }
    return seastar::make_ready_future<bool>(false);
  }
  seastar::future<size_t> erase(version_t trx_id, const std::function<bool(const V&)>& pred) {
    size_t cnt = 0;
    for (row_type& record: datas_) {
      version_t d_time = record.deleted_time();
      if (trx_id >= record.created_time() && \
          (d_time == UNSET_VERSION || d_time > trx_id) && \
          pred(record.data())) {
        record.set_deleted_time(trx_id);
        cnt++;
      }
    }
    return seastar::make_ready_future<size_t>(cnt);
  }
  seastar::future<size_t> update(version_t trx_id, 
                           const std::function<bool(const V&)>& pred, 
                           const std::function<void(V&)>& setter) {
    std::list<row_type> new_records;
    size_t cnt = 0;
    for (row_type&& record: datas_) {
      version_t d_time = record.deleted_time();
      if (trx_id >= record.created_time() && \
          (d_time == UNSET_VERSION || d_time > trx_id) && \
          pred(record.data())) {
        record.set_deleted_time(trx_id);
        new_records.emplace_back(Record<V>(trx_id, record.data()));
        setter(new_records.back());
        cnt++;
      }
    }
    datas_.merge(new_records);
    return seastar::make_ready_future<size_t>(cnt);
  }
private:
  std::list<Record<V>> datas_;
};

END_NAMESPACE(laomd)