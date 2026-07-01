# Linter Suppressions Registry

All suppressed/disabled rules and linters.
Last updated: 2026-06-28

---

## cppcheck

| Suppression | File | Reason |
|-------------|------|--------|
| normalCheckLevelMaxBranches | .mega-linter.yml args | Info reported as error due to --error-exitcode=1 |
| invalidPrintfArgType_s | xinfc-wsc.c:33 | False positive — cppcheck does not see -DXINFC_VERSION from Makefile |

## hadolint (.hadolint.yaml)

| Rule | Description | Solution |
|------|-------------|----------|
| DL3008 | apt-get without version pinning | Dev image OK |
| DL3016 | npm without version pinning | Dev tools OK |
| DL3013 | pip without version pinning | Dev tools OK |
| DL3042 | pip without --no-cache | multi-stage OK |
| DL3018 | apk without version pinning | Dev image OK |
| DL3059 | Consecutive RUN | multi-stage OK |
| DL3003 | cd in RUN | Build scripts OK |
| SC2164 | cd without protective operator | Build scripts OK |

## markdownlint (.markdownlint.json)

| Rule | Description | Rationale |
|------|-------------|-----------|
| MD013 | Line length | Standard practice |
| MD033 | Inline HTML | Required for HTML in .md |
| MD040 | Code language | Not all blocks have language |
| MD041 | First heading | Frontmatter interferes |
| MD060 | Table alignment | Variable-width tables |
| MD024 siblings_only | Duplicate headings | Keep a Changelog format |

## zizmor (.zizmor.yml)

| Rule | Reason |
|------|--------|
| unpinned-uses | Actions pinned to SHA but zizmor needs GitHub API to verify, which is rate-limited in CI |
| artipacked | Low confidence — CI uses minimal tokens |
| cache-poisoning | Low confidence — read-only workflows |
| excessive-permissions | Permissions already minimal in all workflows |
| superfluous-actions | Low confidence — standard action patterns |

## shellcheck

| Rule | File | Rationale |
|------|------|-----------|
| SC1091 | utils.sh, sync-handler.sh, hotplug-nfc-sync.sh | `/lib/functions.sh` does not exist in shellcheck environment (OpenWrt runtime only) |
| SC2154 | hotplug-nfc-sync.sh, sync-handler.sh | `config_get`/`config_get_bool` are OpenWrt builtins that assign variables. shellcheck does not recognise them → false positive |
| SC2148 | utils.sh | Sourced library — no shebang by design |
| SC2034 | utils.sh | `SSID`/`KEY`/`ENC` set in `nfc_read_wifi_creds()`, consumed by sourcing scripts |
| FILTER_REGEX_EXCLUDE | .mega-linter.yml | `Makefile` excluded — shellcheck cannot parse Makefile syntax (SC1073). postinst/prerm are inline in Makefile via `define` blocks, which use `$$` escaping that shellcheck does not understand |

## semgrep (.mega-linter.yml)

Exclusions via `--exclude-rule` in `REPOSITORY_SEMGREP_ARGUMENTS`.

| Rule | Description | Reason |
|------|-------------|--------|
| `gitlab-sast.c.buffer.c_buffer_rule-strcpy` | `strcpy()` | GCC `-Wconversion` covers this |
| `gitlab-sast.c.buffer.c_buffer_rule-strncpy` | `strncpy()` | GCC `-Wconversion` covers this |
| `gitlab-sast.c.buffer.c_buffer_rule-strcat` | `strcat()` | GCC `-Wconversion` covers this |
| `gitlab-sast.c.buffer.c_buffer_rule-memcpy-CopyMemory` | `memcpy()` | GCC `-Wconversion` covers this |
| `gitlab-sast.c.buffer.c_buffer_rule-strlen-wcslen` | `strlen()` | GCC `-Wconversion` covers this |
| `gitlab-sast.c.misc.c_misc_rule-fopen-open` | `fopen()`/`open()` | Required for I2C device access |
| `0xdea.raptor-write-into-stack-buffer` | Stack buffer writes | NFC buffer sizes are hardware-determined |
| `0xdea.raptor-integer-truncation` | Integer truncation | Explicit casts added; GCC `-Wconversion` catches rest |
| `0xdea.raptor-signed-unsigned-conversion` | Signed/unsigned | `size_t` migration handles this |
| `0xdea.raptor-secure-fill` | `memset()` for secrets | NDEF password lives in stack buffer that goes out of scope; no explicit clear needed |
| `0xdea.raptor-interesting-api-calls` | Dangerous API calls | False positives on I2C/NFC operations |
| `0xdea.raptor-argv-envp-access` | `argv`/`envp` access | Not used |
| `0xdea.raptor-pointer-subtraction` | Pointer arithmetic | GCC `-Wconversion` handles this |
| `c.lang.security.insecure-use-memset.insecure-use-memset` | `memset()` for secrets | Not applicable — no secret data |
| `0xdea.raptor-high-entropy-assignment` | High-entropy string | False positive on file paths |
| `0xdea.raptor-memory-address-exposure` | %p/address in stderr | Debug hex dump output only |
| `0xdea.raptor-missing-default-in-switch` | Switch without default | WSC attr_id enum is exhaustive |
| `0xdea.raptor-typos` | `==` used with enum values | False positive: `crypt == wifi_crypt_none` and `errno == EWOULDBLOCK` are correct C. Suppressed inline with `// nosemgrep: raptor-typos` |
| `0xdea.raptor-suspicious-assert` | `_Static_assert()` flagged as runtime assertion | `_Static_assert` is compile-time, not runtime. Suppressed inline with `/* nosemgrep: raptor-suspicious-assert */` in `i2c_nfc_device.c` |

## clang-tidy (.clang-tidy)

| Rule | Reason |
|------|--------|
| `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` | OpenWrt uses musl libc, which has no `_FORTIFY_SOURCE` in libc itself. The SDK provides [fortify-headers](https://git.2f30.org/fortify-headers) — standalone headers using GCC `__builtin_object_size` and `#include_next` to wrap `memcpy`/`snprintf` at compile time. These are invisible to clang-tidy on Alpine, so the analyzer sees naked C11 functions and flags them — a false positive. On a real OpenWrt build, all calls are already fortified. |

---

## Custom MegaLinter Flavor

Custom image built from `docker/Dockerfile.megalinter` with 17 linters, rumdl included.
