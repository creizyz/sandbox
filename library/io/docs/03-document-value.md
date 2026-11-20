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

## Core binary framing and errors

A binary stream contains consecutive values. Tags and booleans occupy one byte.
Other numeric payloads retain the writer's native representation and byte order.
Strings, arrays and objects use native-endian 16-bit unsigned lengths or element
counts (maximum 65,535). In a tag-11 object, each key starts with a native
`std::size_t` byte length, as in earlier writers. Exchanging binary data requires
compatible type representations, including the width of `std::size_t`.

`write_binary` checks representable lengths throughout the value before emitting a
tag. Oversized fields set the output stream's failure bit. An underlying I/O error
can leave partial output. `read_binary` consumes one value and returns an empty
`Value` on failure. Check the stream to distinguish failure from a successful
null. Unknown tags, invalid boolean bytes, duplicate object keys and incomplete
payloads fail the entire enclosing value. Stream exception masks apply to both
operations.
