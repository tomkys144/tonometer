#include "main.hpp"
#include "AnalogIn.h"
#include "SlicingBlockDevice.h"
#include "ThisThread.h"
#include "mbed_power_mgmt.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <vector>

// main() runs in its own thread in the OS
int main() {
  AnalogIn bp_stream(PA_0, 3.3);
  AnalogIn bpm_stream(PA_1, 3.3);
  static BufferedSerial pc(PA_2, PA_3, 115200);
  char buffer[20];
  uint8_t bp[2];
  uint8_t bpm;

  uint32_t lastVoltage_bpm, amplitude_bpm;

  vector<bpm_t> bpm_measurements;

  vector<bpm_t> beat_measurements;

  printf("Started device!\n\n");

  while (true) {
    uint32_t read_voltage = bpm_stream.read_u16();
    std::tie(amplitude_bpm, lastVoltage_bpm) =
        getAmp(read_voltage, lastVoltage_bpm);

    int16_t currBP = getBloodPressure(bp_stream.read_voltage(), mmHg);

    if (amplitude_bpm > BPM_TRASHOLD) {
      if (currBP > 50) {
        //   printf("%d, %d\n", amplitude_bpm, currBP);

        bpm_t measurement = {amplitude_bpm, currBP};
        beat_measurements.push_back(measurement);
      }

    } else if (beat_measurements.size() > 0) {
      bpm_t measurement = {0, 0};
      bpm_t curr_mes;
      for (auto i = 0; i < beat_measurements.size(); i++) {
        bpm_t curr_mes = beat_measurements.at(i);
        // printf("%d, %d\n", curr_mes.amplitude, curr_mes.bloodPressure);
        if (curr_mes.amplitude > measurement.amplitude) {
          measurement = curr_mes;
        }
      }

      printf("Amplitude: %d | BP: %d\n\n", measurement.amplitude,
             measurement.bloodPressure);
      bpm_measurements.push_back(measurement);

      beat_measurements.clear();
    }

    if (bpm_measurements.size() > 0 && currBP < 50) {
      printf("MEASSUREMENT FINISHED\n");
      int16_t map, sbp, dbp = 0;
      tie(map, sbp, dbp) = calcBP(bpm_measurements);

      bp[0] = sbp;
      bp[1] = dbp;

      bpm_measurements.clear();
      printf("Pressure is %d/%d\n\n", bp[0], bp[1]);
    }
  }

  ThisThread::sleep_for(10ms);
}

int16_t getBloodPressure(double voltage, PressureUnits_t units) {
  voltage = round(voltage * 100);
  //   printf("Voltage: %f\n", voltage);
  double bp_mmHg = (voltage * BP_CONST_A) + BP_CONST_B;
  if (bp_mmHg < 0) {
    bp_mmHg = 0;
  }
  double blood_pressure = convertUnits(bp_mmHg, units);
  //   printf("BP: %f\n", blood_pressure);
  return int16_t(blood_pressure);
}

double convertUnits(double bp_mmHg, PressureUnits_t units) {
  switch (units) {
  case mmHg:
    return bp_mmHg;

  case Pa:
    return bp_mmHg * MMHG2PA;

  case psi:
    return bp_mmHg * MMHG2PSI;

  default:
    return -1;
  }
}

std::tuple<uint32_t, uint32_t> getAmp(uint32_t currentVoltage,
                                      uint32_t lastVoltage) {
  // printf("Current voltage: %d\nLast voltage: %d\n", currentVoltage,
  // lastVoltage);
  if (currentVoltage < lastVoltage) {
    return {lastVoltage, 0};
  }

  return {0, currentVoltage};
}

std::tuple<int16_t, int16_t, int16_t> calcBP(vector<bpm_t> &measurements) {
  bpm_t map, sbp, dbp = {1, 1};

  size_t map_offset = 0;

  for (auto i = 0; i < measurements.size(); i++) {
    bpm_t curr_mes = measurements.at(i);

    if (curr_mes.amplitude > map.amplitude) {
      map = curr_mes;
      map_offset = i;
    }
  }

  printf("MAP_amp: %d | MAP: %d\n", map.amplitude, map.bloodPressure);

  uint32_t min_const =
      static_cast<uint32_t>(map.amplitude * SBP_CONST - BP_UNCERTAINTY);
  uint32_t max_const =
      static_cast<uint32_t>(map.amplitude * SBP_CONST + BP_UNCERTAINTY);

  for (auto i = 0; i < measurements.size(); i++) {
    bpm_t curr_mes = measurements.at(i);

    if (curr_mes.amplitude > min_const && curr_mes.amplitude < max_const) {
      sbp = curr_mes;
    }
  }

  min_const = static_cast<uint32_t>(map.amplitude * DBP_CONST - BP_UNCERTAINTY);
  max_const = static_cast<uint32_t>(map.amplitude * DBP_CONST + BP_UNCERTAINTY);

  for (auto i = 0; i < measurements.size(); i++) {
    bpm_t curr_mes = measurements.at(i);

    if (curr_mes.amplitude > min_const && curr_mes.amplitude < max_const) {
      dbp = curr_mes;
    }
  }

  return {map.bloodPressure, sbp.bloodPressure, dbp.bloodPressure};
}
