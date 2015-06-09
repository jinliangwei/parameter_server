#include <petuum_ps_common/util/high_resolution_timer.hpp>
#include <cmath>
#include <iostream>
#include <random>

// http://en.wikipedia.org/wiki/Fast_inverse_square_root
// kept original comments
float FastInverseSqrt(float x) {
  float x2 = x * 0.5F;
  float y = x;
  long i = * (long *) &y; // evil floating point bit level hacking
  i = 0x5f3759df - ( i >> 1 ); // what the fuck?
  y = * (float *) &i;
  y = y * (1.5F - (x2 * y * y)); // 1st iteration
  //y  = y * (1.5F - (x2 * y * y)); // 2nd iteration, this can be removed

  return y;
}

//const size_t kNumTrials = 1 << 26;
const size_t kNumTrials = 20000;

float rand_vals[kNumTrials];
float approx_vals[kNumTrials];
float exact_vals[kNumTrials];
float err = 0;

int main (int argc, char *argv[]) {
  {
    std::mt19937 gen(12345);
    std::uniform_real_distribution<float> dist(0, 10);

    for (int i = 0; i < kNumTrials; ++i) {
      rand_vals[i] = dist(gen);
    }
  }

  {
    petuum::HighResolutionTimer timer;
    for (int i = 0; i < kNumTrials; ++i) {
      approx_vals[i] = FastInverseSqrt(rand_vals[i]);
    }
    std::cout << "approx sec = " << timer.elapsed() << std::endl;
  }

  {
    petuum::HighResolutionTimer timer;
    for (int i = 0; i < kNumTrials; ++i) {
      exact_vals[i] = 1 / sqrt(rand_vals[i]);
    }
    std::cout << "real sec = " << timer.elapsed() << std::endl;
  }

  for (int i = 0; i < kNumTrials; ++i) {
    err += std::abs(approx_vals[i] - exact_vals[i]);
    //std::cout << exact_vals[i] << " " << approx_vals[i] << std::endl;
  }
  std::cout << "error = " << err << std::endl;
}
