with import <nixpkgs> { };

clangStdenv.mkDerivation {
  name = "roofit";
  src = ./.;

  buildInputs = [
    (root.overrideAttrs (old: { cmakeFlags = old.cmakeFlags ++ [ "-Droofit=OFF -Dtmva=OFF -Dtestsupport=ON" ]; }))
    cmake
    gsl
    gtest
    nlohmann_json
    pkg-config
  ];

  cmakeFlags = [
  ];
}
