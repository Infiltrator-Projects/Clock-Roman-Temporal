# Ancient Roman Temporal Clock — independent resource roadmap

## Source-copy baseline completed

- [x] Identify provider `roman-temporal` in Common 1.19.78 and record its exact original revision.
- [x] Copy the provider-specific implementation into this repository.
- [x] Copy the shared supporting code it needs locally, retaining the originals in Common.
- [x] Exclude the other executable provider implementations from the copy.
- [x] Add a local build for `Ancient Roman Temporal Clock.so` and verify native ABI identity.
- [x] Compare behaviour with the original module and retain focused regression fixtures.
- [x] Update documentation to describe a working source copy.

## Later independent delivery

- [ ] Extend independent historical/model and environment qualification as required.
- [x] Add standalone release automation and versioning with deterministic archives.
- [x] Add one-provider Debian packaging and an aggressive native source installer.
- [x] Transfer this installed module through dpkg Replaces; retain Common source.
- [ ] Verify installed consumer integration and publish qualified releases.

Independent delivery is now implemented in `packaging/` and `.github/workflows/release.yml`; qualification evidence and model limits remain in the validation documents.

See [source provenance](../SOURCE.md) and [recorded results](../VALIDATION-RESULTS.json).
