# JSON I/O

The experimental API in [io/json.hpp](../json.hpp) reads and writes JSON documents using
`io::JsonValue`. JSON objects retain their member names as ordinary data, including `type`
and `value`. The binary `Value` API in [core/value.hpp](../../core/value.hpp) is separate.

## Reading

- `io::parse_json` reads a string and `io::read_json` reads the remaining input stream.
  Each expects one complete document. Parse errors carry a zero-based byte offset in
  that document. A valid JSON `null` is a successful result.
- Input accepts `//` comments through the next line break or end of input, and `/* ... */`
  comments through the first closing delimiter. Comments can appear wherever whitespace
  is allowed. String contents are preserved, and trailing commas are rejected.
- Strings are UTF-8. Unicode escapes are decoded to UTF-8. Malformed UTF-8, invalid
  escapes, and unpaired UTF-16 surrogates are rejected.
- When a member name appears more than once, its last value wins. The nesting limit is
  256 arrays or objects.

## Writing

`io::write_json` returns text or writes to a stream. The default output is compact.
Pretty output uses two spaces per nesting level. Both produce standard JSON and emit
object keys in map order. Comments are discarded when reading.

Numbers retain their original JSON token in `io::JsonNumber::m_text`. A caller that
needs a native numeric type can select a conversion and its range policy. Writing
validates number tokens and UTF-8 and applies the same nesting limit as parsing.
