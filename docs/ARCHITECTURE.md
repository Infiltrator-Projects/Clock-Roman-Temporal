# Ancient Roman Temporal Clock — architecture and boundaries

## Current ownership

**Documentation-only:** The runnable provider, tests, current combined resource packaging and release process are still in Libraries/Common. This new repository has not yet been populated with executable source, standalone tests, an installer, or a verified release.

The current canonical output name and build rules are defined in `Libraries/resources/temporal/Makefile`. The existing source is under `Libraries/resources/temporal/src/clocks/` (with Common/shared helpers where needed), and public-ABI and model regression tests include `Libraries/resources/temporal/tests/test-clocks.c`.

## Proposed independent ownership

This repository should own **exactly one** provider with stable ID `roman-temporal`: its implementation, direct build, focused tests, package/installer, version, documentation and releases. Common remains the owner of neutral date/time arithmetic, versioned temporal ABI declarations, module discovery, metadata/executable validation and shared system temporal policy.

Do not copy entire unrelated engine implementations into each new repository. Preserve shared neutral helpers in Common. A consumer must not implement a private substitute for this provider.

## Native resource interface

- **Case-sensitive path:** `/System/Resources/Clocks/Ancient Roman Temporal Clock.so`.
- **Identity:** `infiltratr_temporal_resource_metadata_v1` with valid version, stable ID, canonical display name and correct `clock` kind.
- **Execution:** a validated clock-face and next-visible-boundary provider ABI; metadata alone is not an executable resource.
- **Discovery:** Common scans native resource directories and ignores wrong-kind, malformed, missing or untrusted candidates, without hiding other valid providers.
- **Trust:** installed native `.so` code executes in the client process, so this contract applies to trusted packaged modules.
- **Lifecycle:** per-process catalogue snapshots mean consumers may need restarting after resources change.

## Build / package ownership

The new project must build `Ancient Roman Temporal Clock.so` directly, without aliases or a filename-renaming step. The released binary must be portable across its stated runtime baseline; do not embed `-march=native` from an unrelated CI host. Retain the stable resource ABI and readable metadata.

The **existing** `infiltrator-calendar-resources` Debian package currently owns this same resource path. Before a new independent Debian package takes ownership, coordinate package splitting or a declared handover; never create two competing package owners for the file.

## Scope

The clock owns both its notation and next-visible-tick scheduling. Its output needs valid geographic coordinates; missing/invalid coordinates are unavailable, not an implicit Greenwich location. Undefined solar crossings at extreme latitudes must be reported as unavailable rather than invented.

These Markdown documents do not themselves migrate code, modify CI, change system package ownership or install compiled libraries.

- [Libraries/Common temporal sources](https://github.com/Infiltrator-Projects/Libraries/tree/main/resources/temporal)
- [Current canonical build catalogue](https://github.com/Infiltrator-Projects/Libraries/blob/main/resources/temporal/Makefile)
- [Calendar chronology and clock models](https://github.com/Infiltrator-Projects/Calendar/blob/main/docs/MODELS.md)
- [Historical fidelity audit](https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/TEMPORAL_HISTORICAL_FIDELITY.md)
- [Temporal discovery and executable provider ABI](https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/TEMPORAL_PROVIDERS.md)
