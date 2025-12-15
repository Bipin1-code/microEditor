
# **Terminal Text Editor**
A minimal terminal-based text editor written from scratch in C — inspired by classic editors like kilo, nano, and small parts of Emacs.
This is a learning-focused project, built to strengthen understanding of terminal handling, raw mode, rendering, buffers, file I/O, and editor architecture.
---
## 🚀 Project Purpose

- The goal of this project is to build a fully functional text editor from the ground up, without using external libraries.
- The editor focuses on understanding:
  - Raw terminal modes
  - Escape sequences & VT processing
  - Efficient screen rendering
  - Text buffer data structures
  - Editor state machine
  - File open/save operations
  - Line editing
  - Cross-platform separation (planned)
---
#### **Warning:** This is not intended to be a production-grade editor.
#### It is a carefully planned learning project to deeply understand how editors like kilo, nano, and small parts of Emacs internally work.
---
## 🎯 Milestones
### ✔️ Milestone 1 — Raw Mode

- Disable canonical mode
- Disable echo, signals, and other default terminal handling
- Read input byte-by-byte

### ✔️ Milestone 2 — Keyboard Input Layer

- Recognize normal keys
- Handle special keys (arrows, Home/End, PageUp/PageDown)
- Parse multi-byte escape sequences

### ✔️ Milestone 3 — Text Buffer / Rows

- Editable text stored as dynamic rows
- Efficient insert/delete operations
- Row rendering logic (tabs → spaces, etc.)

### ✔️ Milestone 4 — Screen Rendering

- Full-screen redraw engine
- Scroll logic (vertical & horizontal)
- Cursor tracking & viewport handling

### ✔️ Milestone 5 — File Opening

- Load file into row buffer
- Handle any file size (limited by memory)
- Render upon open

### ✔️ Milestone 6 — Status Bar

- Two-line layout:
  - One status/info bar
  - One message/minibuffer area
  - Temporary prompt messages

 - Keybindings indicator (example: Ctrl+G)

### ⚙️ Milestone 7 — Saving

- Next implementation step:
  - Press Ctrl-S to save
  - Join all editor rows into a single text block
  - Write to a temporary file
  - Atomically rename → ensures safe write
  - Add “Save As” (create file if not exists)
---
## 📌 **Upcoming Features (TODO)**
- Mini prompt (like Emacs minibuffer) for commands. We have, but we can improve it more
- Cross-platform abstraction layer (Windows + POSIX) 
- Syntax highlighting (long-term goal)
- folder sturcture view
- function or paragrah collapse
- Jump function of cursor
---
## 📘 Project Roadmap Philosophy

This project is part of a larger Software Foundations Roadmap.
You will revisit this editor multiple times per week, progressively adding features and using it as a platform to apply new concepts from future projects.

*This approach ensures:*
- Continuous revision of past concepts
- Deep reinforcement of fundamentals
- A growing “playground” project where new ideas can be integrated
- Practical experience with low-level text processing and UI

---
## 🧠 Why This Project Matters

1. Building a text editor teaches essential systems concepts:
2. Terminal I/O
3. Efficient data structures
4. Incremental rendering
5. Memory management
6. File safety
7. Cross-platform concerns
8. Event loops
9. User interface design without GUI frameworks

# ***It’s one of the best hands-on projects for becoming a strong systems programmer.***
