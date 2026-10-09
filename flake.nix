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

          # The builtin clad is a plugin for a specific Clang, and RooFit runs
          # it through that Clang at runtime (see src/clad/CMakeLists.txt). It
          # is independent of the LLVM inside ROOT: the two never meet. LLVM 22
          # is the newest release that clad supports, see LLVM_MAX_SUPPORTED in
          # clad/CMakeLists.txt.
          llvmPackages = pkgs.llvmPackages_22;

          # The wrapped clang knows where the C and C++ standard libraries
          # live; the unwrapped one in llvmPackages.clang-unwrapped does not.
          # The two tweaks are for clad's own test suite (-Dclad_tests=ON).
          # They have to be baked into the wrapper rather than exported by the
          # shell, because lit runs the compiler with a stripped environment.
          clang = llvmPackages.clang.override (old: {
            extraBuildCommands = (old.extraBuildCommands or "") + ''
              # The wrapper passes linker flags to clang even when only
              # compiling, and the resulting "unused argument" warnings make
              # the tests fail, since they assert that clang stays silent.
              echo "-Qunused-arguments" >> $out/nix-support/cc-cflags

              # Make -fopenmp work out of the box, for the OpenMP tests.
              echo "-isystem ${llvmPackages.openmp.dev}/include" >> $out/nix-support/cc-cflags
              echo "-L${llvmPackages.openmp}/lib" >> $out/nix-support/cc-ldflags
            '';
          });

          # Nixpkgs installs clang outside of LLVM's own prefix, but clad
          # expects to find it next to the LLVM tools: the default for
          # clad_compiler is the clang++ in LLVM_TOOLS_BINARY_DIR, and clad's
          # tests look for clang and llvm-config there as well. Hence a joined
          # prefix containing all of them, with the *wrapped* clang.
          llvmTools = pkgs.symlinkJoin {
            name = "llvm-tools-with-clang-${llvmPackages.llvm.version}";
            paths = [
              llvmPackages.llvm # FileCheck, not, count, ...
              llvmPackages.llvm.dev # llvm-config
              clang
            ];
          };

          # LLVMConfig.cmake hardcodes LLVM_TOOLS_BINARY_DIR, so re-point it at
          # the joined prefix above. Everything else keeps referring to the
          # original store paths.
          llvmCMakeDir = pkgs.runCommand "llvm-cmake-dir-${llvmPackages.llvm.version}" { } ''
            mkdir -p $out
            ln -s ${llvmPackages.llvm.dev}/lib/cmake/llvm/* $out/
            rm $out/LLVMConfig.cmake
            substitute ${llvmPackages.llvm.dev}/lib/cmake/llvm/LLVMConfig.cmake $out/LLVMConfig.cmake \
              --replace-fail "${llvmPackages.llvm}/bin" "${llvmTools}/bin"
          '';
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
            # For the builtin clad: FileCheck and friends, and lit, which are
            # only needed with -Dclad_tests=ON, but are cheap to have around.
            llvmPackages.llvm
            lit
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
            # in this shell for that purpose), and -Dclad_tests=ON to also
            # build and run the test suite of the builtin clad.
            export CONFIGURE_ARGS=" \
               -DCMAKE_BUILD_TYPE=RelWithDebInfo \
               -DCMAKE_INSTALL_PREFIX=../install \
               -Dccache=ON \
               -Dtesting=ON \
               -Dmathmore=ON \
               -DLLVM_DIR=${llvmCMakeDir} \
               -DClang_DIR=${llvmPackages.clang-unwrapped.dev}/lib/cmake/clang \
               -DLLVM_EXTERNAL_LIT=${pkgs.lit}/bin/lit \
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
