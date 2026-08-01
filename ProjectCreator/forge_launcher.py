#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import platform
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk

from project_creator import create_project_from_template


APP_NAME = "Forge Launcher"
PROJECT_CREATOR_DIR = Path(__file__).resolve().parent
CONFIG_DIR = PROJECT_CREATOR_DIR / ".anvil"
CONFIG_PATH = CONFIG_DIR / "forge_launcher.json"


@dataclass
class Project:
    name: str
    path: str

    @property
    def project_path(self) -> Path:
        return Path(self.path).expanduser().resolve()


@dataclass
class LauncherConfig:
    forge_executable: str = ""
    projects_root: str = str(Path.home() / "AnvilProjects")
    projects: list[dict] | None = None

    def __post_init__(self) -> None:
        if self.projects is None:
            self.projects = []


class ForgeLauncher(tk.Tk):
    def __init__(self) -> None:
        super().__init__()

        self.title(APP_NAME)
        self.geometry("760x480")
        self.minsize(680, 420)

        self.config_data = self.load_config()
        self.projects = [
            Project(**project)
            for project in self.config_data.projects or []
        ]

        self.protocol("WM_DELETE_WINDOW", self.on_close)

        self.create_styles()
        self.create_ui()
        self.refresh_project_list()

    def create_styles(self) -> None:
        style = ttk.Style(self)

        try:
            if platform.system() == "Darwin":
                style.theme_use("aqua")
            elif "vista" in style.theme_names():
                style.theme_use("vista")
            else:
                style.theme_use("clam")
        except tk.TclError:
            pass

        style.configure("Title.TLabel", font=("TkDefaultFont", 18, "bold"))
        style.configure("Subtitle.TLabel", font=("TkDefaultFont", 10))
        style.configure("Primary.TButton", font=("TkDefaultFont", 11, "bold"))

    def create_ui(self) -> None:
        outer = ttk.Frame(self, padding=18)
        outer.pack(fill=tk.BOTH, expand=True)

        header = ttk.Frame(outer)
        header.pack(fill=tk.X)

        ttk.Label(
            header,
            text="Forge Launcher",
            style="Title.TLabel",
        ).pack(side=tk.LEFT)

        ttk.Button(
            header,
            text="Settings",
            command=self.open_settings,
        ).pack(side=tk.RIGHT)

        ttk.Label(
            outer,
            text="Create, manage, and launch Anvil projects.",
            style="Subtitle.TLabel",
        ).pack(anchor=tk.W, pady=(4, 16))

        content = ttk.Frame(outer)
        content.pack(fill=tk.BOTH, expand=True)

        list_frame = ttk.LabelFrame(content, text="Projects", padding=10)
        list_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.project_list = tk.Listbox(
            list_frame,
            activestyle="dotbox",
            exportselection=False,
            font=("TkDefaultFont", 11),
        )
        self.project_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.project_list.bind(
            "<Double-Button-1>",
            lambda _event: self.launch_selected(),
        )

        scrollbar = ttk.Scrollbar(
            list_frame,
            orient=tk.VERTICAL,
            command=self.project_list.yview,
        )
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.project_list.config(yscrollcommand=scrollbar.set)

        actions = ttk.Frame(content, padding=(14, 0, 0, 0))
        actions.pack(side=tk.RIGHT, fill=tk.Y)

        ttk.Button(
            actions,
            text="Launch Forge",
            style="Primary.TButton",
            command=self.launch_selected,
            width=20,
        ).pack(fill=tk.X, pady=(0, 10))

        ttk.Button(
            actions,
            text="Create Project",
            command=self.create_project,
            width=20,
        ).pack(fill=tk.X, pady=5)

        ttk.Button(
            actions,
            text="Open Project Folder",
            command=self.open_project_folder,
            width=20,
        ).pack(fill=tk.X, pady=5)

        ttk.Separator(actions).pack(fill=tk.X, pady=12)

        ttk.Button(
            actions,
            text="Remove From List",
            command=self.remove_selected,
            width=20,
        ).pack(fill=tk.X, pady=5)

        ttk.Button(
            actions,
            text="Refresh",
            command=self.refresh_project_list,
            width=20,
        ).pack(fill=tk.X, pady=5)

        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(
            outer,
            textvariable=self.status_var,
            relief=tk.SUNKEN,
            anchor=tk.W,
            padding=(8, 4),
        ).pack(fill=tk.X, pady=(14, 0))

    def load_config(self) -> LauncherConfig:
        CONFIG_DIR.mkdir(parents=True, exist_ok=True)

        if not CONFIG_PATH.exists():
            return LauncherConfig()

        try:
            data = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
            return LauncherConfig(
                forge_executable=data.get("forge_executable", ""),
                projects_root=data.get(
                    "projects_root",
                    str(Path.home() / "AnvilProjects"),
                ),
                projects=data.get("projects", []),
            )
        except (OSError, json.JSONDecodeError, TypeError):
            messagebox.showwarning(
                APP_NAME,
                f"Could not read config:\n{CONFIG_PATH}\n\nA new config will be used.",
            )
            return LauncherConfig()

    def save_config(self) -> None:
        CONFIG_DIR.mkdir(parents=True, exist_ok=True)
        self.config_data.projects = [asdict(project) for project in self.projects]
        CONFIG_PATH.write_text(
            json.dumps(asdict(self.config_data), indent=2),
            encoding="utf-8",
        )

    def refresh_project_list(self) -> None:
        selected_index = self.get_selected_index()
        self.project_list.delete(0, tk.END)

        for project in self.projects:
            exists_marker = "" if project.project_path.exists() else "  [missing]"
            self.project_list.insert(
                tk.END,
                f"{project.name}{exists_marker}\n    {project.project_path}",
            )

        if self.projects:
            index = selected_index if selected_index is not None else 0
            index = min(index, len(self.projects) - 1)
            self.project_list.selection_set(index)
            self.project_list.activate(index)

        self.status_var.set(f"{len(self.projects)} project(s)")

    def get_selected_index(self) -> int | None:
        selection = self.project_list.curselection()
        return selection[0] if selection else None

    def get_selected_project(self) -> Project | None:
        index = self.get_selected_index()
        if index is None:
            messagebox.showinfo(APP_NAME, "Select a project first.")
            return None
        return self.projects[index]

    def create_project(self) -> None:
        name = simpledialog.askstring(APP_NAME, "Project name:", parent=self)
        if name is None:
            return

        safe_name = "".join(
            character
            for character in name.strip()
            if character.isalnum() or character in (" ", "-", "_")
        ).strip()

        if not safe_name:
            messagebox.showerror(APP_NAME, "Project name contains no valid characters.")
            return

        projects_root = Path(
            self.config_data.projects_root
        ).expanduser().resolve()
        projects_root.mkdir(parents=True, exist_ok=True)
        project_path = projects_root / safe_name

        try:
            create_project_from_template(project_path, safe_name)
        except OSError as error:
            messagebox.showerror(APP_NAME, f"Could not create project:\n{error}")
            return

        self.projects.append(Project(name=safe_name, path=str(project_path)))
        self.save_config()
        self.refresh_project_list()

        index = len(self.projects) - 1
        self.project_list.selection_clear(0, tk.END)
        self.project_list.selection_set(index)
        self.project_list.activate(index)
        self.status_var.set(f"Created {safe_name}")

    def launch_selected(self) -> None:
        project = self.get_selected_project()
        if project is None:
            return

        if not project.project_path.exists():
            messagebox.showerror(
                APP_NAME,
                f"Project folder does not exist:\n{project.project_path}",
            )
            return

        executable_text = self.config_data.forge_executable.strip()
        if not executable_text:
            messagebox.showinfo(
                APP_NAME,
                "Set the Forge executable in Settings first.",
            )
            self.open_settings()
            return

        executable = Path(executable_text).expanduser().resolve()
        if not executable.exists():
            messagebox.showerror(
                APP_NAME,
                f"Forge executable was not found:\n{executable}",
            )
            return

        project_file = self.find_project_file(project.project_path)
        project_argument = project_file or project.project_path

        try:
            subprocess.Popen(
                [str(executable), "-prj", str(project_argument)],
                cwd=str(executable.parent),
                start_new_session=True,
            )
        except OSError as error:
            messagebox.showerror(APP_NAME, f"Could not launch Forge:\n{error}")
            return

        self.status_var.set(f"Launched {project.name}")

    @staticmethod
    def find_project_file(project_path: Path) -> Path | None:
        project_files = sorted(project_path.glob("*.anv"))
        return project_files[0] if project_files else None

    def open_project_folder(self) -> None:
        project = self.get_selected_project()
        if project is None:
            return

        path = project.project_path
        if not path.exists():
            messagebox.showerror(APP_NAME, f"Folder does not exist:\n{path}")
            return

        try:
            if sys.platform == "darwin":
                subprocess.Popen(["open", str(path)])
            elif os.name == "nt":
                os.startfile(path)  # type: ignore[attr-defined]
            else:
                subprocess.Popen(["xdg-open", str(path)])
        except OSError as error:
            messagebox.showerror(APP_NAME, f"Could not open project folder:\n{error}")

    def remove_selected(self) -> None:
        index = self.get_selected_index()
        if index is None:
            messagebox.showinfo(APP_NAME, "Select a project first.")
            return

        project = self.projects[index]
        confirmed = messagebox.askyesno(
            APP_NAME,
            f"Remove '{project.name}' from the launcher?\n\n"
            "The project files will not be deleted.",
        )
        if not confirmed:
            return

        del self.projects[index]
        self.save_config()
        self.refresh_project_list()
        self.status_var.set(f"Removed {project.name}")

    def open_settings(self) -> None:
        window = tk.Toplevel(self)
        window.title("Forge Launcher Settings")
        window.geometry("650x220")
        window.resizable(False, False)
        window.transient(self)
        window.grab_set()

        frame = ttk.Frame(window, padding=18)
        frame.pack(fill=tk.BOTH, expand=True)

        executable_var = tk.StringVar(value=self.config_data.forge_executable)
        projects_root_var = tk.StringVar(value=self.config_data.projects_root)

        ttk.Label(frame, text="Forge executable").grid(
            row=0,
            column=0,
            sticky=tk.W,
            pady=(0, 5),
        )
        executable_entry = ttk.Entry(
            frame,
            textvariable=executable_var,
            width=62,
        )
        executable_entry.grid(row=1, column=0, sticky=tk.EW, padx=(0, 8))

        def choose_executable() -> None:
            file_path = filedialog.askopenfilename(
                title="Select Forge executable",
                parent=window,
            )
            if file_path:
                executable_var.set(file_path)

        ttk.Button(
            frame,
            text="Browse",
            command=choose_executable,
        ).grid(row=1, column=1)

        ttk.Label(frame, text="Projects folder").grid(
            row=2,
            column=0,
            sticky=tk.W,
            pady=(18, 5),
        )
        ttk.Entry(
            frame,
            textvariable=projects_root_var,
            width=62,
        ).grid(row=3, column=0, sticky=tk.EW, padx=(0, 8))

        def choose_projects_root() -> None:
            directory = filedialog.askdirectory(
                title="Select projects folder",
                parent=window,
            )
            if directory:
                projects_root_var.set(directory)

        ttk.Button(
            frame,
            text="Browse",
            command=choose_projects_root,
        ).grid(row=3, column=1)

        button_row = ttk.Frame(frame)
        button_row.grid(
            row=4,
            column=0,
            columnspan=2,
            sticky=tk.E,
            pady=(22, 0),
        )

        def save_settings() -> None:
            self.config_data.forge_executable = executable_var.get().strip()
            self.config_data.projects_root = projects_root_var.get().strip()
            self.save_config()
            window.destroy()
            self.status_var.set("Settings saved")

        ttk.Button(
            button_row,
            text="Cancel",
            command=window.destroy,
        ).pack(side=tk.RIGHT, padx=(8, 0))
        ttk.Button(
            button_row,
            text="Save",
            command=save_settings,
        ).pack(side=tk.RIGHT)

        executable_entry.focus_set()

    def on_close(self) -> None:
        self.save_config()
        self.destroy()


def main() -> int:
    app = ForgeLauncher()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
