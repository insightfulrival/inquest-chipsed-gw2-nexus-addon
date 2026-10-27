# Inquest Chipsed GW2 Addon — Fedora / Linux

A port of the Inquest Chipset utilities for **Guild Wars 2 + Nexus through Steam/Proton on Fedora Linux**.

This repository contains the source code, Linux build and installation scripts, the addon's click sound, and the required dependencies. Original Blish HUD binaries (`PocketUtilities.dll`, PDB files, and .NET configuration), Visual Studio projects, executables, and personal test data are excluded.

**Nexus runs inside GW2's Windows process through Proton.** Building on Fedora therefore produces a Windows x64 DLL. This DLL is required at runtime, but it is generated locally and ignored by Git. This project provides a Linux installation workflow.

## Easy installation on Fedora

You need **x86_64 Fedora**, Steam, Guild Wars 2, and Nexus already working through Proton. Close GW2 before installing. Run these commands in a terminal:

```bash
sudo dnf install git python3 curl tar xz coreutils
git clone https://github.com/insightfulrival/inquest-chipsed-gw2-addon.git
cd inquest-chipsed-gw2-addon
./setup-fedora.sh
```

Only the Fedora package installation uses `sudo`. Run the project scripts as your regular user.

`setup-fedora.sh` automatically downloads **Zig 0.14.1 for Linux** into `.tools/` if that version is unavailable on `PATH`, checks the archive against the SHA-256 published in the [official Zig download index](https://ziglang.org/download/index.json), builds the addon, and runs the installer. It leaves your system toolchain unchanged. The first build may take a few minutes; download progress appears in the terminal.

The default game location is `~/.local/share/Steam/steamapps/common/Guild Wars 2`. For another Steam library, specify the directory containing `Gw2-64.exe` and `d3d11.dll`:

```bash
./setup-fedora.sh --game "/path/to/steamapps/common/Guild Wars 2"
```

For Steam Flatpak with its default library:

```bash
./setup-fedora.sh --game "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Guild Wars 2"
```

The installer copies the DLL and sound into `addons`, backs up the Nexus configuration, and enables automatic loading. The addon will load the next time you start GW2. Clicking must be activated manually.

### Check the addon in game

1. Start GW2 through Steam using your existing Proton and Nexus setup.
2. Open Nexus and check that **Inquest Chipset** is enabled in the addon list.
3. Press `Ctrl+Shift+I` to open the settings window.
4. Change shortcuts under **Nexus → Keybinds** if you have conflicts or use a different keyboard layout.

### Build separately and update

To build without installing into the game:

```bash
./setup-fedora.sh --build-only
```

If Zig 0.14.1 is already installed elsewhere:

```bash
ZIG=/path/to/zig ./setup-fedora.sh
```

To update, close GW2 and run these commands from the project directory:

```bash
git pull --ff-only
./setup-fedora.sh
```

Add `--game "/path/to/Guild Wars 2"` again if your game is installed elsewhere. To install an existing build later, you can also run `python3 install.py --game "/path/to/Guild Wars 2"`.

### Troubleshooting installation

- **“Close GW2”**: fully exit the game and run the command again. The installer does not edit the configuration while GW2 is running.
- **“Nexus installation was not found”**: check the `--game` directory. It must contain `d3d11.dll` and `addons/Nexus/AddonConfig.json`. If the configuration file does not exist yet, launch the game with Nexus once, then close it.
- **Download or SHA-256 verification failed**: check your connection and run the script again. The build will not proceed with an unverified archive.
- **The addon does not appear in game**: check `addons/Nexus/Nexus.log` for messages about `InquestChipset.dll`.

## Usage

| Default shortcut | Action |
| --- | --- |
| Hold `,` | Repeat double clicks at the cursor every 50 ms |
| `[` | Choose an interval of 0.25–300 seconds and repeat clicks at the selected position; press again to stop |
| `Ctrl+Space` | Dodge-jump |
| `Ctrl+Shift+I` | Open addon settings |

Change shortcuts under **Nexus → Keybinds**: `INQUEST_HOLD`, `INQUEST_TOGGLE`, `INQUEST_DODGEJUMP`, and `INQUEST_SETTINGS`. The key labeled `[` may have a different label on your keyboard layout. Settings are also available in the addon's Nexus options.

Dodge-jump uses the game's bindings known to Nexus, falling back to `V` / `Space` by default. You can disable “Use Nexus game bindings” in settings and change the fallback key codes.

Clicking stops when entering combat or leaving gameplay. Chat, the map, camera movement, modifier keys, and loss of game focus suspend input. Interval mode preserves its remaining time while paused. Dodge-jump is blocked while mounted, in chat, or on the map. Use “Stop all clicking” to stop clicking.

Settings are saved in `addons/InquestChipset/settings.ini`. To disable the addon, turn it off in Nexus. To uninstall, close GW2 and remove `addons/InquestChipset.dll`; you can keep the settings folder.

## Validation and limitations

The build was verified on Fedora with Zig 0.14.1. The DLL passed a loading test using Wine 11 from Proton Experimental, covering the `GetAddonDef` export, Nexus API v6, the interface, all four shortcuts, input blocking outside gameplay, and unloading/reloading. This was an isolated test; clicking and dodge-jump have not yet been verified on a character in GW2.

The interface was rebuilt with ImGui. The port uses one original sound sample with configurable volume, rather than randomly choosing between three samples.

## Credits and provenance

This is an unofficial port. The original module was created by **Nekres**.

- [Original Blish module](https://github.com/agaertner/bhm-pocket-utilities), reference source at commit `cf8f2306d7996ac242cd1aef7a5fd99444607137`.
- [Nexus API](https://github.com/RaidcoreGG/Nexus-API), commit `9b2c53df86c00db6495642bfcff2d0611bd957ef`; license in `vendor/Nexus.LICENSE`.
- [Mumble API](https://github.com/RaidcoreGG/RCGG-lib-mumble-api), commit `8c93788c3042dd6b401c0a93d17e34e1d4414e9a`; license in `vendor/Mumble.LICENSE`.
- [ImGui Nexus 1.80](https://github.com/RaidcoreGG/imgui), commit `58075c4414b985b352d10718b02a8c43f25efd7c`; license in `vendor/imgui/LICENSE.txt`.
