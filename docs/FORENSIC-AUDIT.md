# Ancient Roman Temporal Clock: forensic audit, 27 October 2026

The provider was built and exercised independently on Linux x86-64 with GCC 13, GLib 2.80 and ICU 74.2 where applicable. Compilation, ABI integrity, model conformance and historical authority are separate findings.

`make check` runs 84 baseline regression cases plus 80 independently sourced model fixtures. Calendar providers also check the failure-output contract. The broader audit used the counts and limits in [FORENSIC-AUDIT.json](../FORENSIC-AUDIT.json).

## Corrections

- NOAA fractional-year phase now uses 366 days in leap years.

## Accuracy boundaries and unresolved findings

- Numerical model/reconstruction, not an exact historical observing practice or precision ephemeris. See the measured astronomy errors in the full audit.
- Solar events assume a spherical sea-level horizon and standard depression; terrain, weather, and genuine polar no-crossing conditions are outside the reconstruction.
- Finite sampled checks establish only the stated model/ranges. They do not certify every historical region, all translations, every host ICU/CLDR release, or another operating system/architecture.

## Provenance and reproducibility

The original Common 1.19.78 source remains unchanged. Source-manifest hashes describe the initial extraction; subsequent corrections are recorded by Git and this audit. Original copy-comparison results are historical evidence of the extraction, not a claim that corrected code remains behavior-identical to Common.

2 baseline observations changed following documented corrections. Independent model/contract fixtures cover those bug classes.

Relevant authorities: [calendrical](https://reingold.co/calendars.shtml), [convertdate](https://convertdate.readthedocs.io/), [ICU](https://unicode-org.github.io/icu/userguide/datetime/calendar/), [NOAA](https://gml.noaa.gov/grad/solcalc/solareqns.PDF), [USNO](https://aa.usno.navy.mil/faq/GAST), [NAOJ](https://eco.mtk.nao.ac.jp/koyomi/wiki/C7F6CCC02FCCEBCCC0A4C8C6FCCAEB.html), [BWC](https://bahai-library.com/pdf/uhj/uhj_bahai_dates_172-221.pdf), [HKO](https://www.hko.gov.hk/en/gts/time/conversion.htm), [KASI](https://astro.kasi.re.kr/life/pageView/5?search_year=2017&search_month=02&search_dp=1&search_check=G), [HijriDate](https://hijridate.readthedocs.io/en/stable/background.html), [Persian](https://github.com/unicode-org/icu/blob/main/icu4c/source/i18n/persncal.cpp).
