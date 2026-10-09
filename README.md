# recomp-launcher

A skinnable Dear ImGui launcher and in-game menu for psxrecomp games, starting
with R4: Ridge Racer Type 4. Drop-in for recomp-ui: point a game at it with
`-DRECOMP_UI_ROOT=<this repo>`.

- Design and ABI inventory: [docs/DESIGN.md](docs/DESIGN.md)
- Skin format: [docs/SKIN_SCHEMA.md](docs/SKIN_SCHEMA.md)

```sh
git submodule update --init
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DR4L_RECOMP_UI_REFERENCE=<recomp-ui checkout>   # optional layout test
ninja -C build && ctest --test-dir build
build/r4l-skin-render --shots out --assets assets --skin r4 --size 1280x800
tools/skin_diff.py out/home.png reference.png -o diff.png
```

MIT licensed (`LICENSE`). Third-party components and their licenses:
`THIRD-PARTY-NOTICES.md`.
