from __future__ import annotations

import shutil
from pathlib import Path


PROJECT_CREATOR_DIR = Path(__file__).resolve().parent
WORKSPACE_ROOT = PROJECT_CREATOR_DIR.parent
TEMPLATE_DIR = PROJECT_CREATOR_DIR / "Template"
FONT_SOURCE = PROJECT_CREATOR_DIR / "JetBrainsMono-Bold.ttf"
ICONS_SOURCE = WORKSPACE_ROOT / "AnvilEngine" / "Resources" / "Icons"

PROJECT_FILE_TEMPLATE = "Project.anv"
TEXT_TEMPLATE_SUFFIXES = {
    ".anv",
    ".ascn",
    ".glsl",
    ".py",
    ".toml",
    ".json",
    ".txt",
    ".md",
}


def _replace_template_tokens(
    project_dir: Path,
    project_name: str,
) -> None:
    replacements = {
        "{{PROJECT_NAME}}": project_name,
        "{{PROJECT_DIR}}": project_dir.as_posix(),
    }

    for path in project_dir.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in TEXT_TEMPLATE_SUFFIXES:
            continue

        content = path.read_text(encoding="utf-8")
        for token, value in replacements.items():
            content = content.replace(token, value)
        path.write_text(content, encoding="utf-8")


def create_project_from_template(
    project_path: Path,
    project_name: str,
) -> Path:
    project_dir = Path(project_path).expanduser().resolve()

    if project_dir.exists():
        raise FileExistsError(f"Project directory already exists: {project_dir}")

    if not TEMPLATE_DIR.is_dir():
        raise FileNotFoundError(f"Project template was not found: {TEMPLATE_DIR}")

    shutil.copytree(TEMPLATE_DIR, project_dir)

    template_project_file = project_dir / PROJECT_FILE_TEMPLATE
    project_file = project_dir / f"{project_name}.anv"

    if not template_project_file.is_file():
        shutil.rmtree(project_dir, ignore_errors=True)
        raise FileNotFoundError(
            f"Project file template was not found: {template_project_file}"
        )

    template_project_file.rename(project_file)

    fonts_dir = project_dir / "Assets" / "Fonts"
    fonts_dir.mkdir(parents=True, exist_ok=True)
    if FONT_SOURCE.is_file():
        shutil.copy2(FONT_SOURCE, fonts_dir / FONT_SOURCE.name)
    else:
        shutil.rmtree(project_dir, ignore_errors=True)
        raise FileNotFoundError(f"Default font was not found: {FONT_SOURCE}")

    icons_destination = (
        project_dir / "Assets" / "com.anvstu.engine" / "Icons"
    )
    if ICONS_SOURCE.is_dir():
        shutil.copytree(ICONS_SOURCE, icons_destination, dirs_exist_ok=True)
    else:
        shutil.rmtree(project_dir, ignore_errors=True)
        raise FileNotFoundError(f"Engine icons were not found: {ICONS_SOURCE}")

    _replace_template_tokens(project_dir, project_name)
    return project_file
