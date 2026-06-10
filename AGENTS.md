# Agent Instructions

## C++ Code Style

Use these conventions for all new C++ code and when touching existing C++ files:

- Use `.h` headers, not `.hpp`.
- Keep each `.h` file next to its corresponding `.cpp` file under the relevant `src/` directory.
- Use PascalCase for class names.
- Use PascalCase for method names.
- Use `m_<Name>` for member variables, with PascalCase after the `m_` prefix.
- Put opening curly braces on a new line.
- Use 4 spaces for indentation.
- Order class declarations as:
  1. private fields
  2. public methods
  3. private/protected methods

Prefer updating nearby code to match these conventions when it is already being changed, but avoid broad mechanical rewrites unless the task calls for them.
