from pathlib import Path
import sys
import shutil

def write(path: Path, text: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")

def cpy_file(src: Path, dst: Path):
    shutil.copy(src, dst)

def create_project(name: str, root: str):
    project_dir = Path(root).resolve()
    project_dir = project_dir / name # Create a subdirectory for the project
    assets = project_dir / "Assets"
    engine = assets / "com.anvstu.engine"

    # TODO: need to copy/paste font into font dir for imgui to use
    dirs = [
        assets / "Scenes",
        assets / "Fonts",
        engine / "ShaderLib",
        engine / "Settings"
    ]

    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)

    cpy_file(Path("./JetBrainsMono-Bold.ttf"), assets / "Fonts")

    write(project_dir / f"{name}.anv", f"""[Settings]
Description = 'Anvil Project'
ProjDir = '{project_dir.as_posix()}'
ProjName = '{name}'
Version = '0.0.0'

[Settings.StartScene]
Name = 'Default'
Path = '@Assets/Scenes/Default.ascn'

[Settings.WindowInfo]
Height = 720
Width = 1280

[Settings.Directories]
Assets = 'Assets'
EngineRes = '@Assets/com.anvstu.engine'
Cache = '@Res/Cache'
AssetMeta = '@Res/AssetMeta'
Settings = '@Res/Settings'
""")

    write(assets / "Scenes" / "Default.ascn", """[Scene]
Context = '2D'
Name = 'Default'
Path = '@Assets/Scenes/Default.ascn'
UUID = ''
""")

    write(engine / "ShaderLib" / "sprite.glsl", """#type vert
#version 450

layout(push_constant)
uniform PushData
{
    mat4 model;
    vec4 color;
} push_data;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProjection;
} camera;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	gl_Position = camera.ViewProjection * 
    push_data.model * 
    vec4(inPosition, 0.0, 1.0);

	fragColor = push_data.color;
}
 
#type frag
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 fragColor;

void main() {
    outColor = fragColor;
}
""")
    


    print(f"Created Anvil project: {project_dir}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python create_project.py <ProjectName> <ProjectDir>")
        sys.exit(1)

    create_project(sys.argv[1], sys.argv[2])