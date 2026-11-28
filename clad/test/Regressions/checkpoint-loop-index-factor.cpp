// RUN: %cladclang -std=c++17 -I%S/../../include %s -o %t
// RUN: %t | %filecheck_exec %s

// The reverse sweep of a checkpointed loop runs its body again, so it reads
// every value the body reads, not only those an adjoint needs. Here the index
// is read only as a factor of a step size through which nothing
// differentiated flows. Such a factor needs no storing for the product's own
// adjoint, and TBR once took that for the index not being needed at all: the
// reverse sweep did not step it back, and every recomputed iteration read the
// index the forward loop ended with. RooFit's code generation writes its
// numeric integrals like this.

#include "clad/Differentiator/Differentiator.h"
#include <cstdio>

double g(double* p, const double* x) { return p[0] * x[0] * x[0]; }

// The trapezoidal integral of p[0] * x^2 over [0, 1] in ten steps.

double counted(double* p, int) {
  double res = 0.0;
  double x[1];
  const int n = 10;
  double eps = 1.0 / n;
#pragma clad checkpoint loop
  for (int i = 0; i < n; ++i) {
    x[0] = eps * i;
    double a = g(p, x);
    x[0] = eps * (i + 1);
    double b = g(p, x);
    res += (a + b) * 0.5 * eps;
  }
  return res;
}

double runtime_bound(double* p, int n) {
  double res = 0.0;
  double x[1];
  double eps = 1.0 / 10;
#pragma clad checkpoint loop
  for (int i = 0; i < n; ++i) {
    x[0] = eps * i;
    double a = g(p, x);
    x[0] = eps * (i + 1);
    double b = g(p, x);
    res += (a + b) * 0.5 * eps;
  }
  return res;
}

double not_counted(double* p, int n) {
  double res = 0.0;
  double x[1];
  double eps = 1.0 / 10;
#pragma clad checkpoint loop
  for (int i = 0; i < n; i += 1) {
    x[0] = eps * i;
    double a = g(p, x);
    x[0] = eps * (i + 1);
    double b = g(p, x);
    res += (a + b) * 0.5 * eps;
  }
  return res;
}

int main() {
  double p[1] = {2.0};
  // The integral of x^2 over [0, 1] by the trapezoidal rule in ten steps.
  double dp[1] = {0};
  auto d_counted = clad::gradient(counted, "p");
  d_counted.execute(p, 10, dp);
  printf("%.6f\n", dp[0]); // CHECK-EXEC: 0.335000

  dp[0] = 0;
  auto d_runtime_bound = clad::gradient(runtime_bound, "p");
  d_runtime_bound.execute(p, 10, dp);
  printf("%.6f\n", dp[0]); // CHECK-EXEC: 0.335000

  dp[0] = 0;
  auto d_not_counted = clad::gradient(not_counted, "p");
  d_not_counted.execute(p, 10, dp);
  printf("%.6f\n", dp[0]); // CHECK-EXEC: 0.335000
}
