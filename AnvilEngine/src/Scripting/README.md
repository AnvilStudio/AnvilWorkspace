# Anvil Python Scripting

Anvil embeds CPython when the engine is built with `ANV_ENABLE_PYTHON`.
Scripts are attached to entities through `Component::Script`; there is no global
`main.py` callback. Each scripted entity owns an independent Python object.

## Project layout

```text
MyProject/
├── MyProject.anv
├── Assets/
└── Scripts/
    └── PlayerController.py
```

## Writing a script

```python
import anvil


class PlayerController(anvil.Script):
    speed: float = 5.0
    player_name: str = "Player"
    enabled_message: bool = True

    def on_create(self):
        if self.enabled_message:
            anvil.log(f"Created {self.player_name}")

    def on_update(self, delta_time: float):
        x, y = self.position
        self.position = (x + self.speed * delta_time, y)

    def on_destroy(self):
        anvil.log(f"Destroyed {self.player_name}")
```

The class must derive from `anvil.Script`. The runtime creates one instance for
each entity that references the module and class.

## Attaching a script

In C++:

```cpp
auto entity = scene.CreateEntity("Player");
auto& script = scene.AddComponent<anv::Component::Script>(entity);
script.modulePath = "PlayerController.py";
script.className = "PlayerController";
```

The equivalent serialized scene component is:

```toml
[Entities."entity-uuid".Script]
Module = "PlayerController.py"
Class = "PlayerController"
Enabled = true

[Entities."entity-uuid".Script.Fields.speed]
Type = "Float"
Value = "5.0"
```

## Lifecycle

- `on_create()` runs after the Python object is constructed and reflected fields
  are restored.
- `on_update(delta_time)` runs from `Scene::OnUpdate` while the component is
  enabled.
- `on_destroy()` runs before entity destruction, scene shutdown, hot reload, or
  interpreter shutdown.

Missing lifecycle methods are inherited as no-ops from `anvil.Script`.
Exceptions are captured and printed through Anvil's logger with a Python
traceback.

## Reflected fields

Class annotations expose editable and serializable fields. The initial runtime
supports:

- `bool`
- `int`
- `float`
- `str`

Defaults are read from the Python class the first time the script is loaded.
Values stored in `Component::Script::fields` override the defaults and survive
scene saves and module reloads.

## Entity API

The initial native entity binding exposes the entity's `Transform2d`:

```python
x, y = self.position
self.position = (10.0, 4.0)
self.rotation = 45.0
```

Logging is also available:

```python
anvil.log("Information")
anvil.warn("Warning")
anvil.error("Error")
```

Component-specific proxy types can be added to the native `_anvil` module
without changing the script lifecycle or serialization model.

## Hot reload

Anvil checks loaded Python files during scene updates. When a file timestamp
changes, it:

1. saves reflected instance fields,
2. calls `on_destroy()`,
3. loads a fresh module generation,
4. reconstructs each entity instance,
5. restores reflected fields, and
6. calls `on_create()`.

A reload can also be requested from C++:

```cpp
anv::PythonScriptEngine::RequestReload("PlayerController.py");
```

## Build requirements on macOS

The selected Python installation must provide its development headers and embed
linker configuration:

```sh
python3-config --includes
python3-config --embed --ldflags
```

Regenerate project files after changing the Premake configuration. When
`python3-config` is unavailable, the scripting subsystem builds as a safe no-op
implementation and logs that Python support is disabled.

## Threading

The CPython runtime and all entity bindings currently execute on Anvil's main
thread. Worker jobs may prepare data for scripts, but must not call Python or
access `PyObject*` values directly.
