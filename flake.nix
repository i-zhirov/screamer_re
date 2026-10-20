{
  description = "Screamer reverse engineering tools";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs =
    { self, nixpkgs }:
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
        }
      );
    };
}
