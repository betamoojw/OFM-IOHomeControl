# Upstream source and capture alignment

Verified on 2026-11-08: upstream [8680ba4](https://github.com/laberning/home_io_control/commit/8680ba4c8d96a3ca412c6321c3d431622b9b247a), OFM 4f78936, OAM f275a26. The exact baseline OFM workflow 42104201195 and OAM workflow 42104217607 completed successfully. New local results and hardware observations are reported separately.

`test/corpus/upstream_alignment_frames.json` preserves reviewed functional replies from nine original YAML files at that exact upstream commit. Each source has its original URL and SHA-256. `tools/import_upstream_alignment.py` regenerates the selection from the original source checkout and validates it with `--check`. Authentication/key bodies and unreviewed register contents are excluded. Complete published frames are class A; application summaries stay B, text observations C, constructed controller/authentication transcripts S. None of these community captures supplies a verifiable device key or proves an OFM exchange's authentication.

The new RS100 captures still contain zero in the second local-sensor body byte. Upstream accepts other bytes, but no supplied original proves a nonzero variant. OFM retains its strict zero check; auxiliary B9 remains opaque. No lux or irradiance unit follows. The single battery capture is 100; codec bounds 0..100 are protocol validation, not evidence for unobserved discharge levels or a new KNX battery object.

The source archive and raw acquisition metadata are retained privately in `Velocet/Findings/2026-11-08-qualified-sensors-and-atlantic/`. No private key, missing proof bytes, unconfirmed physical effect or newly successful remote CI is inferred from the baseline.
