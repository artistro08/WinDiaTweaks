# WinDiaTweaks

Small tweaks for [Dia](https://www.diabrowser.com) on Windows, switched on and off from Dia's own Settings.

Unofficial and not affiliated with The Browser Company. Windows 11 x64, Dia for Windows (tested with 1.5.0.588).

Inspired by [DiaTweaks](https://github.com/JaceThings/DiaTweaks) for macOS by [Jace](https://x.com/JaceThings). WinDiaTweaks brings those tweaks to Dia for Windows, rebuilt from scratch for its WinUI interface, and adds features of its own on top: an Arc-style URL bar in the sidebar, a floating Ctrl+T bar, presets and more.

## Tweaks

Each tweak changes one part of how Dia looks, like the gap around pages or a button in the toolbar. Turn them on and off in **Dia → Settings → Patches**, or with the `diatweaks` command. Changes apply straight away.

At the top of **Patches**, **Preset** sets everything at once:

- **None**: Dia as it comes (the default).
- **Arc**: the address bar and buttons move into the sidebar, the toolbar hides, pages go edge to edge with square corners, Ctrl+T opens a floating bar, the window turns acrylic.
- **Custom**: your own mix. Changing any tweak switches to it.

| Tweak | Does |
|---|---|
| **Page** | |
| Edge-to-edge web content | Removes the gap between pages and the window edge |
| Square corners by the sidebar | Pages meet the sidebar and window edges with straight corners |
| **Toolbar** | |
| Hide toolbar (reveal on hover) | Hides the toolbar; it slides back in while the pointer is at the top of the window |
| Show toolbar with the sidebar | With the toolbar hidden, shows it while the sidebar is open |
| Show toolbar on new tab pages | With the toolbar hidden, keeps it on new tab pages |
| Arc URL bar ✨ | Moves the sidebar button, back, forward, reload and the address into the top of the sidebar, like Arc. The address shows just the site; its buttons appear on hover |
| Floating new tab bar ✨ | Ctrl+T shows the address bar floating over the page instead of a New Tab page; Enter opens the result in a new tab |
| **Toolbar items** | |
| Always show address field | Keeps the address field's background and buttons visible, not just on hover |
| Hide sidebar button | Hides the sidebar button |
| Hide tab search button ✨ | Hides the ⌄ button at the top of the sidebar |
| Hide Chat button | Hides the Chat button |
| **Sidebar & tab bar** | |
| Hide pinned tab border | Removes the colored border around the selected pinned tab |
| Profile emoji | Shows an emoji for this profile at the top of the sidebar |
| Don't pop up downloads | Stops the downloads list from opening by itself for each download |
| Translucent sidebar | Lets the page show through the sidebar when it floats over the page |
| **Window** | |
| Hide window buttons | Hides minimize, maximize and close (Alt+F4 still closes) |
| Window material | Mica, Mica Alt or Acrylic behind the tabs and sidebar |

✨ New in WinDiaTweaks, not in the macOS original. How each one is done on Windows: [docs/PORTING.md](docs/PORTING.md).

## Install

1. Install [Dia](https://www.diabrowser.com) and [Git](https://git-scm.com/download/win).
2. Install Microsoft's C++ build tools (free). In PowerShell:

   ```powershell
   winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
   ```

   Already have Visual Studio 2022 with **Desktop development with C++**? Skip this.
3. Paste this into PowerShell:

   ```powershell
   git clone https://github.com/zioder/WinDiaTweaks.git "$HOME\WinDiaTweaks"; & "$HOME\WinDiaTweaks\install.cmd"
   ```

It builds WinDiaTweaks on your PC, adds **Dia (Tweaks)** to the Start menu and reopens Dia with the tweaks. Open Dia from **Dia (Tweaks)** from now on, then go to **Settings → Patches**.

## The `diatweaks` command

`diatweaks.exe` is in the `bin` folder. Run it on its own to open Dia with the tweaks (or reload them if Dia already has them), or:

| Command | Does |
|---|---|
| `diatweaks list` | Shows every tweak and whether it's on |
| `diatweaks enable hideToolbar` | Turns a tweak on; Dia changes straight away |
| `diatweaks disable hideToolbar` | Turns a tweak off (`all` works too) |
| `diatweaks set windowMaterial acrylic` | Sets a tweak's option |
| `diatweaks status` | Shows whether Dia is running with the tweaks, and which are on |
| `diatweaks restart` | Closes Dia and opens it again with the tweaks |

Tweak names work as keys or titles, like `hideToolbar` or `"Hide toolbar (reveal on hover)"`.

## Updating

```powershell
cd "$HOME\WinDiaTweaks"; git pull; .\install.cmd
```

After Dia itself updates, run the same. If the build complains, delete the `gen` and `winmd` folders and run `install.cmd` again: they're made from the Dia you have installed.

## FAQ

<details>
<summary><strong>Is it safe?</strong></summary>

It's built on your PC from this code. It only changes how Dia looks, and never reads your pages, history or passwords. Your settings are one text file, `bin\tweaks.conf`.

</details>

<details>
<summary><strong>How does it work?</strong></summary>

Nothing in Dia's install folder is changed. `diatweaks.exe` opens Dia and loads a small add-on into it. The add-on uses WinUI's XAML diagnostics interface, the same one Visual Studio's Live Visual Tree uses, to move, hide and restyle parts of Dia while it runs. Turning a tweak off puts Dia's own values back.

</details>

<details>
<summary><strong>Does it run in the background?</strong></summary>

No. It only does anything while Dia is open. Dia opened any other way than **Dia (Tweaks)** or `diatweaks` runs without the tweaks.

</details>

<details>
<summary><strong>My antivirus complains.</strong></summary>

Loading an add-on into another app is something antivirus software watches for. You built it yourself from this code, so you can read exactly what it does. Allow `bin\diatweaks.exe` and `bin\DiaTweaksTap.dll` if it's blocked.

</details>

<details>
<summary><strong>What if Dia keeps crashing?</strong></summary>

If Dia closes twice in a row right after opening, it opens next time with the tweaks off. **Settings → Patches** has a button to turn them back on.

</details>

## Uninstall

Run `uninstall.cmd` in the WinDiaTweaks folder. It removes the Start menu shortcut and reopens Dia without the tweaks. Then delete the folder.

## Credits

The idea, the tweak list and the Settings → Patches design come from [DiaTweaks](https://github.com/JaceThings/DiaTweaks) by [Jace](https://x.com/JaceThings). Thank you! WinDiaTweaks is an independent Windows version and isn't maintained by Jace.

Dia is a trademark of The Browser Company. This project isn't affiliated with, endorsed by or supported by The Browser Company. Please don't report WinDiaTweaks problems to them; [open an issue here](https://github.com/zioder/WinDiaTweaks/issues) instead.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

[MIT](LICENSE)
