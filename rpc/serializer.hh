#pragma once

#include <seastar/core/sstring.hh>
#include <seastar/rpc/rpc.hh>

namespace laomd {
struct rpc_serializer {};
namespace rpc = seastar::rpc;
using seastar::sstring;

template <typename T, typename Output>
inline void write_arithmetic_type(Output &out, T v) {
  static_assert(std::is_arithmetic<T>::value, "must be arithmetic type");
  return out.write(reinterpret_cast<const char *>(&v), sizeof(T));
}

template <typename T, typename Input> inline T read_arithmetic_type(Input &in) {
  static_assert(std::is_arithmetic<T>::value, "must be arithmetic type");
  T v;
  in.read(reinterpret_cast<char *>(&v), sizeof(T));
  return v;
}

template <typename Output>
inline void write(rpc_serializer, Output &output, bool v) {
  return write_arithmetic_type(output, v);
}
template <typename Output>
inline void write(rpc_serializer, Output &output, int32_t v) {
  return write_arithmetic_type(output, v);
}
template <typename Output>
inline void write(rpc_serializer, Output &output, uint32_t v) {
  return write_arithmetic_type(output, v);
}
template <typename Output>
inline void write(rpc_serializer, Output &output, int64_t v) {
  return write_arithmetic_type(output, v);
}
template <typename Output>
inline void write(rpc_serializer, Output &output, uint64_t v) {
  return write_arithmetic_type(output, v);
}
template <typename Output>
inline void write(rpc_serializer, Output &output, double v) {
  return write_arithmetic_type(output, v);
}

template <typename Input>
inline bool read(rpc_serializer, Input &input, rpc::type<bool>) {
  return read_arithmetic_type<bool>(input);
}
template <typename Input>
inline int32_t read(rpc_serializer, Input &input, rpc::type<int32_t>) {
  return read_arithmetic_type<int32_t>(input);
}
template <typename Input>
inline uint32_t read(rpc_serializer, Input &input, rpc::type<uint32_t>) {
  return read_arithmetic_type<uint32_t>(input);
}
template <typename Input>
inline uint64_t read(rpc_serializer, Input &input, rpc::type<uint64_t>) {
  return read_arithmetic_type<uint64_t>(input);
}
template <typename Input>
inline uint64_t read(rpc_serializer, Input &input, rpc::type<int64_t>) {
  return read_arithmetic_type<int64_t>(input);
}
template <typename Input>
inline double read(rpc_serializer, Input &input, rpc::type<double>) {
  return read_arithmetic_type<double>(input);
}

template <typename Output>
inline void write(rpc_serializer, Output &out, const sstring &v) {
  write_arithmetic_type(out, uint32_t(v.size()));
  out.write(v.c_str(), v.size());
}

template <typename Input>
inline sstring read(rpc_serializer, Input &in, rpc::type<sstring>) {
  auto size = read_arithmetic_type<uint32_t>(in);
  sstring ret(sstring::initialized_later(), size);
  in.read(ret.begin(), size);
  return ret;
}
} // namespace laomd