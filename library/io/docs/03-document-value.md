# Document values and core conversion

[`io::DocumentValue`](../document-value.hpp) is the shared JSON and YAML value type.
Its alternatives are null, boolean, number, string, array, and string-keyed object.
The [core `Value`](../../core/value.hpp) also has distinct native numeric types and
supports binary serialization.

## Converting to and from core

`io::DocumentValue::from_core()` and `to_core()` convert recursively and return
`std::expected`. They preserve booleans, null, string data, and collection contents.
`io::Number::from<T>()` and `to<T>()` support the exact numeric alternatives in core
`Value`. Numbers converted from core carry a type hint so a direct round trip recovers
the original C++ type. Character alternatives are represented by their numeric codes.

JSON and YAML write plain numbers and omit the C++ type hint. A number parsed from either
format starts without a hint. Converting it to core uses `int64_t` for integer tokens,
`uint64_t` for nonnegative integers above the signed range, and `double` for decimal or
exponent tokens. Floating-point conversion may round. Document equality compares number
tokens and ignores the type hint.

Conversion reports malformed tokens, values outside the destination range, non-finite core
numbers, and nesting deeper than 256 containers.

## Core binary booleans

Core `Value` uses `std::monostate` for null and a separate `bool` alternative for
`true` and `false`. Binary booleans use tag 12 followed by one byte, 0 or 1.
The existing binary tags 0 through 11 retain their meanings. Older readers do not support tag 12.
