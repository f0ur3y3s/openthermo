# Coding standard

This project follows the **Barr Group Embedded C Coding Standard (BARR-C:2018)**
for all first-party code, plus the project rules below. Where BARR-C leaves a
choice ("preferred practice"), the project rules make it. Deviations are listed
at the end; anything not listed there is expected to comply.

**Scope.** Everything under `src/` and `components/`, except `components/u8g2`
(a build wrapper around third-party code). Test code under `test/` follows the
same rules except where noted in the deviations.

## How it is enforced

| Layer | What it catches | How to run |
|---|---|---|
| Compiler | Implicit conversions, shadowing, missing prototypes, qualifier casts. `-Werror` makes every warning fatal. Flags live in `cmake/strict_warnings.cmake`, applied per component. | `pio run` |
| clang-format | Layout: 80 columns, 4-space indent, Allman braces, mandatory braces, `char * p_name` spacing, aligned declarations. | `clang-format -i <files>` |
| clang-tidy | Naming prefixes and case, function length, braces, `bugprone`/`cert`/`misc` checks. Config in `.clang-tidy`. | `pio run -t compiledb` then `python tools/tidy_db.py --run` |
| Unit tests | Behaviour of the pure-logic modules. | `pio test -e native` |
| Review | What tools cannot see: the checklist at the end of this file. | — |

## Project rules

### 1. Declarations: all at the top, all initialised

Every local variable is declared at the top of its function and initialised in
its declaration, even when the first real assignment follows a line later.
BARR-C requires initialisation before use and prefers declaring at first use;
this project chooses the top-of-function form so every function opens with a
complete inventory of its state.

```c
esp_err_t    err   = ESP_FAIL;
nvs_handle_t h_nvs = 0U;
```

### 2. Single exit, goto-based cleanup

Every function has exactly one `return`, at the bottom. Errors jump **forward**
with `goto` to a label in the same function; BARR-C permits `goto` only in this
form. Resources are released in reverse order of acquisition by labels that
fall through into each other, so each resource has exactly one release point:

```c
esp_err_t example(void)
{
    esp_err_t    err   = ESP_FAIL;
    nvs_handle_t h_nvs = 0U;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h_nvs);
    if (ESP_OK != err)
    {
        goto done;                 // nothing acquired yet
    }

    err = nvs_set_blob(h_nvs, NVS_KEY, &g_cfg, sizeof(g_cfg));
    if (ESP_OK != err)
    {
        goto close_nvs;            // release what we hold, then exit
    }

    err = nvs_commit(h_nvs);

close_nvs:
    nvs_close(h_nvs);
done:
    return err;
}
```

- Labels are lowercase and name the action they perform (`close_nvs`,
  `cleanup_client`), with `done` always last, directly above the `return`.
- When a function keeps what it acquired on success (an init function), its
  success path ends with `goto done;` so it skips the release labels, which
  then run only on failure. `display_hal_init()` and `net_bring_up_stack()`
  are examples.
- Never jump backwards, and never jump into a block.
- A function with nothing to release still uses `goto done` for early exits.
- `continue` is not used.

### 3. Naming

| What | Rule | Example |
|---|---|---|
| Functions, variables, members, types | lowercase with underscores | `settings_get`, `zone_idx` |
| Public function, type or macro | prefixed with its module name | `net_ip_str()`, `button_event_t`, `NET_AP_SSID` |
| Types | end in `_t` | `settings_t` |
| Macros, enum constants | UPPERCASE | `SETTINGS_ZONES`, `SCHEDULE_MODE_BUTTON` |
| File-scope variable (static or not) | `g_` | `g_fsm` |
| Pointer | `p_`; pointer to pointer `pp_`; file-scope pointer `gp_` | `p_cfg`, `gp_sta_netif` |
| Boolean | `b_`; file-scope `g_b_` | `b_visible`, `g_b_joined` |
| Opaque handle (ESP-IDF/FreeRTOS `*_handle_t`, NVS handle) | `h_`; file-scope `g_h_` | `h_nvs`, `g_h_lock` |
| Any variable | 3 to 31 characters, including loop counters | `zone_idx`, never `i` |
| Log tag | a `LOG_TAG` macro per file, not ESP-IDF's `TAG` variable | `#define LOG_TAG "net"` |

Handles are pointers underneath, but this code never dereferences them, so
they carry `h_` rather than `p_`; `.clang-tidy` exempts the `h_` patterns.

### 4. Types

- Fixed-width integers from `<stdint.h>` for all numbers. Plain `char` only
  for text. `bool` from `<stdbool.h>` for flags.
- `int` appears only where an API forces it (for example `snprintf`'s return),
  and is not stored.
- No floating point in stored state. Values that arrive as decimals (a
  temperature, a coordinate) are kept as scaled integers, e.g. `int16_t`
  tenths of a degree.

### 5. Casts

Every cast carries a comment explaining why the value is in range for the
target type, on the same line or the line above. Prefer choosing types that
make the cast unnecessary.

```c
// Both limits are within uint8_t, so the clamped value narrows losslessly.
p_cfg->contrast = (uint8_t)clamp_u16(p_cfg->contrast, CONTRAST_MIN, CONTRAST_MAX);
```

### 6. Control flow

- Braces on every body, enforced by clang-format.
- Every `switch` has a `default`.
- Every `if ... else if` chain ends in an `else`, with a comment if it is
  empty.
- Constants on the left of equality tests: `if (NULL == p_cfg)`.
- A return value that reports status or a count, when deliberately ignored,
  is cast to `(void)` so the choice is visible. The standard string and memory
  functions (`strncpy`, `memcpy`, `memset`), whose return is just their
  argument back, are exempt.

### 7. Functions

- At most 100 lines (clang-tidy enforces); one job each.
- Every public function is declared in its module's header with a comment
  saying what it does, what it needs and what it returns.
- Every pointer parameter is checked for `NULL` before use, unless the
  function is `static` and every caller is visible in the same file.
- Non-public functions are `static`.

### 8. Modules and headers

- One module per component under `components/`, with its public header in
  `include/` and anything private beside the `.c` files.
- Header guards are `#ifndef MODULE_H` / `#define MODULE_H` /
  `#endif /* MODULE_H */`, never `#pragma once`.
- Each `.c` file includes its own header first.
- Headers declare; they never define variables or function bodies.
- Component requirements are minimal and as private as possible
  (`PRIV_REQUIRES` unless a public header needs the dependency).

### 9. Tasks and ownership

- The main task owns the display. Only code running on it touches `display`
  and `pages`: `app.c` for the start-up screens, then `app_ui.c` and the
  page modules it calls once the UI loop is running.
- Other tasks signal the UI task through `app_events` bits, never by calling
  into it or by sharing `volatile` flags.
- Shared data is read as a snapshot from a getter that takes the owner's
  mutex (`settings_get()`), never field by field.

### 10. Persistent data

`settings_t` is stored in NVS as a raw blob, so its layout *is* the on-flash
format. `settings_logic.c` pins the size and key offsets with
`_Static_assert`. Changing the layout requires bumping `SETTINGS_VERSION`,
freezing the previous layout in its own header, and adding a migration from
it, so saved settings survive the update.

## Deviations

| # | Deviation | Why |
|---|---|---|
| D1 | Third-party code (`components/u8g2`, ESP-IDF, and cJSON, which ships with ESP-IDF) is exempt. `test/test_weather` compiles cJSON into its translation unit with the conversion and shadowing warnings switched off around that one `#include`. | Not ours to change. Our code is isolated from it by its own wrappers. |
| D2 | ESP-IDF names used as-is: `app_main`, `ESP_LOGx`, `ESP_ERROR_CHECK`, IDF types and enum values. | Required by the framework. `app_main` cannot take a module prefix. |
| D3 | `ESP_ERROR_CHECK` (abort on error) is allowed in `app.c` start-up only. | The clock cannot run without NVS, its display or its network stack; resetting is the most useful response. Everywhere else, errors are returned. |
| D4 | Unit tests `#include` the `.c` file under test. | Keeps the native build free of ESP-IDF and build plumbing. Production code never includes a `.c` file. |
| D5 | `settings_defaults.h` uses `__has_include` (GCC, standardised in C23). | Lets the build succeed without the git-ignored secrets file, falling back to the recovery access point. |
| D6 | `misc-header-include-cycle` is disabled in `.clang-tidy`. | ESP-IDF's own FreeRTOS headers include each other in a cycle. |
| D7 | `int8_t` is exempt from the signed-char checks in `.clang-tidy`. | u8g2 reports font metrics as `int8_t`, which is numeric data, not text. |
| D8 | `src/` (the main component) is compiled without the strict warning flags, and holds only `main.c`'s three-line `app_main()`. | PlatformIO's ESP-IDF builder copies the main component's compile flags into the global environment that ESP-IDF itself is built with, so `-Werror -Wconversion` there breaks the framework build. All real code lives in `components/app`, which does get the flags; clang-tidy still checks `src/`. |

## Review checklist

Tools cannot check these. Read every change against them.

- [ ] Every local declared at the top of its function and initialised there.
- [ ] One `return`, at the bottom; every `goto` jumps forward to a cleanup
      label; resources released in reverse order, each exactly once.
- [ ] Booleans carry `b_`, handles carry `h_`; no name shorter than 3
      characters.
- [ ] Every cast has a range comment.
- [ ] Every `switch` has a `default`; every `else if` chain ends in `else`.
- [ ] No drawing outside the UI task; no new `volatile` flags between tasks.
- [ ] If `settings_t` changed: version bumped, migration added, static
      asserts updated.
- [ ] New pure logic has host tests under `test/`.
