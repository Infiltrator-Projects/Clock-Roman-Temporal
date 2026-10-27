# Ancient Roman Temporal Clock — copy validation

## Completed checks

- [x] Record Common 1.19.78 source revision `8d60662114e3052380f6f88ad3cacd9732ae7131` and original/copy file hashes.
- [x] Copy the selected `roman-temporal` provider and the supporting definitions and headers it needs; retain Common unchanged.
- [x] Build the canonical `Ancient Roman Temporal Clock.so` independently with strict C11 warnings treated as errors.
- [x] Verify metadata, callback ABI, stable identity, canonical name and the two exported entry points.
- [x] Match the original provider across **608** applicable callback observations, including invalid inputs and boundaries.
- [x] Retain **84** focused baseline fixtures; run them with `make check`.
- [x] Pass Common's existing module tests against the complete copied resource set: 6 calendar tests and 16 clock tests.

The comparison verifies that splitting and copying the code preserved the existing behaviour. Model and historical-reference claims remain limited to [MODEL.md](MODEL.md) and the original evidence; these are not newly independently proved by comparing an implementation with its origin.

## Remaining qualification

- [ ] Add independent historical/reference evidence beyond the preserved model audit where required.
- [ ] Qualify other compiler, operating-system, runtime and architecture baselines.
- [ ] Add and qualify standalone release CI if independent releases are requested.
- [ ] Create and test an independent package/installer, including upgrade/removal and any coordinated package ownership transition.
- [ ] Test actual installed consumer discovery through the native resource directory.
- [ ] Publish only artifacts from an explicitly qualified release state.

No Common files are removed or modified by this copy. See [source record](../SOURCE.md) and [machine-readable results](../VALIDATION-RESULTS.json) for exact evidence and host limitations.
