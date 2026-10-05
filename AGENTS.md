# AGENT.md

You must adhere to this project's rules for all changes made.
All code produced must remain compliant with these rules.

## Architectural principles

- Apply the Single Responsibility Principle (SRP). 
Each module, struct, function, or component must have a clear and well-defined responsibility.

- Apply the Dependency Inversion Principle (DIP). 
High-level modules must not depend directly on low-level modules. 
Both must depend on abstractions.

## Code organization

- Follow Clean Code principles.
- Ideally, each source code file should contain a maximum of 200 lines. 
If this limit is exceeded, consider splitting the file into smaller, cohesive modules.
- Perform all changes, adjustments, bug fixes, etc., in separate branches.

- Do not add comments to the code without my permission.
- Functions must be small and have a single responsibility.
- Avoid deeply nested conditionals.

- Prefer:
- early returns; 
- `match`; 
- `if let`; 
- `let else`; 
- decomposition into smaller functions; 
- types and enums to represent states explicitly.

- Avoid code duplication. 
When there is genuinely shared behavior, extract an appropriate abstraction.

- Do not create abstractions prematurely. 
An abstraction should exist because it solves a real design problem,
not merely to anticipate a possible future need.

## Code of conduct

You may be proactive within the context of the task, but never outside of it. If, during a task, you notice a section of code that requires refactoring or correction that was not requested, do not perform it. Simply note what was identified in the final report so I can decide whether to include it in a future task. 

## C++

- Prioritize idiomatic C++ code.

- Use the type system to represent rules and states whenever possible.

- Prefer enums over boolean flags when multiple states are possible.
Avoid operations that assume success without checking for errors.
With `Option<T>`, avoid `unwrap()` and `expect()` when the absence of a value is a normal case, unless that case is handled first.
Avoid `panic!`, `unreachable!`, and `assert!` for expected runtime scenarios.
Prefer explicit error handling.

- Use `Option<T>` when a value might not exist.
- Do not silence warnings without a clear justification.

## Testing

- All relevant behavior must be covered by tests.
- Algorithms and domain rules must be testable.
- Bug fixes should, whenever possible, include a test that reproduces the issue prior to the fix.
- Deterministic domain functions must have unit tests.

## Quality

Before considering a change complete:

1. The project must compile without errors.
2. No new warnings should be introduced without justification.
3. Existing tests must continue to pass.
4. New behaviors must have tests where applicable.
5. The code must remain simple, readable, and consistent with the existing architecture.