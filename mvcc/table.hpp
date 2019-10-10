#pragma once

#include <list>
#include <seastar/core/future.hh>
#include "row.hpp"

BEGIN_NAMESPACE(laomd)

template<typename V>
class MVCCTable {
  using row_type = Record<V>;
  using bin_constraint_type = std::function<bool(const V&, const V&)>;
  using unary_constraint_type = std::function<bool(const V&)>;
public:
  seastar::future<> add_constraint2(const bin_constraint_type& constraint) {
    bin_constraints_.emplace_back(constraint);
    return seastar::make_ready_future();
  }

  seastar::future<> add_constraint1(const unary_constraint_type& constraint) {
    unary_constraints_.emplace_back(constraint);
    return seastar::make_ready_future();
  }

  seastar::future<std::list<V>> select(version_t trx_id, const std::function<bool(const V&)>& pred, size_t limit=-1) const {
    std::list<V> res;
    for (const row_type& record: records_) {
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

  seastar::future<> insert(version_t trx_id, const V& value) {
    if (check_constraints(trx_id, value)) {
      return seastar::make_ready_future().then([&] {
        records_.emplace_back(Record<V>(trx_id, value));
      }); 
    }
    throw std::logic_error("check_constraints failed.");
  }

  seastar::future<size_t> erase(version_t trx_id, const std::function<bool(const V&)>& pred) {
    size_t cnt = 0;
    for (row_type& record: records_) {
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
    for (row_type&& record: records_) {
      version_t d_time = record.deleted_time();
      if (trx_id >= record.created_time() && \
          (d_time == UNSET_VERSION || d_time > trx_id) && \
          pred(record.data())) {
        auto new_record = Record<V>(trx_id, record.data());
        setter(new_record);
        insert(trx_id, new_record).get();  // throw an exception if failed
        record.set_deleted_time(trx_id);
        cnt++;
      }
    }
    records_.merge(new_records);
    return seastar::make_ready_future<size_t>(cnt);
  }
private:
  bool check_constraints(version_t trx_id, const V& v) const {
    for (const auto& constraint: unary_constraints_) {
      if (constraint(v)) {
        return false;
      }
    }
    for (const auto& record: records_) {
      version_t d_time = record.deleted_time();
      if (trx_id >= record.created_time() && \
          (d_time == UNSET_VERSION || d_time > trx_id)) {
        for (const auto& constraint: bin_constraints_) {
          if (constraint(record.data(), v)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  std::list<Record<V>> records_;
  std::vector<bin_constraint_type> bin_constraints_;
  std::vector<unary_constraint_type> unary_constraints_;
};

END_NAMESPACE(laomd)