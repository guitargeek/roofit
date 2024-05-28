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
    libxml2
    nlohmann_json
    pkg-config
  ];

  cmakeFlags = [
  ];

  shellHook = ''

    alias configure="cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=../install \
        -Dccache=ON \
        -Dclad=ON \
        -Dtesting=ON \
        -Dmathmore=ON \
        -Dxml=OFF \
        .."

    alias build="cmake --build . --target install -j32"

    export ROOT_INCLUDE_PATH=$PWD/install/include
    export LD_LIBRARY_PATH=$PWD/install/lib:$LD_LIBRARY_PATH
  '';
}
