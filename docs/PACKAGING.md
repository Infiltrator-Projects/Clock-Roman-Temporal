# Ancient Roman Temporal Clock packaging

Version: `VERSION`. Package: `infiltrator-clock-roman-temporal`.

`make release` creates exactly three user assets: a generic amd64 `.deb` using `-O2 -march=x86-64 -mtune=generic -flto`, a self-extracting source `.run`, and a full source ZIP including this provider, private helpers, public ABI headers, tests, licenses and build tools. No Common checkout or download is used. The `.run` verifies its embedded source checksum before extraction.

The aggressive installer compiles twice with GCC: instrumentation and actual provider-ABI training, then `-O3`, the compiler's local CPU target, LTO and measured PGO. Missing profile data is a build failure. Copy-regression and model-accuracy checks run during training, on the final build and after stripping. Floating-point semantics are preserved; no fast-math flag is used. Calendar authority comparison remains a separate forensic audit because some published authorities intentionally disagree.

The native package has version `VERSION+native1`; the generic release uses `VERSION`. Both own only `/System/Resources/Clocks/Ancient Roman Temporal Clock.so` and passive documentation under `/System/Resources/Documentation/Clock-Roman-Temporal/`. No installer scripts, training code or source archive are installed. `BUILD-INFO` records source revision, flags, PGO, package identity and the installed module's SHA-256. Runtime dependencies include the actual minimum GLIBC symbol version, GLib where needed, and an ICU 72–81 alternative for ICU-backed formatting.

`Replaces: infiltrator-calendar-resources` permits dpkg to transfer this provider's existing path from the old combined package without deleting the other providers. Removing this package removes its own module; removing a consumer does not. Reinstalling the legacy combined package is not the independent upgrade path; InfiltratorOS now installs the independent providers. Common's source implementation is preserved.

The release workflow installs and tests the exact native `.run` on Ubuntu 24.04, seals the three qualified artifacts, stages a draft release, verifies every remotely uploaded byte, and publishes that draft. Published releases are immutable; product changes require advancing `VERSION`. A rerun for a superseded main commit does not publish.

## Public two-file package

Installs only .so and illustrated PDF. No BUILD-INFO or source documents are
installed; native checks use the Debian X-Infiltrator metadata.
