# Forge Launcher

A simple Python/Tkinter launcher for Anvil Forge.

## Features

- Create new Anvil project folders
- Generate a starter `.anvilproj` file
- Remember projects between launches
- Select the Forge executable
- Launch Forge with the selected project file/path
- Open a project folder in Finder, Explorer, or the Linux file manager

## Run

```bash
python3 forge_launcher.py
```

Tkinter ships with most Python installations.

On macOS, the python.org installer normally includes Tkinter. If your Homebrew
Python does not include it, install a Python build with Tk support.

## Configure

1. Open **Settings**.
2. Select the compiled Forge executable.
3. Select the default project directory.
4. Create a project.
5. Select it and click **Launch Forge**.

## Forge command-line expectation

The launcher starts Forge like this:

```text
Forge -prj /path/to/MyProject/MyProject.anv
```


