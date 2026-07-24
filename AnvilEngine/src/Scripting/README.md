# Anvil Python Scripting

Anvil embeds CPython when the engine is built with `ANV_ENABLE_PYTHON`.
On macOS, Premake obtains the compiler and linker flags from the active
`python3-config` installation.

## Project layout

Create a `Scripts` directory in the project root and add `main.py`:

```text
MyProject/
├── MyProject.anv
├── Assets/
└── Scripts/
    └── main.py
```

Opening a project does not execute arbitrary scripts during project parsing.
Forge starts the Python runtime from its application setup lifecycle.

## Script lifecycle

`Scripts/main.py` may define any of these optional functions:

```python
import anvil


def on_create():
    anvil.log("Python gameplay initialized")


def on_update(delta_time: float):
    # Called once per frame.
    pass


def on_destroy():
    anvil.log("Python gameplay shutting down")
```

Missing callbacks are ignored. An exception is captured by the engine and
reported through Anvil's logger instead of crossing the C++ boundary.

## Native API

The first milestone exposes three logging functions:

```python
anvil.log("Information")
anvil.warn("Warning")
anvil.error("Error")
```

The public module name is `anvil`. Its current implementation is backed by the
native `_anvil` module registered by `PythonScriptEngine`.

## Build requirements on macOS

The selected `python3` installation must provide development headers and the
embed linker configuration:

```sh
python3-config --includes
python3-config --embed --ldflags
```

After changing Premake configuration, regenerate the project files before
building. If `python3-config` is unavailable, Anvil compiles the scripting
subsystem as a no-op implementation and logs that scripting is disabled.

## Current scope

This milestone provides interpreter ownership, project module loading,
lifecycle callbacks, logging, and exception containment. Entity handles,
component proxies, reflected Python fields, hot reload, and packaged runtime
distribution remain future milestones.
