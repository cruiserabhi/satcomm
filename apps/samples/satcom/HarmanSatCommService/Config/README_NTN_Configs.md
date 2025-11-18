# NTN Runtime Configuration – Profiles & Reference

This document describes the provided runtime profiles for the NB‑NTN/SATCOM stack and how each JSON field maps to behavior.

## JSON Schema (abridged)

- `version` (string) – schema/profile version.
- `satcom.init_timeout_ms` (integer, default **5000**) – time to wait for SATCOM init before deeming not ready.
- `ntn.enable` (bool) – enable/disable NTN subsystem.
- `ntn.slot_id` (1 or 2) – SIM slot used by the client.
- `ntn.require_profile_active` (bool, default **false**) – require NTN profile active on SIM/eUICC before enabling.
- `ntn.system_selection.service_domain_preference` – one of **UNKNOWN**, **CS_ONLY**, **PS_ONLY**, **CS_PS**.
- `ntn.system_selection.rat_preferences.modes` – array of RAT preference tokens (unique):
  `PREF_CDMA_1X, PREF_CDMA_EVDO, PREF_GSM, PREF_WCDMA, PREF_LTE, PREF_TDSCDMA, PREF_NR5G, PREF_NB1_NTN, PREF_NR5G_NSA, PREF_NR5G_SA`.
- `ntn.system_selection.rat_preferences.raw_bitmask_override` (integer|null) – if set, overrides the derived bitmask from `modes`.
- `ntn.system_selection.bands.*_bands` – arrays of band indexes. Leave empty for modem defaults.
- `data_plane.app_id` (integer, default **0**) – application identifier used for data-plane namespaces.
- `data_plane.max_tx_pdu_override` (integer|null) – if set, caps TX PDU size; when `null`, modem decides.
- `diagnostics.log_level` – one of **TRACE, DEBUG, INFO, WARN, ERROR** (default **INFO**).

## Derived fields

- **rat.bitmask** – computed as bit position of each mode in the order listed above (bit 0 = `PREF_CDMA_1X`, bit 7 = `PREF_NB1_NTN`, etc.).
- **bands.\*** – rendered as comma-separated band indices; **None** means "no override".

## Profile Notes

These short notes are also embedded in the Excel under the *Note* column.

- **ntn_nbntn_strict** – Strict NB‑NTN only configuration; PS domain, no terrestrial fallback.
- **ntn_prod_like_ipv4** – Production-like profile using IPv4, conservative timeouts and INFO logging.
- **ntn_ps_only_nbntn_min** – Packet‑switched only; NB‑IoT NTN only; strict selection for satellite‑only validation.
- **ntn_ps_only_nbntn_plus_lte** – Packet‑switched only; prefers NB‑IoT NTN plus LTE as secondary.
- **ntn_verbose_dev** – Developer profile with DEBUG logging and relaxed timeouts.
- **satcom_off** – Disables NTN; SATCOM can remain initialized only if other modules require it.
- **ntn_csps_allrats** – Enables both circuit‑ and packet‑switched services; all RATs allowed.
- **ntn_disabled_fallback_lte** – NTN disabled; use LTE only.
- **ntn_hardened** – Stricter timeouts; WARN/ERROR‑only logging for production hardening.
- **ntn_ipv6_localhost** – Development profile tuned for IPv6 localhost transports.

## Using these files

1. Pick or derive a profile JSON and place it alongside your application (e.g., `ntn_config.json`).
2. At startup, load and validate against the shared schema. If the file is missing or invalid, use defaults shown above.
3. Apply `system_selection` (service domain, RAT modes, bands) before bringing data plane up.
4. Respect `diagnostics.log_level` for your logging backend.

