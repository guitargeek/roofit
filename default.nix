with import <nixpkgs> { };

clangStdenv.mkDerivation {
  name = "roofit";
  src = ./.;

  buildInputs = [
    (root.overrideAttrs (old: { cmakeFlags = old.cmakeFlags ++ [ "-Droofit=OFF -Dtmva=OFF -Dtestsupport=ON" ]; }))
    cmake
    gtest
    gsl
    pkg-config
  ];

  cmakeFlags = [
  ];
}
