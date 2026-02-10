# FLTK Text Editor

A lightweight, cross-platform text editor built with **C++17** and **FLTK** — featuring file management, find & replace, clipboard operations, change tracking, and a live status bar with real-time cursor position.

---

## Features

- **File Operations** — New, Open, Save, Save As with native file dialogs
- **Find & Replace** — Search forward through documents with optional text replacement
- **Clipboard** — Cut, Copy, Paste, Delete with standard keyboard shortcuts
- **Change Tracking** — Modified state displayed in the title bar (`*` indicator)
- **Live Status Bar** — Real-time line number, column position, and word count updated via timer
- **Menu Bar** — Organized File / Edit / Search / About menus with accelerator keys
- **Clean Resize** — Editor and status bar resize proportionally with the window

## Screenshot

```
+---------------------------------------------------+
| File   Edit   Search   About                      |
+---------------------------------------------------+
|                                                   |
|  The quick brown fox jumps over the lazy dog.     |
|  Lorem ipsum dolor sit amet, consectetur          |
|  adipiscing elit. Sed do eiusmod tempor.          |
|                                                   |
+---------------------------------------------------+
| Ln 2, Col 15  |  Words: 23                        |
+---------------------------------------------------+
```

## Project Structure

```
fltk-text-editor/
├── src/
│   └── editor.cpp        # Complete editor implementation (~310 lines)
├── CMakeLists.txt         # CMake build configuration
├── LICENSE
└── README.md
```

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| GUI Framework | FLTK 1.3+ |
| Build System | CMake 3.16+ |
| Platform | Cross-platform (Linux, macOS, Windows) |

## Build & Run

### Prerequisites

- **CMake 3.16+**
- **FLTK 1.3+** development libraries
- **C++17** compatible compiler

### Install FLTK

**Ubuntu / Debian:**
```bash
sudo apt install libfltk1.3-dev
```

**macOS (Homebrew):**
```bash
brew install fltk
```

**Windows (vcpkg):**
```powershell
vcpkg install fltk:x64-windows
```

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Run

```bash
./fltk_text_editor           # Linux / macOS
.\fltk_text_editor.exe       # Windows
```

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` | Save As |
| `Ctrl+Q` | Quit |
| `Ctrl+X` | Cut |
| `Ctrl+C` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+F` | Find |
| `Ctrl+H` | Find & Replace |

## Architecture

The editor is built as a single `EditorWindow` class extending `Fl_Double_Window`:

- **`Fl_Text_Buffer`** — Manages document content and modification state
- **`Fl_Text_Editor`** — Provides the text editing widget with syntax-aware rendering
- **`Fl_Menu_Bar`** — Handles menu items with callback routing
- **`Fl_Box`** — Status bar displaying cursor position and word count
- **Timer callback** — Updates the status bar every 200ms without blocking the UI

## License

MIT — see [LICENSE](LICENSE) for details.
