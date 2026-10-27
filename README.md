# Ancient Roman Temporal Clock

**InfiltratorOS clock resource** · **Stable ID:** `roman-temporal` · **Module:** `Ancient Roman Temporal Clock.so`

Twelve unequal daylight temporal hours and night watches between computed solar crossings.

**Historical scope/accuracy:** Solar reconstruction, not independent surviving historical observations.

**Intended installation:** `/System/Resources/Clocks/Ancient Roman Temporal Clock.so` (case-sensitive)

The clock owns both its notation and next-visible-tick scheduling. Its output needs valid geographic coordinates; missing/invalid coordinates are unavailable, not an implicit Greenwich location. Undefined solar crossings at extreme latitudes must be reported as unavailable rather than invented.

## Status

**Documentation-only:** The runnable provider, tests, current combined resource packaging and release process are still in Libraries/Common. This new repository has not yet been populated with executable source, standalone tests, an installer, or a verified release.

This repository is intended to become an independently buildable, testable and installable provider, not a duplicate application-specific implementation. Calendar, Files, System Settings and compatible consumers discover executable providers through Common.

## Documentation

- [Model and evidence](docs/MODEL.md)
- [Architecture and ownership](docs/ARCHITECTURE.md)
- [Validation requirements](docs/VALIDATION.md)
- [Roadmap](docs/ROADMAP.md)
- [Documentation index](docs/README.md)

## Existing authoritative sources

- [Libraries/Common temporal sources](https://github.com/Infiltrator-Projects/Libraries/tree/main/resources/temporal)
- [Current canonical build catalogue](https://github.com/Infiltrator-Projects/Libraries/blob/main/resources/temporal/Makefile)
- [Calendar chronology and clock models](https://github.com/Infiltrator-Projects/Calendar/blob/main/docs/MODELS.md)
- [Historical fidelity audit](https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/TEMPORAL_HISTORICAL_FIDELITY.md)
- [Temporal discovery and executable provider ABI](https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/TEMPORAL_PROVIDERS.md)
