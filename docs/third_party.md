# Native renderer prototype — third-party code

The experimental native DX12 renderer currently includes adapted source
from **dx12-imgui-overlay** by Maxim Goncharenko:

- Project: https://github.com/kacejot/dx12-imgui-overlay
- Source revision: `e5087b986215d5c2092313458c68779eb4d99304`
- License: MIT (see `src/platform/dx12/LICENSE`)
- Vendored files: `overlay.cpp`, `overlay.h`, `log.h`.
- NCREC modifications: add explicit show/hide interface, intercept UI input,
  render directly into Cyberpunk's DX12 swapchain; adapt API integration.

Dependency builds also fetch pinned Dear ImGui and MinHook source revisions.
Do not remove source attribution/license notices. Both are independently
licensed and carry their own upstream license files.

**Risk:** DXGI function/vtable hooking may collide with CET, ReShade or other
overlay injectors. This is an experimental proof of concept. The upstream
renderer is not a compatibility guarantee, and the hook may need major
rework for lifecycle, synchronization, and API coexistence.
