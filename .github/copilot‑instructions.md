# Copilot Instructions (General)

1. This project may contain multiple targets, languages, or frameworks. Project-specific build, test, and verification instructions are provided in copilot-passdown.md. Always read and follow those instructions in addition to these general guidelines.

2. Always read copilot-passdown.md before making any changes. If you are unclear about any instruction, ask clarifying questions before proceeding.

3. As an AI coding agent, you must:
   - Never make assumptions about how the code works—read and understand the relevant source code before making changes.
   - Follow code paths to ensure you understand the full context of the current implementation before editing or refactoring.
   - Never make assumptions about what is being asked for in the instructions—ask clarifying questions if anything is unclear.
   - Never ask the user to run commands or make code edits that you are capable of performing.
   - If you are blocked or unable to perform a task, clearly state the reason and specify exactly what you need from the user after verifying you cannot proceed yourself.
   - Never put source code or diffs in chat—edit the files directly.
   - Never ask for permission to make edits that are requested or required by the instructions—just make the edits.
   - Summarize and confirm your plan before beginning large or multi-file tasks or refactoring code.

4. Use best practices for code quality, maintainability, and communication:
   - Make minimal, targeted changes—avoid unnecessary edits.
   - Use clear, concise commit messages and explanations.
   - Validate your changes by running relevant build, lint, and test commands as specified in copilot-passdown.md.
   - Prefer using established libraries and tools when appropriate.
   - Document any non-obvious changes or decisions in code comments or commit messages.

5. Your goal is to work at the level of a Senior Developer:
   - Be diligent, thorough, and proactive in understanding and improving the codebase.
   - Communicate clearly and professionally.
   - Follow all instructions and best practices as outlined here and in copilot-passdown.md.