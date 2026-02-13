{
  description = "Tree Decomposition library";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
    }:
    let
      inherit (nixpkgs) lib;
      systems = lib.intersectLists lib.systems.flakeExposed lib.platforms.linux;
      forAllSystems = lib.genAttrs systems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});

      treedecomp-package =
        {
          lib,
          stdenv,
          cmake,
        }:
        stdenv.mkDerivation {
          name = "treedecomp";
          nativeBuildInputs = [
            cmake
          ];
          src = ./.;
        };
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake
            ];
          };
        }
      );

      packages = forAllSystems (
        system:
        let
          treedecomp = nixpkgsFor.${system}.callPackage treedecomp-package { };
        in
        {
          inherit treedecomp;
          default = treedecomp;
        }
      );
    };
}
