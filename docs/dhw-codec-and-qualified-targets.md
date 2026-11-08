# Dedicated Calypso hot-water codec

Sources: [Calypso control implementation](https://github.com/laberning/home_io_control/commit/7a21ae215dd75c6f8c5ae1bfe3fd5304ea1a021f), [mode/target display confirmation](https://github.com/laberning/home_io_control/issues/198#issuecomment-6713229196), [AUTO/boost follow-up](https://github.com/laberning/home_io_control/issues/198#issuecomment-6718670438). Original status and register replies are included in the alignment fixture.

Manufacturer 0C/profile 51/0 is distinct from radiator 52/1. Family-00 values use big endian: six-byte water-temperature records start with signed tenths of a degree, energy is a four-byte Wh counter, mode is 00/01/02 and volume 00C8 is the observed 200-litre unit. Remaining temperature bytes stay opaque. Heating-state interpretation uses only the upstream-correlated heating/element/compressor/fan positions and rejects unreviewed multi-valued flags. A counter decrease is a reset/change, not fabricated rollover energy; conversion to kWh uses a wide floating representation.

The separate 16-bit main-parameter builder produces `01 ACEI HI LO 00 00`, preserving the configured ACEI. It does not modify window/garage byte-sized builders or percentage polarity. Range normalization is 20..62 °C over 0000..C800. Productive target encoding is restricted to the reported 200-litre manual-mode steps 50, 54 and 62 °C; nonfinite, intermediate, other-volume and AUTO requests fail. AUTO never exposes a valid target merely because a status contains a main parameter.

Boost remains read-only; no holiday/off or split-energy interpretation is added. KNX objects and tank-size selection require the versioned ETS integration and physical acceptance described in the plan. These codec tests do not claim new hardware operation.
