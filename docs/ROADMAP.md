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
- [ ] Add standalone release automation, versioning and artifact reproducibility evidence.
- [ ] Create a package/installer with verified upgrade and removal behaviour.
- [ ] Coordinate any installed package ownership transition as a separate change; Common is intentionally left intact by this task.
- [ ] Verify installed consumer integration and publish qualified releases.

The completed work is a source copy and local build/test baseline. It does not remove Common's implementation or complete a package/installer transition.

See [source provenance](../SOURCE.md) and [recorded results](../VALIDATION-RESULTS.json).
