#ifndef TREELIGHTS_SIM_SYNTHETICBEAT_H
#define TREELIGHTS_SIM_SYNTHETICBEAT_H

/// Deterministic four-on-the-floor groove shaped like the Teensy
/// AudioAnalyzeFFT1024 output (512 bins, ~43 Hz each). Single source of truth
/// shared by the mac app, the render CLI, and the snapshot tests.
void SyntheticBeatBins(double tSeconds, float outBins512[]);

#endif  // TREELIGHTS_SIM_SYNTHETICBEAT_H
