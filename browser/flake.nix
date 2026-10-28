{
  description = "Mast browser: build tooling (AWS builder) and a runner for NixOS";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      # `nix develop` → aws, rsync, ssh, zstd for ./mast
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [ awscli2 jq rsync openssh zstd curl shellcheck ];
      };

      # A normal-Linux filesystem so the Ubuntu-built browser runs on NixOS (`./mast run`).
      packages.${system}.fhs = pkgs.buildFHSEnv {
        name = "mast-fhs";
        targetPkgs = p: with p; [
          glib nss nspr gtk3 cups dbus expat libdrm mesa libgbm libGL libxkbcommon
          alsa-lib at-spi2-atk at-spi2-core pango cairo systemd wayland vulkan-loader
          libva pipewire fontconfig freetype
          libx11 libxcomposite libxdamage libxext libxfixes libxrandr libxcb
          libxcursor libxi libxrender libxtst libxshmfence
        ];
        runScript = "bash";
      };
    };
}
