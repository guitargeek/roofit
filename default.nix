with import <nixpkgs> { };

#clangStdenv?
stdenv.mkDerivation {
  name = "roofit";
  src = ./.;

  buildInputs = [
    (
      root.overrideAttrs (old: {
        cmakeFlags = old.cmakeFlags ++ [ "-Droofit=OFF -Dtmva=OFF -Dtestsupport=ON" ];
        buildInputs = old.buildInputs ++ [ gtest ];
      })
    )
    ccache
    cmake
    gsl
    gtest
    ninja
    libxml2
    nlohmann_json
    pkg-config
  ];

  cmakeFlags = [
  ];

  shellHook = ''

    export CONFIGURE_ARGS=" \
       -DCMAKE_BUILD_TYPE=RelWithDebInfo \
       -DCMAKE_INSTALL_PREFIX=../install \
       -Dccache=ON \
       -Dclad=OFF \
       -Dtesting=ON \
       -Dmathmore=ON \
       -Dxml=OFF \
       .."

    export ROOT_INCLUDE_PATH="$PWD/install/include:${
      pkgs.lib.makeIncludePath [
        pkgs.fftw
      ]
    }"
    export LD_LIBRARY_PATH=$PWD/install/lib:$LD_LIBRARY_PATH
  '';
}
