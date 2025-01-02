with import <nixpkgs> { };

#clangStdenv?
stdenv.mkDerivation {
  name = "roofit";
  src = ./.;

  buildInputs = [
    (root.overrideAttrs (old: { cmakeFlags = old.cmakeFlags ++ [ "-Droofit=OFF -Dtmva=OFF -Dtestsupport=ON" ]; }))
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
        -Dtesting=ON \
        -Dxml=OFF \
        .."

    alias build="cmake --build . --target install -j8"
  '';
}
