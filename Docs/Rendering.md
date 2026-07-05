# Rendering

**Status:** Working on OpenGL 4.5 — full RHI redesign + Vulkan backend is Phase 3 (RAD-31…RAD-42). This document describes the current system and the redesign's shape; the Architecture section will be rewritten as RHI v2 lands.

## The Problem This Solves

The GPU is effectively a second computer with its own memory and its own timeline; "drawing" means the CPU preparing descriptions of work and shipping them across. Two problems shape every renderer ever written:

1. **Shipments are expensive.** Each draw call carries fixed overhead regardless of size — telling the GPU about sprites one at a time collapses at a few thousand of them. The answer is **batching**: gather thousands of quads into one big vertex buffer on the CPU, ship it once, draw it once. It's the difference between one weekly grocery run and a separate trip per item — `Renderer2D` exists to make draw-call count independent of sprite count.
2. **The GPU's language varies.** OpenGL, Vulkan, and DirectX are different APIs for the same hardware, and game code shouldn't care which one is underneath. The **RHI** (Render Hardware Interface) is the engine-facing vocabulary — buffers, textures, shaders, draws — with a per-API backend translating it. How well that vocabulary is chosen decides whether swapping APIs is a backend job or a rewrite; Radiant's current one is GL-shaped, which is exactly Phase 3's problem to fix.

## Architecture (current)

### Layering

```text
Game / Level::OnRender
  └─ Renderer2D            batched 2D drawing (quads, lines, rects, sprites)
       └─ RenderCommand    static facade over one global backend instance
            └─ RendererAPI abstract backend interface (Init/Clear/DrawIndexed/…)
                 └─ OpenGLRendererAPI   immediate GL calls
```

- `RendererAPI` is the abstract backend; a static enum (`API::OpenGL`, hardcoded) selects the implementation via `RendererAPI::Create()`.
- `RenderCommand` holds a single global `Scope<RendererAPI>` and inline-forwards every call — the entire dispatch is one virtual call into a global backend. There is **no command buffer and no render thread**; every call executes GL immediately on the calling thread.
- Resource abstractions (`VertexBuffer`, `IndexBuffer`, `VertexArray`, `Shader`, `Texture2D`, `Framebuffer`, `UniformBuffer`, `GraphicsContext`) each expose a static `Create()` factory switching on the active API and returning a `Ref<OpenGL...>` concrete type.

### Renderer2D batching

All 2D drawing funnels through one CPU-side batcher (`Renderer2D.cpp`, file-static `Renderer2DData`):

- **Limits:** 20,000 quads / 80,000 vertices / 120,000 indices per batch; 32 texture slots (slot 0 = built-in 1×1 white texture for untextured quads).
- **Vertex layout (quads):** position (vec3), color (vec4), texcoord (vec2), texture index (float), tiling factor (float). The quad index buffer is generated once (`0,1,2, 2,3,0` pattern) and never changes.
- **Flow:** `BeginScene(camera, transform)` computes the view-projection and uploads it to a UBO at binding 0 → `DrawQuad`/`DrawLine`/`DrawSprite` append vertices via bump pointer, resolving each texture to a slot (linear search; new texture → next slot; slots full → flush and start a new batch) → `EndScene`/`Flush` uploads the used byte range with `glBufferSubData`, binds the textures and shader, and issues one draw per primitive family.
- Shaders are GLSL 450 with a `std140, binding = 0` camera UBO; the quad fragment shader indexes a `sampler2D[32]` array.

Reaper renders the Level into an **offscreen framebuffer** (RGBA8 color + RED_INTEGER entity-ID attachment + depth) and displays it as an ImGui image — the entity-ID attachment is pre-built plumbing for editor click-picking (RAD-54).

## Design Rationale

- **Batching exists to make draw-call count independent of sprite count** — thousands of quads become a handful of draws. The bump-pointer CPU array + single upload is the standard 2D batching shape and survives into the Vulkan renderer (with per-frame-in-flight buffer rings replacing single-buffer reuse — playbook §5).
- **The abstraction is GL-shaped, and that is the core problem.** `VertexArray` is a GL-only concept promoted to an engine type; `Bind()`-to-use mirrors GL's global state machine; uniforms are set by string name (`glGetUniformLocation` per call); shaders compile from GLSL text at runtime; draws execute immediately against global state. None of this maps to Vulkan — which is why Phase 3 is a **redesign of the RHI layer**, not a backend swap.
- **Why design the new RHI Vulkan-shaped:** a command-buffer/pipeline/descriptor model can trivially host a GL backend, but a GL-shaped interface cannot host Vulkan. The design study references UE's `FRHICommandList` / `IRHICommandContext` / `FGraphicsPipelineStateInitializer` (RAD-31).

## Known Issues & Evolution

- **Phase 3 progression (RAD-31…42):** RHI v2 design doc → Vulkan instance/device → swapchain + frames-in-flight → VMA resources → shaderc GLSL→SPIR-V with reflection → first triangle → pipeline/descriptor abstractions → Renderer2D parity (plus stats and circle rendering, absent today) → ImGui Vulkan backend → **delete the GL backend and `VertexArray`**.
- **Implicit-sync hazard:** the batch VBO is reused and re-uploaded every flush with no orphaning/ring — a classic driver stall source; fixed structurally by frames-in-flight rings in Phase 3.
- **Crash-class bugs: resolved.** RAD-10 (Shutdown double-free — batch base arrays are now `Scope<T[]>`-owned, cursors raw non-owning by convention), RAD-14 (`SetData` per-call leak — uploads read the caller's buffer directly), and RAD-15 (`DrawLine` overflow — capacity-guarded like the quad path) all landed 2026-07-05.
- **Legacy remnants:** `Renderer::Submit` (per-object, string-name uniforms) is incompatible with the 450-core shaders and dies with the GL backend; engine shaders currently live inside Reaper's asset folder — game-owned engine resources, moved engine-side in Phase 4 (RAD-48). `Renderer2D_Text.glsl` sits there orphaned, preserved for the Phase 4 text revival.
