#include "mbed.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

#define BP_CONST_A 2.044185
#define BP_CONST_B -12.55883

#define MMHG2PA 133.3
#define MMHG2PSI 0.01934

#define BPM_TRASHOLD 1000
#define SBP_CONST 0.55
#define DBP_CONST 0.85
#define BP_UNCERTAINTY 500

typedef enum { mmHg, Pa, psi } PressureUnits_t;

int16_t getBloodPressure(double voltage, PressureUnits_t units);

double convertUnits(double bp_mmHg, PressureUnits_t units);

typedef struct {
  uint32_t amplitude;
  int16_t bloodPressure;
} bpm_t;

std::tuple<uint32_t, uint32_t> getAmp(uint32_t currentVoltage,
                                      uint32_t lastVoltage);

std::tuple<int16_t, int16_t, int16_t> calcBP(vector<bpm_t>& measurements);