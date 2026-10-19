# Distribution

## Installer package

The distribution ships one file:

| Field | Value |
| --- | --- |
| Name | `screamer_enUS_1_0_34378.pkg` |
| Size | 407266888 bytes |
| SHA-256 | `5e3717a3f61519a81cdee8e3b81acb7cc45e384d33ca1835d60f4e2ce5432c6f` |
| Format | Apple flat package (XAR archive) |
| Signature | Developer ID Installer: GOG Sp. z o.o. (9WS36Q8886) |
| Timestamp | 2019-12-09T15:39:45Z |

The package identifier is `com.gog.pkg-installer`.  The package installs
into `/Applications`.

## Package members

| Member | Size | Contents |
| --- | ---: | --- |
| `Distribution` | 774 | installer choices and license |
| `Resources/license.html` | 229 | GOG EULA link |
| `package.pkg/PackageInfo` | 498 | bundle metadata |
| `package.pkg/Scripts` | 407249531 | gzip tar of the payload |

The payload tar holds 106 entries.  The root entry is `payload/`, which is
the `Screamer.app` bundle.  The tar holds no `support/` directory.
The `postinstall` script finds no support scripts and performs these steps:

- it moves `payload/*` into the selected directory (default
  `/Applications`),
- it clears the `com.apple.quarantine` attribute,
- it sets the owner to the current user.

## Installed bundle

The installed copy is `~/Documents/Screamer.app`.  The `Launcher` binary in
the installed copy matches the payload copy (SHA-256
`d2663b471ae64f7c7ac5aad90370f0110c21be1a161a15abb3ac18e9c0f3356b`).

| Path | Role |
| --- | --- |
| `Contents/MacOS/Launcher` | macOS launcher (Mach-O x86_64) |
| `Contents/Resources/dosbox/dosbox` | DOSBox universal binary |
| `Contents/Resources/game/` | DOS game files |
| `Contents/Resources/run.apple` | AppleScript menu |
| `Contents/Resources/script` | launcher entry script |
| `Contents/Resources/script_s` | single-player launch script |
| `Contents/Resources/script_c` | network client launch script |
| `Contents/Resources/script_sp` | network host launch script |
| `Contents/Resources/replace_ip` | client address replacement |
| `Contents/Resources/goggame-1207658726.info` | GOG metadata |
| `Extras/` | DOSBox documentation and GOG EULA |

The DOSBox binary is a universal binary with `x86_64`, `i386`, and
`ppc7400` slices.  On Apple silicon it runs through Rosetta 2.

## Launch path

The `run.apple` script shows a menu with a single-player and a multiplayer
path.  The single-player path runs:

```sh
./dosbox/dosbox -conf game/dosboxScreamer.conf -conf game/dosboxScreamer_single.conf
```

The `dosboxScreamer_single.conf` file runs this autoexec sequence:

```dos
@ECHO OFF
mount C "game"
imgmount d "game/scream.dat" -t iso -fs iso
c:
cls
starth.exe
exit
```

The sequence mounts the game directory as drive C and the CD image as drive
D.  It then runs `STARTH.EXE`.

## Bundle metadata error

`Contents/Info.plist` names the bundle "The Dame Was Loaded" and uses the
identifier `com.gog.the_dame_was_loaded`.  The GOG metadata file and the
package name name the game "Screamer".  The bundle metadata is stale.  This
error does not change the game files.
