# C++ Log Analyzer & Error Monitoring Tool

A portable C++17 command-line application for parsing application logs, analyzing operational health, searching/filtering entries, and exporting reports. It uses only the C++ standard library and CMake.

## Features

- Parses `.log` and `.txt` files using a strict, documented log format; malformed and unsupported-level lines are skipped safely.
- Counts DEBUG, INFO, WARNING, and ERROR entries and calculates warning/error percentages.
- Ranks repeated error messages, groups errors by module, and reports the top five errors.
- Searches case-insensitively by message or module; combines level, exact module, and inclusive timestamp filters.
- Exports human-readable text and CSV reports, neutralizing spreadsheet formulas and rendering control characters safely.
- Provides an interactive CLI and a realistic sample dataset.
- Includes CTest-based coverage of parsing, analysis, filtering, file errors, empty files, and reports.

## Technologies

- C++17 and the standard library (`vector`, `map`, `unordered_map`, algorithms, filesystem, streams, regular expressions)
- CMake 3.16+
- MinGW GCC/G++ on Windows or GCC/Clang on Linux

## Prerequisites

- CMake 3.16 or newer.
- A C++17-capable compiler.
- On Windows, MinGW-w64 GCC/G++ and `mingw32-make` available on `PATH`.
- On Linux, GCC or Clang and a supported build tool such as Make or Ninja.

The project has no third-party library dependencies.

## Project structure

```text
.
├── CMakeLists.txt
├── data/
│   └── sample.log
├── include/              # Public types and component interfaces
├── reports/              # Generated reports (ignored by Git)
├── src/                  # Implementations and CLI entry point
└── tests/                # Standalone unit/integration test executable
```

| Component | Responsibility |
| --- | --- |
| `LogEntry.h` | Log level and parsed-entry value types |
| `FileManager` | Reads supported input files and writes report files |
| `LogParser` / `Timestamp` | Validates and parses log lines and timestamps |
| `LogAnalyzer` | Counts, ranks, groups, searches, and filters entries |
| `ReportGenerator` | Produces text and CSV reports |
| `TextSafety.h` | Renders control characters safely in displayed/exported text |
| `main.cpp` | Interactive CLI composition and user-facing errors |
| `LogAnalyzerTests.cpp` | Parser, analysis, file I/O, filtering, and report tests |

The CLI composes the components and reports errors without terminating the menu loop.

## Log format

Each valid line follows this pattern:

```text
[YYYY-MM-DD HH:MM:SS] [LEVEL] [Module] Message text
```

For example:

```text
[2025-03-08 09:00:06] [ERROR] [Database] Connection timeout while querying users
```

Supported levels are `DEBUG`, `INFO`, `WARNING`, and `ERROR`. Timestamps must be real calendar dates and times in the fixed-width format above; filtering uses inclusive lexical timestamp bounds and rejects invalid or reversed ranges. Empty lines and malformed lines are ignored; non-empty malformed lines are counted and shown after loading.

## Build and test

### Windows (MinGW)

From the workspace root in PowerShell:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

The MinGW `bin` directory must be on `PATH`; CMake will use its `mingw32-make` implementation for the requested generator.

### Linux

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`ctest` runs the automated test target. The tests cover valid and malformed lines, calendar and CRLF handling, counts and percentages, error ranking/grouping, keyword and timestamp filters, missing/empty files, and text/CSV report output.

## Run

From the project root:

```powershell
.\build\log-analyzer.exe
```

Or on Linux:

```bash
./build/log-analyzer
```

Choose **Load a log file** and enter `data/sample.log` (or its absolute path). Use the menu to display statistics, search by keyword, combine level/module/date filters, inspect frequent errors, or export `reports/analysis_report.txt` and `reports/analysis_report.csv`. Reports are created on demand, and the output directory is created if needed.

## Example summary

```text
Summary (19 valid entries)
DEBUG: 3  INFO: 6  WARNING: 3  ERROR: 7
Warning rate: 15.79%  Error rate: 36.84%
Malformed lines ignored: 2
```

The exact sample totals reflect the checked-in sample file.

## Concepts demonstrated

- Encapsulation through component classes and value types
- STL containers and algorithms for counting, sorting, searching, and grouping
- File streams and filesystem paths for portable I/O
- `std::optional` for parsing/filter criteria and explicit exception reporting for file errors
- Deterministic sorting, case-insensitive text matching, CSV escaping, and spreadsheet-formula neutralization
- Separation of CLI, parsing, analysis, and report-generation responsibilities

## Future improvements

- Support configurable timestamp formats and time zones.
- Add JSON output and configurable report paths.
- Add streaming analysis for very large log files.
- Add richer date parsing and regular-expression search options.
