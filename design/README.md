# Reference UI

`demo.html` is the original HTML/CSS reference supplied by the NCREC project creator. The actual in-game editor will be built with C++ and Dear ImGui, **not an embedded browser**. The HTML's CDN links are design-time resources only; nothing is fetched by NCREC at runtime.

The production target is a **fixed fullscreen application shell**: 36px top bar, 260px Project/Shots sidebar, centered preview + 44px transport, 320px inspector, 240px timeline, 26px status. Sidebars collapse at narrow resolutions. Only content panes with naturally long content scroll; the application root does not.
