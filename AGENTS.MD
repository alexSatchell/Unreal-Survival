## C++ Formatting

- Prefer compact, single-line C++ statements.
- Do not wrap function calls, argument lists, constructor calls, delegates, or conditionals across multiple lines unless the resulting line would exceed 200 characters.
- Maximum preferred line length: 200 characters.
- Keep function arguments on the same line whenever the complete statement is 200 characters or fewer.
- Do not vertically format arguments merely for readability.
- When modifying existing code, preserve this compact formatting style.

## Unreal Engine C++ Coding Standards

Follow Epic's [C++ Coding Standard for Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine?lang=en-US). The rules below summarize the standards that apply to this project; the Epic documentation is the source of truth when a detail is not listed here.

### Naming and Types

- Use U.S. English, PascalCase, clear descriptive names, and avoid unnecessary abbreviations.
- Use Unreal type prefixes: `U` for `UObject` classes, `A` for `AActor` classes, `S` for Slate widgets, `I` for interfaces, `T` for template classes, `F` for most other structs/classes, `E` for enums, and `C` for concept-like structs.
- Prefix boolean variables and parameters with `b`; prefix output reference parameters with `Out` and input parameters with `In` where useful, such as `bOutResult`.
- Name types and variables with nouns; name methods with verbs or with the value they return. Boolean-returning methods must ask a true/false question, such as `IsVisible()` or `ShouldClearBuffer()`.
- Name macros in uppercase with underscores and a `UE_` prefix. Do not use namespaces for macros.
- Use Unreal's portable types (`int32`, `uint8`, `int64`, `TCHAR`, and so on) when width matters. Never assume the size of `bool`, `TCHAR`, or `PTRINT`.

### Unreal APIs and Modern C++

- Organize classes for readers: public interface first, then protected/private implementation. Keep members private unless they are part of the class interface; use accessors for derived-class access and `final` for classes not designed for inheritance.
- Use `override` and `final` where applicable. Overriding methods should declare both `virtual` and `override`.
- Use `nullptr`, never `NULL`. Use `static_assert` for compile-time assertions.
- Prefer explicit types over `auto`; only use `auto` for lambda variables, verbose iterators, or advanced template cases where the type is not reasonably discernible. Do not use structured bindings.
- Prefer range-based `for` loops. Keep lambdas short, use explicit captures instead of `[&]` or `[=]`, and use weak delegate wrappers for deferred UObject/shared-pointer work.
- Prefer strongly typed enum classes. Flag enums must define `None = 0` and use `ENUM_CLASS_FLAGS`; compare flags explicitly against `None`.
- Use `MoveTemp` for intentional moves of Unreal containers and strings. Use default member initializers when they improve clarity, especially in gameplay code.
- Pass `UObject` types by pointer rather than reference. Avoid boolean flag parameters; prefer a strongly typed enum or parameter struct when an API has multiple options.

### Unreal Containers, Libraries, and Dependencies

- Prefer Unreal containers and strings (`TArray`, `TMap`, `TSet`, `FString`, and related types). Avoid standard containers and strings except for interoperability; use standard-library features when they provide the superior, supported result without mixing idioms within one API.
- Keep dependencies narrow: use `#pragma once`, prefer forward declarations, include the specific headers required, do not rely on transitive includes, and avoid standard-library includes from headers where possible.
- Put externally needed module headers in `Public` and implementation-only code in `Private`. Keep non-trivial implementation out of headers; use inline functions only for trivial accessors or measured performance benefits, and use `FORCEINLINE` sparingly.
- Keep platform-specific implementation in platform-specific source directories and behind platform abstractions. Avoid scattering `PLATFORM_*` conditionals through cross-platform code.
- Mark modified third-party library code with `//@UE5` and an explanation. Mark included third-party sections with searchable begin/end comments.

### Formatting and Control Flow

- Put opening braces on a new line and always use braces, including for one-statement `if`, `else`, and loop bodies.
- Use tabs for leading indentation with a tab size of 4; spaces are allowed only for alignment where needed. Apply the same rule to Unreal C# files.
- Always include a `default` case in `switch` statements. Explicitly break or comment every intentional fall-through.
- Keep pointer and reference symbols next to the type: `FThing* Thing` and `FThing& Thing`.
- Do not shadow variables. Avoid anonymous literals in calls when named constants would clarify intent, and use intermediate variables for complicated expressions.
- Use `TEXT()` around string literals. Keep a blank line at the end of every `.h` and `.cpp` file.
- Fix compiler warnings rather than suppressing them; use `#pragma` only as a last resort. Do not check in unpolished debug code.

### Comments and Source Organization

- Write self-documenting code and comments that explain intent, not obvious mechanics. Do not use comments to justify code that should be rewritten.
- Document public classes with the problem they solve and why they exist. Document public methods once, at their declaration, including purpose, parameter units/ranges/invalid values, return meaning, and relevant `@warning`, `@note`, `@see`, or `@deprecated` details.
- Use respectful, inclusive, professional language in names, comments, UI text, errors, and notifications. Avoid slang, profanity, stereotypes, and terms such as `blacklist`, `whitelist`, `master`, and `slave`; use precise alternatives.
- Avoid unnecessary whitespace-only or mass-renaming changes mixed with behavioral changes. Keep functions and methods focused, split large logic into named sub-functions, and minimize dependency distance.

### Editor-Facing Property Documentation

- Every designer-facing `UPROPERTY` must include a clear `ToolTip` metadata description.
- Tooltips must explain the property's purpose, units or scale, valid range, and any special zero, empty, or tag-matching behavior.
- Apply this rule to data assets, actor instances, components, structs, and other properties exposed in the Unreal Details panel.
- Treat hover documentation as part of the feature: add or update the tooltip whenever an exposed property is added or its behavior changes.
- For transformable actors, use a valid `USceneComponent` root so the standard Actor Transform fields remain available in the Details panel.

### Change Verification

- Do not compile the full Unreal project just to test routine changes. A focused check in the scratchpad is sufficient unless a full build is specifically requested.

## Output style

The reader has ADHD. Shape every response so it can be acted on:

1. Lead with the answer or next action: command, path, or snippet first.
2. Number multi-step work; one bounded action per step.
3. End with one next action doable in under two minutes.
4. Finish the current issue before raising a new one.
5. Restate progress each turn ("step 3 of 5 done").
6. Give time estimates in concrete units, never "a bit".
7. After a change, show what now works.
8. Errors: state location, cause, and fix. No drama.
9. Rank and group long lists; aim for at most five items per group without omitting relevant items.
10. No preamble, no recaps, no closers.

Exceptions: explain fully when asked to explain. Confirm before destructive actions. After three failed fixes, stop and name the doubtful assumption. If the request is ambiguous, ask one short question.
