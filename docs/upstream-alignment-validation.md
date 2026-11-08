# Qualified sensor and Atlantic implementation

Local implementation against upstream 8680ba4 on 2026-11-08. Sources and evidence classes are recorded in [the source report](upstream-alignment-sources.md). This is software validation, not a hardware acceptance report.

Implemented in separate commits:

- Reviewed original captures, source hashes and reproducible importer with authentication/unreviewed bodies omitted from public fixtures.
- Explicit RS100 Solar qualification, typed Sun/Battery subscription, independent outbound reply correlation, opt-in staggered polling, delayed retry, stale/unknown handling and admission-failure accounting. No inferred sunlight unit or new battery KO.
- Explicit unknown/AGILIA/Adelis ETS model selection in previously reserved bits, live model-name qualification, typed Adelis mode/temporary-state writes and observed readback. Adelis does not inherit AGILIA window controls.
- Separate Calypso BE codecs, signed temperature and Wh/reset handling, full-width Execute, confirmed 200-l manual target steps only, mode/status readback and no optimistic shutter publication.
- Bounded opt-in Atlantic diagnostic polling through the existing transaction queue, cancellation and scoped identity ownership.
- Linked-remote union catch-up after the last managed RF exchange, mapping-change invalidation, coalescing and existing duty/admission checks.
- Challenge duration through successful RX rearm and synthetic alternate-destination/repeated discovery regressions.

Validation: `make -C test run check-storage corpus CXX=clang++ PYTHON=/tmp/iohc-alignment-venv/bin/python` completed with exit 0. Native protocol/controller/exchange suites: 471 + 374 + 76 = 921 tests, zero failures. The run also includes production publication/schema checks, mutation tests, storage checks, original-capture provenance checks and the selected real-controller tests in both release and debug. Synthetic transcripts are explicitly class S; passing them does not authenticate community captures or establish a physical action.

Application version 4.10 and module version 0.10 accompany the model selector. Existing parameter sizes, offsets, object addresses and DPTs are retained; the selector occupies reserved bits and defaults to unknown. Existing installations require an explicit model selection and fresh live name before heating writes. Unknown remains fail-closed.

## Pending acceptance

- Test on actual SX1276/SX1262 hardware: RS100 subscription authentication, a second battery sample (preferably below 100), safe poll cost/rate and unknown/stale transitions. No measured lux or irradiance conversion exists.
- Validate Adelis modes and Boost/Drying start/stop/readback, Calypso display/manual mode/50-54-62 targets, counter reset and device poll consistency. Other tanks/target steps, Calypso boost writes, holiday/off and energy splits remain disabled.
- New sensor measurement KOs and separate DHW measurement/control KOs are deferred until RF ownership/type and device acceptance are established. This change supplies local diagnostics and the versioned heating model selector; it does not claim the full new DHW ETS interface is delivered.
- Confirm linked-remote behavior under physical bursts/movement, existing solar ETS fallback polling recommendations, opt-in pairing on discovery RX channel, Tatou menus and Freebox key transfer. No new default polling value, PHY setting, reset or key-push behavior is introduced.
- Qualify VELUX motor-default/silent/fast on the same device before adding ETS speed presets; existing main-only diagnostics remain available. Measure MOTO1000 INFO/DEBUG with identical RF settings.
- Produce and validate a signed `.knxprod` with ETS/XSD available, run CI against the exact committed pin, then perform a supervised test-device flash. Local builds are not GitHub CI or a hardware test.

Local review branches remain separate from the user's working branches pending those gates. Original dirty worktree files remain untouched. No firmware was flashed, RF command sent, repository pushed or new physical success claimed.
