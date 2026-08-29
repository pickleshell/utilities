# Initial Task

GPU Top was used as a test of whether Next Level Agent could take a small application from an initial request to a verified working result.

## Original Prompt

Preserved verbatim from the OpenCode session:

> i wan to create an app that is similar to htop, uses the same graphics but shows graphic 780M card usage similar to ollama ps; echo; rocm-smi --showpower --showuse --showmeminfo vram output

## Parameters Selected During Brainstorming

NLA asked focused questions before presenting the design. The user selected:

- C with ncurses
- The AMD SMI C API as the primary data source
- A GPU-only interface
- Separate Ollama models and GPU processes panels
- htop-style controls, including process termination with confirmation

The user then approved the design and implementation plan. NLA divided the work into bounded tasks, used implementation and review roles, fixed issues found during review, ran the tests, and completed the application without manual coding intervention.

## Verification Recorded by NLA

- Clean build with `-Wall -Wextra`
- Unit tests for Ollama parsing, bar generation, and configuration parsing
- Working headless `--check` mode on an AMD APU
- TUI initialization and rendering under a pseudo-terminal
- Independent review after implementation and fix rounds

Interactive behavior was then confirmed by the user on the target machine.
