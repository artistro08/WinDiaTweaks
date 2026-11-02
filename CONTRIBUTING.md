# Contributing

Keep it small, test it hard, show it working.

## Pull requests

- One change per PR.
- Fill in the template: What, Why, Proof, Tested on, Checklist. Short sentences.
- Never leave a section empty or delete it. If one doesn't apply, write `N/A`.
- Proof is required for anything visible in Dia: a screenshot for static changes, a screen recording for anything that animates. Show before and after when you change existing behaviour.
- Try to break what you made before you open the PR, and fix what you find. Every change has to work with the tweak on and off, with the sidebar docked, closed and floating, in split view, and on the New Tab page.
- Build with `build.cmd` (or `reload.ps1`, which also restarts Dia) and run it on your own Dia.

## How it's put together

| File | What it is |
|---|---|
| `tweaks.conf` | Every tweak: title, description, section, icon, option, and its property rules |
| `src/tap.cpp` | The engine inside Dia: tracks Dia's UI tree, applies the rules, puts Dia's values back |
| `src/ui.cpp` | Settings → Patches, and the tweaks that need real code (Arc URL bar, toolbar slide, ...) |
| `src/diatweaks.cpp` | The `diatweaks` command: opens Dia, loads the add-on, edits `tweaks.conf` |
| `src/engine.h` | What `tap.cpp` and `ui.cpp` share |
| `gen-winrt.cmd` | Makes C++/WinRT headers from the WinUI metadata in your Dia install |

## Adding a tweak

1. Add an entry to `tweaks.conf`. It shows up in **Settings → Patches** and in `diatweaks list` by itself:

   ```
   [hideFooButton] off
   title: Hide Foo button
   desc: Hides the Foo button in the toolbar.
   section: Toolbar Items
   icon: E8BB
   match name=FooButton => Visibility=Collapsed
   ```

   The header of `tweaks.conf` lists every condition (`name`, `type`, `ancestor`, `upN`, `state=`, ...). A rule's values are written the way WinUI prints them.
2. Find the element to change. With Dia open through `diatweaks`:
   - `diatweaks dump` writes Dia's whole UI tree, with names, types and properties, to `bin\tree.txt`.
   - `diatweaks probe FooButton FooButton*` describes live elements: size, position, visibility, corner radius. A trailing `*` lists everything inside.
3. Hiding, moving or recoloring an element is a rule. Anything that has to react to Dia (animations, moving controls, keyboard shortcuts) goes in `src/ui.cpp`; read `Engine_TweakOn("yourKey")` and undo everything you changed when it turns off.
4. Never fight Dia frame by frame. When Dia sets a property your tweak overrides, the engine is told before the frame is drawn and puts your value back, so there's no flicker. Use that (rules, or `RegisterPropertyChangedCallback` in code) rather than timers.

## Testing

There's no automated test run yet. Before a PR, check by hand:

- The tweak on, off, and on again without restarting Dia: Dia must look exactly as before when it's off.
- **Preset → Arc** and **Preset → None** with your tweak in the mix.
- Sidebar docked, closed, and floating on hover; split view; the New Tab page; switching tabs fast.
- `diatweaks restart`: it must apply cleanly when Dia starts.

`bin\tap.log` shows what the engine did.

## Commits

- [Conventional Commits](https://www.conventionalcommits.org): `fix(arc): keep the sidebar button visible in split view`.
- Imperative, lowercase, under 72 characters. Add a body only if the diff would mislead without it.

## AI tools

Use whatever tools you like. You're responsible for what you submit: it has to work, and you have to have tested it yourself.

No slop: no padded descriptions, generated summaries or filler. PRs that read like unchecked AI output are closed without review.

## Never

- Commit Dia itself or anything from its install folder. That includes `winmd\` and `gen\`, which are made from it on your PC.
- Change files in Dia's install folder, or anything outside how Dia looks and behaves on screen.
- Read, store or send pages, history, passwords or other data from Dia.
