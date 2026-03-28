# Prismo

A declarative scene description language and real-time renderer built from scratch in C with OpenGL.

Write human-readable `.scene` files describing 2D and 3D objects, and Prismo parses your code through a hand-written lexer and recursive descent parser, builds an AST, and renders the scene live in an OpenGL window. Edit your scene file, press R, and see changes instantly — no recompilation needed.

---

## Table of Contents

- [Quick Start](#quick-start)
- [How It Works](#how-it-works)
- [Workflow](#workflow)
- [Controls](#controls)
- [Language Reference](#language-reference)
  - [Scene Setup](#scene-setup)
  - [Comments](#comments)
  - [2D Primitives](#2d-primitives)
  - [3D Primitives](#3d-primitives)
  - [Common Properties](#common-properties)
  - [Grouping](#grouping)
  - [Lighting](#lighting)
- [Full Example](#full-example)
- [Architecture](#architecture)
  - [Pipeline Overview](#pipeline-overview)
  - [Source Files](#source-files)
  - [Lexer](#lexer)
  - [Parser](#parser)
  - [AST](#ast)
  - [Renderer](#renderer)
- [Building From Source](#building-from-source)
- [Dependencies](#dependencies)
- [Platform](#platform)
- [What's Implemented](#whats-implemented)
- [What's Not Yet Implemented](#whats-not-yet-implemented)
- [License](#license)

---

## Quick Start

```bash
brew install glfw
make
./prismo examples/demo.scene
```

A window opens with your rendered scene. Press `R` to hot-reload after editing the file.

---

## How It Works

Prismo is a complete compiler frontend + graphics backend in ~1500 lines of C:

```
your_scene.scene  →  Lexer  →  Tokens  →  Parser  →  AST  →  Renderer  →  OpenGL Window
```

1. You write a `.scene` file using the Prismo DSL
2. The **lexer** (tokenizer) breaks your source into tokens — keywords, numbers, hex colors, strings, braces
3. The **parser** (recursive descent) consumes those tokens and builds an **AST** (Abstract Syntax Tree) — a tree of nodes representing your scene
4. The **renderer** walks the AST and translates each node into OpenGL draw calls — setting up shaders, matrices, and geometry
5. GLFW provides the window and event loop
6. When you press R, the file is re-read, re-parsed, and re-rendered — the entire pipeline runs again from scratch

There is no intermediate bytecode, no VM, no scripting engine. It's a straight shot from text to pixels.

---

## Workflow

Open two terminals side by side (or use Emacs with a split):

```bash
# Terminal 1 — renderer (leave this running)
./prismo examples/demo.scene

# Terminal 2 — editor
emacs -nw examples/demo.scene
```

Edit the scene code → save (`C-x C-s` in Emacs) → click the renderer window → press `R` → scene updates live.

You can also create new scene files from scratch:

```bash
emacs -nw examples/myworld.scene
# write your scene code...
./prismo examples/myworld.scene
```

---

## Controls

| Key   | Action                                         |
|-------|-------------------------------------------------|
| `ESC` | Quit the renderer                               |
| `R`   | Hot-reload the scene file from disk              |

---

## Language Reference

### Scene Setup

These are top-level commands that configure the window and camera. They go at the top of your file, outside any braces.

```
canvas <width> <height> "<title>"
```
Sets the window dimensions and title bar text. Default is `800 600 "Prismo"`.

```
background <hex_color>
```
Sets the background clear color. Default is `#0d0d1a`.

```
camera <x> <y> <z>
```
Sets the camera position. The camera always looks toward the origin `(0, 0, 0)` with Y-up. Default is `0 2 -8`.

**Example:**
```
canvas 1200 800 "My Scene"
background #1a1a2e
camera 0 5 -12
```

---

### Comments

Two comment styles are supported:

```
// This is a comment (C-style double slash)
# This is also a comment (only when not followed by hex digits)
```

Comments run to the end of the line. `#ff3344` is parsed as a hex color, not a comment.

---

### 2D Primitives

2D shapes are rendered as flat overlays on top of the 3D scene, using pixel coordinates. Origin `(0, 0)` is the top-left corner. X increases rightward, Y increases downward.

#### circle
```
circle {
    pos <x> <y>
    radius <r>
    color <hex>
}
```
Draws a filled circle centered at `(x, y)` with the given radius in pixels.

#### rect
```
rect {
    pos <x> <y>
    size <width> <height>
    color <hex>
}
```
Draws a filled rectangle. `pos` is the top-left corner.

#### line
```
line {
    from <x1> <y1>
    to <x2> <y2>
    color <hex>
    width <w>
}
```
Draws a line between two points. `width` is the line thickness (default: 1).

#### triangle
```
triangle {
    p1 <x1> <y1>
    p2 <x2> <y2>
    p3 <x3> <y3>
    color <hex>
}
```
Draws a filled triangle from three 2D points.

**2D Example:**
```
circle { pos 400 300  radius 80  color #ff3344 }
rect { pos 10 10  size 200 40  color #333366 }
line { from 0 50  to 200 50  color #4488ff  width 3 }
triangle { p1 100 500  p2 150 440  p3 200 500  color #22cc88 }
```

---

### 3D Primitives

3D shapes are rendered in world space using a perspective camera. Origin `(0, 0, 0)` is the center of the world. Y is up, Z comes toward the camera (in the default camera setup).

#### cube
```
cube {
    pos <x> <y> <z>
    size <w> <h> <d>
    color <hex>
    rotate <rx> <ry> <rz>
}
```
A box centered at `pos`. `size` takes 1, 2, or 3 values — if you pass one value (e.g. `size 2`), it creates a uniform cube.

#### sphere
```
sphere {
    pos <x> <y> <z>
    radius <r>
    color <hex>
}
```
A UV sphere centered at `pos`. Generated with 20 stacks × 20 sectors.

#### pyramid
```
pyramid {
    pos <x> <y> <z>
    base <b>
    height <h>
    color <hex>
}
```
A four-sided pyramid. `base` is the width of the square base, `height` is the apex height. The base sits at the `pos` Y coordinate.

#### plane
```
plane {
    pos <x> <y> <z>
    size <w> <d>
    color <hex>
}
```
A flat horizontal surface (quad) at the given Y position. Useful for ground planes.

**3D Example:**
```
camera 0 3 -10
light { type directional  pos 5 10 5  color #ffffff  intensity 1.0 }

plane { pos 0 -2 0  size 20 20  color #1a1a2e }
cube { pos 0 0 0  size 2 2 2  color #ff3344  rotate 0 45 0 }
sphere { pos 3 0 0  radius 1.5  color #44aaff }
pyramid { pos -3 0 0  base 2  height 3  color #22cc88 }
```

---

### Common Properties

These properties work on any shape (2D or 3D):

| Property    | Syntax                    | Description                                    | Default     |
|-------------|---------------------------|------------------------------------------------|-------------|
| `pos`       | `pos <x> <y> [z]`        | Position. Z is optional for 2D shapes.         | `0 0 0`     |
| `color`     | `color <#rrggbb>`         | Hex color (6 digits, no alpha).                | `#ffffff`   |
| `rotate`    | `rotate <rx> [ry] [rz]`  | Rotation in degrees around X, Y, Z axes.       | `0 0 0`     |
| `scale`     | `scale <sx> [sy] [sz]`   | Scale multiplier on each axis.                 | `1 1 1`     |
| `opacity`   | `opacity <0.0-1.0>`      | Transparency. 0 = invisible, 1 = fully opaque. | `1.0`       |
| `fill`      | `fill solid\|wireframe`  | Render as filled geometry or wireframe edges.  | `solid`     |

**Note:** Optional parameters (in brackets) use `0` as default if omitted for rotation, and copy the first value for scale. So `rotate 45` means `rotate 45 0 0`, and `size 2` on a cube means `size 2 2 2`.

**Wireframe Example:**
```
sphere { pos 0 0 0  radius 2  color #ffffff  fill wireframe }
```

---

### Grouping

Groups let you nest shapes under a shared parent transform. The group's `pos` offsets all children.

```
group "<name>" {
    pos <x> <y> <z>

    cube { pos 0 0 0  size 1 1 1  color #ff0000 }
    sphere { pos 0 2 0  radius 0.5  color #00ff00 }
}
```

The name string is optional (but useful for readability). Child positions are relative to the group's position. In the example above, if the group is at `pos 5 0 0`, the cube ends up at `(5, 0, 0)` and the sphere at `(5, 2, 0)`.

Groups can contain any shapes: 2D, 3D, lights, or even nested groups.

---

### Lighting

Lights affect how 3D shapes are shaded. They have no effect on 2D shapes.

#### Directional Light
```
light {
    type directional
    pos <x> <y> <z>
    color <hex>
    intensity <0.0-1.0>
}
```
A sun-like light. `pos` defines the direction the light comes FROM (not where it shines TO). So `pos 5 10 5` means light comes from upper-right-front.

#### Ambient Light
```
light {
    type ambient
    color <hex>
    intensity <0.0-1.0>
}
```
Uniform light that fills all shadows. Without ambient light, faces pointing away from the directional light are completely black. A typical ambient intensity is `0.2` to `0.3`.

#### Point Light
```
light {
    type point
    pos <x> <y> <z>
    color <hex>
    intensity <0.0-1.0>
}
```
A light at a specific position in the scene. Currently treated the same as directional (attenuation not yet implemented).

**Typical Lighting Setup:**
```
light { type directional  pos 5 10 5  color #ffffff  intensity 1.0 }
light { type ambient  color #222244  intensity 0.25 }
```

---

## Full Example

```
// demo.scene — a complete Prismo scene

canvas 900 650 "Prismo Demo"
background #0a0a1a

camera 0 3 -10

// Lighting
light { type directional  pos 5 10 5  color #ffffff  intensity 1.0 }
light { type ambient  color #222244  intensity 0.3 }

// Ground
plane { pos 0 -2 0  size 20 20  color #1a1a2e }

// Stacked shapes
group "tower" {
    pos 0 0 0
    cube { pos 0 -1 0  size 2 2 2  color #ff3344 }
    cube { pos 0 1 0  size 1.5 1.5 1.5  color #44aaff  rotate 45 0 0 }
    sphere { pos 0 2.5 0  radius 0.8  color #ffcc00 }
}

// Side decorations
sphere { pos -4 -1 0  radius 1.0  color #22cc88 }
sphere { pos 4 -1 0  radius 1.0  color #aa44ff }
pyramid { pos -4 -1 4  base 2  height 3  color #ff6600 }

// Wireframe
sphere { pos 0 -1 5  radius 1.5  color #ffffff  fill wireframe }

// 2D overlay
circle { pos 820 50  radius 30  color #ffcc00 }
rect { pos 10 10  size 160 30  color #333355 }
triangle { p1 50 600  p2 100 550  p3 150 600  color #22cc88 }
line { from 10 45  to 170 45  color #4488ff  width 2 }
```

---

## Architecture

### Pipeline Overview

```
  ┌──────────────┐     ┌────────────┐     ┌────────────┐     ┌──────────────┐
  │  .scene file │ ──▸ │   Lexer    │ ──▸ │   Parser   │ ──▸ │   Renderer   │
  │  (your code) │     │ (tokenize) │     │ (build AST)│     │ (OpenGL draw)│
  └──────────────┘     └────────────┘     └────────────┘     └──────────────┘
                                                                     │
                                                              ┌──────▼──────┐
                                                              │ GLFW Window │
                                                              │  (display)  │
                                                              └─────────────┘
```

On hot-reload (press R), the entire pipeline re-runs from file read to render. The old AST is freed and a new one is built from scratch.

### Source Files

```
prismo/
├── src/
│   ├── main.c           Entry point. GLFW window, file I/O, main loop, hot reload.
│   ├── lexer.h/c        Tokenizer. Converts source text into a stream of tokens.
│   ├── parser.h/c       Recursive descent parser. Consumes tokens, builds the AST.
│   ├── ast.h/c          AST node type definitions, memory management.
│   ├── renderer.h/c     AST walker. Compiles shaders, generates meshes, draws.
│   └── math_util.h      4x4 matrix math — perspective, lookat, rotate, translate.
├── examples/
│   └── demo.scene       Example scene file.
├── Makefile             Build script for macOS.
└── README.md            You are here.
```

### Lexer

The lexer (`src/lexer.c`) is a hand-written scanner that reads the source character by character and emits tokens. It handles:

- **Keywords:** `canvas`, `background`, `camera`, `circle`, `cube`, `sphere`, `pos`, `color`, `rotate`, etc. — matched from a static keyword table.
- **Numbers:** integers and floats, including negative values (`-3.14`).
- **Hex colors:** `#rrggbb` — the RGB values are parsed and stored as floats (0.0–1.0) directly on the token.
- **Strings:** double-quoted (`"hello world"`).
- **Braces:** `{` and `}`.
- **Comments:** `//` and `#` (when not followed by a hex digit).

The lexer is a single-pass scanner with no backtracking. It skips whitespace and comments automatically between tokens.

### Parser

The parser (`src/parser.c`) is a recursive descent parser — one function per grammar rule. The grammar looks roughly like:

```
scene      → (command | shape)*
command    → canvas | background | camera
shape      → (circle | rect | cube | sphere | ...) '{' property* '}'
           | group [string] '{' (property | shape)* '}'
           | light '{' property* '}'
property   → pos | color | rotate | scale | opacity | fill | radius | size | ...
```

Each shape parser calls `parse_common_prop()` for shared properties (pos, color, rotate, etc.) and handles shape-specific properties itself. This avoids code duplication across the 10+ shape types.

The parser builds an AST (Abstract Syntax Tree) where each node is a tagged union (`struct ASTNode`) — the `type` field tells you which shape it is, and the `data` union holds the shape-specific data.

### AST

The AST (`src/ast.h`) uses a tagged union pattern:

```c
struct ASTNode {
    NodeType type;       // NODE_CUBE, NODE_SPHERE, etc.
    union {
        CubeNode     cube;
        SphereNode   sphere;
        CircleNode   circle;
        GroupNode    group;
        LightNode    light;
        // ...
    } data;
};
```

Every shape node contains a `ShapeProps` struct with the common properties (pos, color, rotate, scale, opacity, fill), plus shape-specific fields (radius, size, base, height, etc.).

The root of the AST is a `Scene` struct containing a dynamic array of `ASTNode` pointers and the scene settings (canvas size, background color, camera position).

### Renderer

The renderer (`src/renderer.c`) does three things:

1. **Shader compilation** — two shader programs are compiled at init: a 3D shader (with lighting — diffuse + ambient) and a 2D shader (flat color, orthographic projection). Shader source is embedded as C string literals.

2. **Mesh generation** — at init, geometry is generated for each primitive type (cube, sphere, pyramid, plane, circle, quad). Vertex data includes positions and normals. Meshes are uploaded to GPU as VAO/VBO pairs.

3. **Scene drawing** — the renderer walks the AST node by node. For each node, it:
   - Builds a model matrix from the shape's properties (translate × rotate × scale)
   - Sets shader uniforms (model, view, projection, color, lighting)
   - Binds the appropriate VAO
   - Issues a `glDrawArrays` or `glDrawElements` call

3D shapes are drawn first with depth testing enabled. 2D shapes are drawn on top with depth testing disabled, using an orthographic projection that maps pixel coordinates.

---

## Building From Source

```bash
# Clone or extract the project
cd prismo

# Install GLFW
brew install glfw

# Build (uses Apple clang)
make

# Run
./prismo examples/demo.scene

# Clean build artifacts
make clean
```

The Makefile uses `pkg-config` to find GLFW, with a fallback to `/opt/homebrew` (Apple Silicon default).

---

## Dependencies

| Dependency | What it does                | Install                      |
|------------|-----------------------------|-----------------------------|
| GLFW       | Window creation, input      | `brew install glfw`         |
| OpenGL     | GPU rendering               | Ships with macOS            |
| Apple Clang| C compiler                  | Xcode Command Line Tools    |

No other external libraries. Matrix math, mesh generation, parsing, and lexing are all implemented from scratch.

---

## Platform

Built and tested on:

- macOS Tahoe (26.x) on Apple M4
- Apple Clang 17
- OpenGL 4.1 (Metal backend)

Should work on any macOS with Apple Silicon or Intel, as long as GLFW is installed. Linux support would require swapping `<OpenGL/gl3.h>` for GLAD/GLEW — not hard but not done yet.

---

## What's Implemented

- Hand-written lexer with keyword table, hex color parsing, two comment styles
- Recursive descent parser producing a typed AST
- 2D primitives: circle, rect, line, triangle
- 3D primitives: cube, sphere, pyramid, plane
- Directional and ambient lighting with diffuse shading
- Hex color support (#rrggbb)
- Per-shape transforms: position, rotation, scale
- Opacity and wireframe rendering
- Shape grouping with parent transforms
- Hot reload (press R to re-parse and re-render)
- Anti-aliased rendering (MSAA 4x)
- Retina/HiDPI display support

## What's Not Yet Implemented

- **Text rendering** — requires a font atlas (FreeType or stb_truetype)
- **Cylinder mesh** — mesh generation not yet written
- **Nested group transforms** — only one level of group offset currently works
- **Point light attenuation** — point lights currently behave like directional
- **Specular highlights** — only diffuse + ambient shading
- **Mouse camera control** — camera is static (set in the .scene file)
- **Animation** — no time-based properties yet
- **Error recovery** — parser stops on first error

---

## License

Do whatever you want with it.