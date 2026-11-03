{
  description = "Screamer reverse engineering tools";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  # The LX/LE loader extension for Ghidra is released for Ghidra 12.0.x, so the
  # Ghidra shell uses a matching nixpkgs branch.  Ghidra 12.1.x removed an API
  # the loader uses.
  inputs.nixpkgs-ghidra.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs, nixpkgs-ghidra }:
    let
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          ghidraPkgs = nixpkgs-ghidra.legacyPackages.${system};
        in
        {
          # Project tools: the Python interpreter, the capstone disassembly
          # library, and radare2 for the static cross-check.
          #
          # DOSBox-X stays on Homebrew.  The run-time probe depends on the
          # DOSBox-X debugger control socket, so its version is pinned in
          # docs/reference/tooling.md.
          default = pkgs.mkShell {
            packages = [
              pkgs.python3
              pkgs.capstone
              pkgs.radare2
            ];
          };

          # Ghidra with the LX/LE loader for the DOS/4GW executables.  Use
          # `nix develop .#ghidra` when the decompiler is needed.  The loader
          # extension is installed under the Ghidra user settings directory,
          # outside the read-only Nix store.
          ghidra = ghidraPkgs.mkShell {
            packages = [
              pkgs.python3
              ghidraPkgs.ghidra
            ];
            shellHook = ''
              export GHIDRA_INSTALL_DIR="${ghidraPkgs.ghidra}/lib/ghidra"
              export PATH="$GHIDRA_INSTALL_DIR/support:$PATH"
            '';
          };

          # All three views for the mandatory cross-check.  The Ghidra pin
          # matches the LX/LE loader extension.
          cross-check = pkgs.mkShell {
            packages = [
              pkgs.python3
              pkgs.capstone
              pkgs.radare2
              ghidraPkgs.ghidra
              pkgs.curl
              pkgs.unzip
            ];
            shellHook = ''
              export GHIDRA_INSTALL_DIR="${ghidraPkgs.ghidra}/lib/ghidra"
              export PATH="$GHIDRA_INSTALL_DIR/support:$PATH"
            '';
          };
        }
      );
    };
}
