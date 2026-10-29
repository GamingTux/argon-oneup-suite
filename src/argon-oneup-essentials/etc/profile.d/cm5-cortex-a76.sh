# Optimal CPU tuning flags for Raspberry Pi 5 / CM5 (ARM Cortex-A76 + Crypto + DotProd)
: "${CFLAGS:=-O2 -pipe -mcpu=cortex-a76+crypto+dotprod}"
: "${CXXFLAGS:=-O2 -pipe -mcpu=cortex-a76+crypto+dotprod}"
export CFLAGS CXXFLAGS
