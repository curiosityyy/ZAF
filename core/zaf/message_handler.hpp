#pragma once

#include <memory>

#include "callable_signature.hpp"
#include "message.hpp"
#include "macros.hpp"
#include "zaf_exception.hpp"

namespace zaf {
// Store the type-erased user-defined message handler
struct MessageHandler {
  virtual void process(Message&) = 0;
  virtual ~MessageHandler() = default;
};

// Store the original user-defined message handler
// and process the messages with the handler
template<typename Handler>
class TypedMessageHandler : public MessageHandler {
private:
  Handler handler;
  using ArgTypes = typename traits::is_callable<Handler>::args_t;
  std::vector<std::uintptr_t> message_element_addrs;

public:
  template<typename H>
  TypedMessageHandler(H&& handler): handler(std::forward<H>(handler)) {
    static_assert(traits::is_callable<Handler>::value,
      " Not an acceptable handler.");
    message_element_addrs.resize(ArgTypes::size);
  }

  // recover the types of the arguments in the message
  // then forward the arguments into the handler
  void process(Message& m) override {
    if (m.types_hash_code() != ArgTypes::hash_code()) {
      throw ZAFException("The hash code of the message content types does not"
        " match with the argument types of the message handler.",
        " Expected: ", ArgTypes::hash_code(),
        " Actual: ", m.types_hash_code());
    }
    // no `auto` is allowed in the argument type, otherwise we need to
    // match the lambda with arguments in compile time.
    if (m.is_serialized()) {
      if constexpr (traits::all_handler_arguments_serializable<ArgTypes>::value) {
        process(static_cast<SerializedMessage&>(m),
          std::make_index_sequence<ArgTypes::size>());
      } else {
        throw ZAFException("The TypedMessageHandler contains non-serializable"
          " argument(s) but receives a serialized message.");
      }
    } else {
      process(m, std::make_index_sequence<ArgTypes::size>());
    }
  }

  template<size_t ... I>
  inline void process(Message& m, std::index_sequence<I ...>) {
    m.fill_with_element_addrs(message_element_addrs);
    try {
      handler(
        static_cast<typename ArgTypes::template arg_t<I>>(
          *reinterpret_cast<typename ArgTypes::template decay_arg_t<I>*>(
            message_element_addrs.operator[](I)
          )
        )...
      );
    } catch (...) {
      std::throw_with_nested(ZAFException(
        "Exception caught in ", __PRETTY_FUNCTION__,
        " when handling typed message with code ", m.get_code()
      ));
    }
  }

  template<size_t ... I>
  inline void process(SerializedMessage& m, std::index_sequence<I ...>) {
    auto&& content = m.deserialize_content<ArgTypes>();
    try {
      handler(
        static_cast<typename ArgTypes::template arg_t<I>>(
          std::get<I>(content)
        )...
      );
    } catch (...) {
      std::throw_with_nested(ZAFException(
        "Exception caught in ", __PRETTY_FUNCTION__,
        " when handling serialized message with code ", m.get_code()
      ));
    }
  }
};
} // namespace zaf
