// Tests for RooFit's builtin clad: the round trip from code with a clad
// request to a loaded derivative.

#include <RooFit/Detail/BuiltinClad.h>

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <stdexcept>

using RooFit::Detail::BuiltinClad::Library;

TEST(BuiltinClad, Gradient)
{
   const char *code = R"(
#include <clad/Differentiator/Differentiator.h>

#include <cmath>

double f(double *p) { return p[0] * p[0] * std::sin(p[1]); }

#pragma clad ON
extern "C" __attribute__((visibility("default"))) void *f_gradient()
{
   return reinterpret_cast<void *>(clad::gradient(f, "p").getFunctionPtr());
}
#pragma clad OFF
)";

   Library lib{"testBuiltinClad_gradient", code};

   auto grad = reinterpret_cast<void (*)(double *, double *)>(lib.entryPoint("f_gradient"));
   ASSERT_NE(grad, nullptr) << lib.compilerOutput();
   EXPECT_THROW(lib.entryPoint("no_such_function"), std::runtime_error);

   // The primal is not exported: hidden visibility keeps it out of the way of
   // any other definition in the process.
   EXPECT_EQ(lib.symbol("f"), nullptr);

   double p[2] = {2.0, 0.5};
   double out[2] = {0.0, 0.0};
   grad(p, out);
   EXPECT_DOUBLE_EQ(out[0], 2.0 * p[0] * std::sin(p[1]));
   EXPECT_DOUBLE_EQ(out[1], p[0] * p[0] * std::cos(p[1]));

   // The source stays around for debugging, in the working directory.
   EXPECT_TRUE(std::filesystem::exists(lib.sourcePath()));
   EXPECT_TRUE(std::filesystem::exists(lib.path()));
   EXPECT_EQ(std::filesystem::path(lib.path()).parent_path(), RooFit::Detail::BuiltinClad::workDir());
}

TEST(BuiltinClad, Cache)
{
   const char *code = "extern \"C\" __attribute__((visibility(\"default\"))) void *f() { return nullptr; }\n";
   auto a = Library::compile("testBuiltinClad_cache", code);
   auto b = Library::compile("testBuiltinClad_cache_again", code);
   EXPECT_EQ(a, b);
   EXPECT_NE(a->symbol("f"), nullptr);
   auto c = Library::compile("testBuiltinClad_cache_other", std::string(code) + "int g;\n");
   EXPECT_NE(a, c);
}

TEST(BuiltinClad, CompileError)
{
   try {
      Library lib{"testBuiltinClad_error", "int x = ;\n"};
      FAIL() << "compiling invalid code did not throw";
   } catch (std::runtime_error const &e) {
      // The compiler output is part of the message.
      EXPECT_NE(std::string(e.what()).find("error:"), std::string::npos) << e.what();
   }
}
