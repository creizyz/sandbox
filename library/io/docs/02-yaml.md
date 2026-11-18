# YAML I/O

The experimental API in [io/yaml.hpp](../yaml.hpp) uses the same [io::DocumentValue](../document-value.hpp)
model as [JSON I/O](01-json.md). It supports portable YAML configuration documents with
nulls, booleans, numbers, strings, sequences, and mappings with string keys.

## Reading

- `io::parse_yaml` reads a string and `io::read_yaml` reads the remaining input stream.
  Both expect one document. Errors from parsing or interpreting the document carry a
  one-based line and column.
- The reader uses YAML 1.2 Core scalar rules. Quoted and block scalars are strings unless
  an explicit supported tag specifies another type. Comments beginning with `#` are
  discarded.
- Numbers are stored as decimal tokens compatible with JSON. YAML spellings such as
  `+001`, `.5`, and `0xFF` are normalized to `1`, `0.5`, and `255`. Decimal values
  avoid floating-point conversion. Octal and hexadecimal integers must fit in 64 unsigned
  bits. Non-finite numbers are rejected.
- Anchors, aliases, custom tags, non-string keys, duplicate keys, and multiple documents
  are rejected. The nesting limit is 256 sequences or mappings.

## Writing

`io::write_yaml` returns text or writes to a stream. It uses block mappings and sequences
for nonempty collections, emits mapping keys in map order, and quotes strings to preserve
their types. It validates UTF-8, numeric tokens, and nesting before writing to a stream.

The codec uses [LibYAML](https://github.com/yaml/libyaml) for syntax and emission. Sandbox
defines the scalar rules and checks whether a document fits the shared value model.
