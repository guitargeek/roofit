// Tests for the RooONNXFunction
// Authors: Jonas Rembser, CERN 2026

#include <RooONNXFunction.h>
#include <RooRealVar.h>
#include <RooDataSet.h>
#include <RooEvaluatorWrapper.h>
#include <RooHelpers.h>
#include <RooWorkspace.h>

#include <TFile.h>

#include <gtest/gtest.h>

#include <fstream>

namespace {

std::vector<double> readDoublesFromFile(const std::string &filename)
{
   std::vector<double> values;
   std::ifstream file(filename);

   if (!file) {
      std::cerr << "Error: Could not open file " << filename << "\n";
      return values;
   }

   double x;
   while (file >> x) {
      values.push_back(x);
   }

   return values;
}

void fillArgs(RooArgList &args, int n)
{
   for (int i = 0; i < n; ++i) {
      auto v = std::make_unique<RooRealVar>(std::to_string(i).c_str(), "", 0.1, -10.0, 10.0);
      args.addOwned(std::move(v));
   }
}

} // namespace

/// Basic test for the evaluation of a RooONNXFunction with a single input
/// vector.
TEST(RooONNXFunction, Basic_1Tensor)
{
   double refPred = readDoublesFromFile("regression_mlp_pred.txt")[0];

   RooArgList args;
   fillArgs(args, 10);

   RooONNXFunction roo_func{"func", "", {args}, "regression_mlp.onnx"};

   EXPECT_NEAR(roo_func.getVal(), refPred, 1e-5);
}

#if 0
TEST(RooONNXFunction, Basic_2Tensors)
{
   double refPred = readDoublesFromFile("regression_mlp_two_input_pred.txt")[0];

   RooArgList args;
   fillArgs(args, 10);

   RooONNXFunction roo_func{"func", "", {args}, "regression_mlp_two_input.onnx"};

   EXPECT_NEAR(roo_func.getVal(), refPred, 1e-5);
}
#endif

// Test the serialization to RooWorkspace. The ONNX payload will be embedded in
// the RooWorkspace as a binary blob.
TEST(RooONNXFunction, Basic_RooWorkspace)
{
   RooHelpers::LocalChangeMsgLevel chmsglvl{RooFit::WARNING, 0u, RooFit::ObjectHandling, true};

   // Write to RooWorkspace
   {
      RooArgList args;
      fillArgs(args, 10);

      RooONNXFunction roo_func{"func", "", {args}, "regression_mlp.onnx"};
      RooWorkspace ws{"ws"};
      ws.import(roo_func);
      ws.writeToFile("RooONNXFunction_Basic.root");
   }

   // Read back and validate
   std::unique_ptr<TFile> file{TFile::Open("RooONNXFunction_Basic.root")};
   RooWorkspace *ws = dynamic_cast<RooWorkspace *>(file->Get("ws"));
   auto *roo_func = dynamic_cast<RooONNXFunction *>(ws->function("func"));

   double refPred = readDoublesFromFile("regression_mlp_pred.txt")[0];
   EXPECT_NEAR(roo_func->getVal(), refPred, 1e-5);
}

#ifdef ROOFIT_CLAD
/// Basic test for getting the analytic gradient of a RooONNXFunction with a
/// single input vector.
TEST(RooONNXFunction, Basic_CodegenAD)
{
   RooHelpers::LocalChangeMsgLevel chmsglvl{RooFit::WARNING, 0u, RooFit::Fitting, true};

   double refPred = readDoublesFromFile("regression_mlp_pred.txt")[0];
   std::vector<double> refGrad = readDoublesFromFile("regression_mlp_grad_0.txt");

   RooArgList args;
   fillArgs(args, 10);

   RooONNXFunction roo_func{"func", "", {args}, "regression_mlp.onnx"};

   RooDataSet data("data", "data", {});

   RooFit::Experimental::RooEvaluatorWrapper roo_final{roo_func, &data, false, "", nullptr, false};

   EXPECT_NEAR(roo_final.getVal(), refPred, 1e-5);

   roo_final.generateGradient();

   std::vector<double> output_vec(10);

   roo_final.gradient(output_vec.data());
   roo_final.setUseGeneratedFunctionCode(true);
   // For debugging
   // roo_final.writeDebugMacro("codegen");

   for (int i = 0; i < 10; ++i) {
      EXPECT_NEAR(output_vec[i], refGrad[i], 1e-5);
   }

   // Zero out gradient output buffer and recalculate, just to check that no
   // internal state in not reset.
   for (int i = 0; i < 10; ++i) {
      output_vec[i] = 0.;
   }

   roo_final.gradient(output_vec.data());

   for (int i = 0; i < 10; ++i) {
      EXPECT_NEAR(output_vec[i], refGrad[i], 1e-5);
   }
}
#endif
