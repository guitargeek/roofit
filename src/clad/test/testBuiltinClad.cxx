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

// Comments do not make a difference to the cache, but string literals do, even
// if they look like comments.
TEST(BuiltinClad, CacheIgnoresComments)
{
   const char *code = "/* a block comment */\n"
                      "extern \"C\" __attribute__((visibility(\"default\"))) void *f() { return nullptr; } // a comment\n";
   const char *sameCode = "extern \"C\" __attribute__((visibility(\"default\"))) void *f() { return nullptr; }\n"
                          "// another comment\n"
                          "/* and yet\n   another one */\n";
   auto a = Library::compile("testBuiltinClad_comments", code);
   auto b = Library::compile("testBuiltinClad_comments_again", sameCode);
   EXPECT_EQ(a, b);

   const char *withString = "extern \"C\" __attribute__((visibility(\"default\"))) const char *f() { return \"// not a comment\"; }\n";
   const char *withOtherString =
      "extern \"C\" __attribute__((visibility(\"default\"))) const char *f() { return \"// not a comment either\"; }\n";
   const char *withRawString =
      "extern \"C\" __attribute__((visibility(\"default\"))) const char *f() { return R\"(// \" /* raw)\"; }\n";
   auto c = Library::compile("testBuiltinClad_string", withString);
   auto d = Library::compile("testBuiltinClad_string_other", withOtherString);
   auto e = Library::compile("testBuiltinClad_string_raw", withRawString);
   EXPECT_NE(c, d);
   EXPECT_NE(c, e);
   EXPECT_NE(d, e);
   EXPECT_STREQ(reinterpret_cast<const char *(*)()>(c->symbol("f"))(), "// not a comment");
   EXPECT_STREQ(reinterpret_cast<const char *(*)()>(e->symbol("f"))(), "// \" /* raw");
}

TEST(BuiltinClad, PrecompiledHeader)
{
   using RooFit::Detail::BuiltinClad::PrecompiledHeader;

   const char *preamble = R"(
#include <clad/Differentiator/Differentiator.h>
#include <cmath>
#define EXPORT extern "C" __attribute__((visibility("default")))
)";
   auto pch = PrecompiledHeader::compile("testBuiltinClad_pch", preamble);
   EXPECT_TRUE(std::filesystem::exists(pch->headerPath()));
   EXPECT_TRUE(std::filesystem::exists(pch->path()));
   EXPECT_EQ(PrecompiledHeader::compile("testBuiltinClad_pch_again", std::string("// comment\n") + preamble), pch);

   // The code includes the header again, which the precompiled header has to
   // absorb: the pragma once that it carries is part of it.
   std::string code = std::string("#include \"") + pch->headerPath() + "\"\n" +
                      R"(
double f(double *p) { return std::exp(p[0]) * p[1]; }

#pragma clad ON
EXPORT void *f_gradient()
{
   return reinterpret_cast<void *>(clad::gradient(f, "p").getFunctionPtr());
}
#pragma clad OFF
)";
   auto lib = Library::compile("testBuiltinClad_with_pch", code, pch);
   EXPECT_EQ(lib->precompiledHeader(), pch);
   EXPECT_NE(lib->command().find("-include-pch"), std::string::npos) << lib->command();
   auto grad = reinterpret_cast<void (*)(double *, double *)>(lib->entryPoint("f_gradient"));
   double p[2] = {0.5, 3.0};
   double out[2] = {0.0, 0.0};
   grad(p, out);
   EXPECT_DOUBLE_EQ(out[0], std::exp(p[0]) * p[1]);
   EXPECT_DOUBLE_EQ(out[1], std::exp(p[0]));

   // The same code without the precompiled header is a different library, and
   // the same code with it is the same one.
   EXPECT_NE(Library::compile("testBuiltinClad_without_pch", code), lib);
   EXPECT_EQ(Library::compile("testBuiltinClad_with_pch_again", code, pch), lib);
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
