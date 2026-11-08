# Adelis model and register boundary

Sources: [model-specific upstream implementation](https://github.com/laberning/home_io_control/commit/3923fd5f9a4f54e785186f28f0c43ada7b4dd7c7), [five controls reported working](https://github.com/laberning/home_io_control/issues/202#issuecomment-6710286656), [maintainer follow-up](https://github.com/laberning/home_io_control/issues/202#issuecomment-6718101806). Original mode, drying, sweep and boost captures are included with hashes in the alignment fixture.

AGILIA and Adelis share manufacturer 0C/profile 52/subprofile 1. The new model discriminator has Unknown, AGILIA and Adelis values; protocol identity alone never selects Adelis. Exact `ADELIS 2 DIGIT` plus optional zero padding identifies the live name. AGILIA must not satisfy that check.

The Adelis codec treats 01xx numeric fields as little endian. It distinguishes mode 0136 values 00/02/03 from temporary state 013E 00/01/02, decodes room temperature 0115 with signed/sentinel checks, setpoints 0103/0138 in tenths and 0126 as working-rate percent. It does not use 0137 as a setpoint or claim electrical watts. The observed 013A 60→55 progression remains a minutes candidate. Program phases both report mode 02; no comfort/eco-phase inference is made.

Only typed mode/temporary-state payload builders exist; no arbitrary address/value or schedule writer. Weekly 0131..0134 remain raw diagnostic reads. No Adelis window-detection/contact KO is introduced. Live command admission requires independent model, name and transaction guards; pure codec success is not write authorization or physical validation.
