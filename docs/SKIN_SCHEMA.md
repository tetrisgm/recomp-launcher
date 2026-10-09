# Skin schema (`skin.json`)

A skin is a folder: `skin.json` plus any images, fonts and WAV files it names.
JSON with `//` and `/* */` comments and trailing commas allowed. Unknown keys
are ignored; every key is optional (built-in defaults fill gaps).

## Units and rectangles

All lengths are **reference units**: pixels on a screen `base_height` tall
(default 720). At 1080p one unit is 1.5 px. Horizontal room grows with the
aspect ratio, so anchor edges instead of hard-coding x for 4:3 / 16:9 / 16:10
/ Steam Deck (1280×800).

A length is one of:

| Form | Meaning |
|---|---|
| `120` | 120 reference units |
| `"50%"` / `"50vw"` | percent of the parent width (viewport for top-level rects) |
| `"100vh"` | percent of the viewport height |
| `"fill"` / `"fill-46"` | the rest of the parent after the offset, minus N units |

A rect:

```json
{ "anchor": "bottom-left", "x": 16, "y": 70, "w": 198, "h": 54 }
```

`anchor` ∈ `top-left top top-right left center right bottom-left bottom
bottom-right`. `x`/`y` are offsets inward from the anchored edges (towards the
centre for `top`, `left`, `center`).

## Top level

| Key | Type | Notes |
|---|---|---|
| `name`, `author` | string | shown in the skin picker / logs |
| `extends` | path | load another skin first (relative to this folder), then override |
| `base_height` | number | reference height, default 720 |
| `palette` | {token: colour} | see below |
| `fonts` | {role: font} | see below |
| `metrics` | {name: number} | see below |
| `layout` | {region: rect} | see below |
| `background` | `{ "layers": [layer…] }` | drawn back to front |
| `sprites` | [sprite…] | decorative, drawn over the background |
| `highlight` | object | selected / focused nav item |
| `cursor` | object | marker beside the selected nav item |
| `transition` | `{ "type": "none|fade|slide-left|slide-up", "ms": 160 }` | on screen change |
| `sounds` | `{ "move": wav, "confirm": wav, "back": wav, "volume": 0..1 }` | |
| `text` | {key: string} | label overrides, below |

Colours: `"#rgb"`, `"#rrggbb"` or `"#rrggbbaa"`.

Paths: relative to the skin folder; `$skins/<x>` = sibling of the skins folder's
entries (`$skins/../fonts/LatoLatin-Bold.ttf` reaches the bundled fonts);
`$exe/<x>` = next to the game executable (the player's own extracted art or HD
pack). Add `"optional": true` to a layer or sprite whose file may be absent.

## palette tokens

Theme: `bg surface surface_hi line text text_dim accent accent2 ok warn bad`.
Elements: `rail footer content control control_hover focus popup row_alt nav
nav_selected play play_hover play_text hero_fade overlay_dim overlay_panel`.

## fonts (roles)

Roles: `body` (all ordinary widgets), `heading` (screen titles), `display`
(Home title), `logo` (rail brand), `nav` (rail items), `button` (Play),
`label` (small captions, footer). A role inherits from the same role in the
`extends` base, else from `body`.

```json
"nav": { "ttf": "$skins/../fonts/LatoLatin-Bold.ttf", "size": 20, "color": "#fff",
         "uppercase": true, "italic": true,
         "shadow": { "dx": 2, "dy": 2, "color": "#000000aa" },
         "outline": { "px": 1, "color": "#000" } }
```

`bitmap` instead of `ttf` uses a BMFont text `.fnt` (one page PNG beside it);
`size` then scales the font's native size.

## metrics

`radius`, `control_radius`, `nav_item_height`, `nav_item_gap`,
`hero_height`, `hero_art` (0 hides the title's procedural hero),
`play_slant` (>0 draws the Play button as a parallelogram).

## layout regions

Launcher: `rail`, `content`, `footer`, `brand`, `play_button`. Append
`@<Screen>` (`rail@Home`) to override a region on one screen.
Overlay: `overlay.panel` (relative to the viewport), then `overlay.rail`,
`overlay.content`, `overlay.footer`, `overlay.brand` relative to the panel.

## background layers

| type | keys |
|---|---|
| `solid` | `color` |
| `gradient` | `colors`: `[top, bottom]` or `[tl, tr, br, bl]` |
| `image` | `file`, `fit` (`cover contain stretch tile`), `scroll` `[ux, uy]` units/s, `colors[0]` tint, `opacity` |
| `stripes` | `color`, `angle` (deg), `spacing`, `width`, `scroll` `[ux, 0]` |
| `streaks` | `colors` (cycled), `count`, `band` `[top%, bottom%]`, `width`, `scroll` `[ux, 0]` — horizontal light streaks |

All layers take `opacity`, `screens` (`["Home", "Graphics", …]`, `Overlay.<Screen>`; empty = all) and `optional`.

## sprites

```json
{ "file": "img/logo.png", "rect": { … }, "tint": "#fff", "opacity": 1,
  "bob_hz": 0, "bob_px": 0, "spin_hz": 0, "screens": ["Home"] }
```

Without `file`, `shape` draws one of `bar slant chevron circle ring`.

## highlight / cursor

```json
"highlight": { "style": "bar|box|fill|glow|slant|sprite", "color": "#ffb000",
               "thickness": 4, "radius": 8, "pulse_hz": 1.2, "sprite": "img/sel.png" },
"cursor":    { "sprite": "img/arrow.png" | "shape": "chevron", "w": 14, "h": 18,
               "offset_x": -4, "offset_y": 0, "bob_hz": 2, "bob_px": 3, "tint": "#e8333a" }
```

## text overrides

`brand.title`, `brand.subtitle`, `nav.<Home|Graphics|Mods|Controls|Netplay|About|Resume|Quit>`,
`title.<Screen title>`, `play`, `play.setup`, `home.title`, `home.tagline`,
`footer.hints`, `footer.overlay`.

## Working loop (people or AI)

1. Edit `skin.json`. A running launcher reloads it within half a second; a
   syntax error keeps the previous skin and prints `skin.json line N: …` in
   the footer.
2. Render: `r4l-skin-render --shots out --assets assets --skin <id> --size 1280x800`
   writes `home graphics mods controls netplay netplay-lobby setup about
   overlay overlay-netplay .png` from a hidden window (no audio).
3. Compare: `tools/skin_diff.py out/home.png reference.png -o diff.png --json`
   gives MAE, SSIM, both dominant palettes and a 4×4 region error grid; copy
   palette entries, then move the rect whose region error is worst.
