# Upstream source and capture alignment

Verified on 2026-11-08: upstream [8680ba4](https://github.com/laberning/home_io_control/commit/8680ba4c8d96a3ca412c6321c3d431622b9b247a), OFM 4f78936, OAM f275a26. The exact baseline OFM workflow 42104201195 and OAM workflow 42104217607 completed successfully. New local results and hardware observations are reported separately.

`test/corpus/upstream_alignment_frames.json` preserves reviewed functional replies from nine original YAML files at that exact upstream commit. Each source has its original URL and SHA-256. `tools/import_upstream_alignment.py` regenerates the selection from the original source checkout and validates it with `--check`. Authentication/key bodies and unreviewed register contents are excluded. Complete published frames are class A; application summaries stay B, text observations C, constructed controller/authentication transcripts S. None of these community captures supplies a verifiable device key or proves an OFM exchange's authentication.

The new RS100 captures still contain zero in the second local-sensor body byte. Upstream accepts other bytes, but no supplied original proves a nonzero variant. OFM retains its strict zero check; auxiliary B9 remains opaque. No lux or irradiance unit follows. The single battery capture is 100; codec bounds 0..100 are protocol validation, not evidence for unobserved discharge levels or a new KNX battery object.

The source archive and raw acquisition metadata are retained privately in `Velocet/Findings/2026-11-08-qualified-sensors-and-atlantic/`. No private key, missing proof bytes, unconfirmed physical effect or newly successful remote CI is inferred from the baseline.

The public importer additionally enforces exact reviewed register widths and factory model names. Weekly schedule bodies and possible/unreviewed trailers remain omitted, even when a source YAML contains them. Originals stay private; the public fixture retains source hashes and omission counts.

### Challenge duration and remaining physical qualification

Source: [upstream timing implementation](https://github.com/laberning/home_io_control/blob/38ac7461548a4dae5d1636735fdbc4a12ba46fa2/src/protocol/exchange_engine.cpp).
OFM `answer_us` starts at return from complete packet read and ends at successful RX rearm after the challenge answer. It includes correlation, HMAC, preparation, TX airtime and the observed TxDone; it is invalid until both TxDone and RX rearm succeed. Upstream starts at challenge acceptance, so these timestamps are not interchangeable. Neither clock measures the physical first preamble edge or interrupt edge. Critical logging remains suppressed through RX rearm. The native busy-RX regression is synthetic (S), not a hardware latency measurement.

MOTO1000 still requires a controlled INFO/DEBUG A/B measurement with identical preamble and physical TX/RX observations. No timeout or preamble default changes follow from the combined settings reported in [issue 163](https://github.com/laberning/home_io_control/issues/163).

### Pairing observations and bounded experiments

[Freebox Delta issue 215](https://github.com/laberning/home_io_control/issues/215) describes an atypical destination `00003D`, repeated discovery and no confirmed key transfer on a modified tester. The new synthetic controller regression verifies the existing address-class handling, explicit arming, repeated discovery after 5 s and 90 s, and unchanged key/permanent hub binding. It does not claim to replay an original RF transcript or demonstrate Freebox compatibility. The existing ten-minute extraction timeout and handshake remain unchanged; request unmodified logs and screen confirmation before changing either.

[Tatou issue 199](https://github.com/laberning/home_io_control/issues/199#issuecomment-6694865469): Expert → Recevoir la clé → Discover & Pair, then Autoriser association. Envoyer la clé is a separate explicitly armed extraction experiment; Connexion belongs to Cozytouch and Jumelage initiates typed discovery at `00FFFB`. No confirmed key transfer, reset or unsolicited key push follows from the menu observation.

[RS100 issue 187](https://github.com/laberning/home_io_control/issues/187): test the existing opt-in pairing-on-discovery-RX-channel experiment on hardware; changing LOW_POWER alone was ineffective. [LightVar issue 121](https://github.com/laberning/home_io_control/issues/121): neither changed bandwidth nor 32/48 preamble established a 2W reply. Isolated 1W enrollment remains a hardware experiment. No global PHY or default channel change is justified.

[VELUX speed issue 205](https://github.com/laberning/home_io_control/issues/205): the reported 44.5/26.5/65 s durations are class C observations. The existing main-only diagnostic and tests remain available. New ETS speed presets and Stop/Ventilation/Favorite selections require the specified device-level qualification before release; current project defaults remain unchanged.
