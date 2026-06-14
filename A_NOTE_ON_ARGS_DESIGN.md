# Args Refactor Design Document

## Overview
Refactoring args.cpp to remove CLI11 dependency and dramatically reduce implementation size using modern C++20 features and a lightweight recursive descent parser.

## BNF Grammar

```bnf
<command_line>  ::= [<program_name>] <option>*

<option>        ::= <flag> 
                  | <long_option> 
                  | <flag_with_value> 
                  | <long_option_with_value>

<flag>          ::= '-' <letter>

<long_option>   ::= '--' <identifier>

<flag_with_value> ::= '-' <letter> <value>
                    | '-' <letter> '=' <value>
                    | '-' <letter><value>           ; concatenated form like -r10

<long_option_with_value> ::= '--' <identifier> '=' <value>
                           | '--' <identifier> <value>

<value>         ::= <slice_notation>
                  | <json_string>
                  | <simple_value>

<slice_notation> ::= '[' [<number>] ':' [<number>] ']'

<json_string>   ::= '`' <json_content> '`'

<simple_value>  ::= [^-][^ ]*

<program_name>  ::= <identifier>

<identifier>    ::= [a-zA-Z_][a-zA-Z0-9_-]*

<letter>        ::= [a-zA-Z]

<number>        ::= [0-9]+
```

## AST Structure (Simplified)

```cpp
// Token types for lexical analysis
enum class TokenType {
    Program,      // program name
    ShortFlag,    // -r, -c, etc.
    LongOption,   // --rows, --columns, etc.
    Value,        // any value (number, string, etc.)
    JsonString,   // `{...}` 
    SliceNotation,// [0:10]
    EndOfInput
};

struct Token {
    TokenType type;
    std::string text;
    size_t position;
};

// Parsed result structure (internal to parser)
struct ParsedOption {
    std::string key;      // normalized key (word form)
    std::string value;    // the value
    bool is_flag{false};  // true if it's a boolean flag
};
```

## Key Design Decisions

### 1. **Tokenization Phase**
- Split input into tokens (flags, options, values, special syntax)
- Recognize slice notation `[n:m]` as a single token
- Recognize JSON strings (backtick-enclosed) as single tokens
- Handle concatenated forms like `-r10`

### 2. **Parsing Phase**
- Recursive descent parser processes token stream
- Map short flags, long options, and word keys to same value
- Special handling for:
  - Distances flag with optional slice notation
  - JSON input (string or file path)
  - Boolean flags (no value)
