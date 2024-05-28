{
  description = "Standalone RooFit development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];

      # This repository is built against a ROOT that does *not* contain RooFit,
      # so that the headers and libraries of the ROOT installation don't shadow
      # the ones built here. TMVA is switched off because it depends on RooFit.
      #
      # The TestSupport library has to be switched on explicitly: the RooFit
      # tests link against ROOT::TestSupport, and an installed ROOT only exports
      # that target when it was configured with -Dtestsupport=ON (or with
      # -Dtesting=ON, which implies it).
      overlay = final: prev: {
        root-no-roofit = prev.root.overrideAttrs (old: {
          pname = "root-no-roofit";
          cmakeFlags = (old.cmakeFlags or [ ]) ++ [
            # Debug symbols in the base ROOT, so you can also step into ROOT
            # code when debugging RooFit.
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            "-Droofit=OFF"
            "-Dtmva=OFF"
            "-Dtestsupport=ON"
          ];

          # ROOT builds clad through ExternalProject_Add, and that sub-build
          # runs via a generated `cmake -P .../clad-build-*.cmake` wrapper
          # which loses the make jobserver file descriptors. The sub-make
          # therefore picks its own job count and runs *on top of* ROOT's own
          # jobs, so it escapes the NIX_BUILD_CORES cap in preBuild below: we
          # measured 15 concurrent cc1plus (3 ROOT + 12 clad) peaking at 19 GB,
          # which OOM-kills the compiler.
          #
          # ROOT has a workaround that forces the clad sub-build to -j 1, but
          # it is gated on CMake older than 3.31.1 -- the assumption being that
          # newer CMake fixes jobserver propagation (Kitware issue 26398). That
          # fix does not survive the `cmake -P` wrapper, and nixpkgs ships
          # CMake 4.x, so the guard never fires. Drop the version condition and
          # always serialize the clad sub-build.
          postPatch = (old.postPatch or "") + ''
            substituteInPlace interpreter/cling/tools/plugins/clad/CMakeLists.txt \
              --replace-fail \
                'if(NOT MSVC AND CMAKE_VERSION VERSION_LESS 3.31.1)' \
                'if(NOT MSVC)'

            # ROOT 6.40's Math/CladDerivator.h unconditionally defines clad pullbacks for
            # TMVA SOFIE's Gemm_Call. With -Dtmva=OFF there is no ::TMVA namespace, the
            # interpreter fails to parse the header, and RooFit's automatic differentiation
            # breaks. Upstream moved this block into tmva/sofie after 6.40.
            substituteInPlace math/mathcore/inc/Math/CladDerivator.h \
              --replace-fail \
                'namespace TMVA::Experimental::SOFIE {' \
                '#if __has_include(<TMVA/SOFIE_common.hxx>)
            namespace TMVA::Experimental::SOFIE {' \
              --replace-fail \
                '} // namespace TMVA::Experimental::SOFIE' \
                '} // namespace TMVA::Experimental::SOFIE
            #endif'
          '';
          # The TestSupport library links against GTest::gtest, so ROOT needs to
          # find GTest at build time.
          buildInputs = (old.buildInputs or [ ]) ++ [ final.gtest ];

        });
      };

      forAllSystems =
        f:
        nixpkgs.lib.genAttrs systems (
          system:
          f (
            import nixpkgs {
              inherit system;
              overlays = [ overlay ];
            }
          )
        );

      # The dev shell, parametrized by the mkShell function so that we can also
      # offer a variant with a different compiler.
      mkRooFitShell =
        pkgs: mkShell:
        let
          # PyROOT is a CPython extension, so the interpreter that imports ROOT
          # has to be the very same one ROOT was built against. That is
          # pkgs.python3, which is what nixpkgs builds ROOT's PyROOT against.
          #
          # numpy is needed by the pythonization tests (RooDataSet.to_numpy()
          # and friends), pandas by RooAbsData.to_pandas().
          pythonEnv = pkgs.python3.withPackages (ps: [
            ps.numpy
            ps.pandas
          ]);
        in
        mkShell {
          # tools
          packages = with pkgs; [
            ccache
            cmake
            ninja
            pkg-config
            pythonEnv
            root-no-roofit
          ];
          # libraries you compile and link against
          buildInputs = with pkgs; [
            fftw
            gsl
            gtest
            libxml2
            nlohmann_json
          ];
          shellHook = ''
            # Suggested arguments for the CMake configuration step, to be used
            # from a build directory next to the repository:
            #
            #   mkdir build && cd build && cmake $CONFIGURE_ARGS
            #
            # Add -Dfftw3=ON if you want the FFT-based RooFit classes (fftw is
            # in this shell for that purpose).
            export CONFIGURE_ARGS=" \
               -DCMAKE_BUILD_TYPE=RelWithDebInfo \
               -DCMAKE_INSTALL_PREFIX=../install \
               -Dccache=ON \
               -Dclad=ON \
               -Dtesting=ON \
               -Dmathmore=ON \
               .."

            # The interpreter needs to find the headers of the externals that
            # RooFit headers include. The paths to your local RooFit build are
            # not set here: source setup.sh from the repository root for that.
            export ROOT_INCLUDE_PATH="${
              pkgs.lib.makeIncludePath [ pkgs.fftw ]
            }''${ROOT_INCLUDE_PATH:+:$ROOT_INCLUDE_PATH}"

            export PROJECT_NAME=roofit
            echo "Entered the standalone RooFit dev shell (ROOT ${pkgs.root-no-roofit.version} without RooFit)."
          '';
        };
    in
    {
      overlays.default = overlay;

      # `nix build .#root-no-roofit` builds only the ROOT base installation,
      # which is what takes long. RooFit itself is built from the working tree
      # with CMake, inside the dev shell.
      packages = forAllSystems (pkgs: {
        inherit (pkgs) root-no-roofit;
      });

      devShells = forAllSystems (pkgs: {
        default = mkRooFitShell pkgs pkgs.mkShell;
        clang = mkRooFitShell pkgs (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; });
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt);
    };
}
