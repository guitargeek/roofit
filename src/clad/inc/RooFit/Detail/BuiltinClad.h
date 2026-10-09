/*
 * Project: RooFit
 * Authors:
 *   Jonas Rembser, CERN 2026
 *
 * Copyright (c) 2026, CERN
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted according to the terms
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)
 */

#ifndef RooFit_Detail_BuiltinClad_h
#define RooFit_Detail_BuiltinClad_h

#include <memory>
#include <string>
#include <vector>

/// The builtin clad of this repository: clad as an external compiler plugin,
/// run the way ACLiC runs the compiler. See src/clad/CMakeLists.txt for why
/// the builtin clad does not run inside the interpreter.
namespace RooFit::Detail::BuiltinClad {

/// A shared library compiled from C++ code with clang and the builtin clad
/// plugin, and loaded into this process.
///
/// The code is written to `<workDir()>/<name>.cxx`, compiled into
/// `<workDir()>/<name>.so` with the command that compileCommand() describes,
/// and the library is loaded with its symbols kept local to it. The compiled
/// code includes the clad headers and whatever is on the include path of the
/// interpreter, so a translation unit that works in the interpreter works here
/// too, with clad::gradient() and friends resolved by the builtin clad. The
/// code that clad generates is written next to the source, for a debugger.
///
/// Only symbols that the code exports explicitly are visible to symbol(),
/// since the code is compiled with hidden visibility: mark them with
/// `__attribute__((visibility("default")))`, preferably as `extern "C"`.
///
/// Clad emits the derivative wherever it likes in the translation unit, under
/// a name that depends on the request, so the robust way to get hold of it is
/// to export a function that returns the pointer from the clad::CladFunction
/// object, and to call that via entryPoint():
///
///     extern "C" __attribute__((visibility("default"))) void *f_gradient()
///     {
///        return reinterpret_cast<void *>(clad::gradient(f, "p").getFunctionPtr());
///     }
class Library {
public:
   /// Compiles and loads the code. Throws std::runtime_error with the compiler
   /// output if the compilation fails, or if the library cannot be loaded.
   Library(std::string const &name, std::string const &code);
   ~Library();

   Library(Library const &) = delete;
   Library &operator=(Library const &) = delete;

   /// Like the constructor, but returns the library that was compiled from the
   /// very same code earlier in this process, if there is one. Fits of the
   /// same model generate the same code, so this spares the compilation. The
   /// libraries stay loaded for the rest of the process.
   static std::shared_ptr<Library> compile(std::string const &name, std::string const &code);

   /// The address of an exported symbol, or nullptr if there is none.
   void *symbol(std::string const &symbolName) const;

   /// Calls the exported function `void *name()` and returns what it returns.
   /// Throws std::runtime_error if there is no such symbol.
   void *entryPoint(std::string const &name) const;

   /// The compiled library.
   std::string const &path() const { return _path; }
   /// The source that was compiled.
   std::string const &sourcePath() const { return _sourcePath; }
   /// The compiler command that was run.
   std::string const &command() const { return _command; }
   /// What the compiler printed, i.e. warnings if the compilation succeeded.
   std::string const &compilerOutput() const { return _compilerOutput; }

private:
   std::string _sourcePath;
   std::string _path;
   std::string _command;
   std::string _compilerOutput;
   void *_handle = nullptr;
};

/// The clang++ that runs the plugin.
std::string const &compiler();

/// The clad plugin, clad.so.
std::string const &pluginPath();

/// The directories with the clad headers.
std::vector<std::string> const &includeDirs();

/// The directory where the sources, the libraries and the generated code are
/// kept. It is a fresh temporary directory that is removed when the process
/// exits, unless the environment variable ROOFIT_CLAD_WORKDIR names a
/// directory, which is then used and kept.
std::string const &workDir();

/// The command that compiles `source` into the shared library `output`. The
/// environment variable ROOFIT_CLAD_EXTRA_FLAGS adds flags to it.
std::string compileCommand(std::string const &source, std::string const &output);

} // namespace RooFit::Detail::BuiltinClad

#endif
