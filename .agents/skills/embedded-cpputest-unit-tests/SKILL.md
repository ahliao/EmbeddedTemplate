---
name: embedded-cpputest-unit-tests
description: Build or update host-based CppUTest unit tests for embedded C/C++ modules by inspecting production code, isolating hardware dependencies, and adding focused tests that run on the host machine.
---

# Embedded CppUTest Unit Tests

Use this skill when asked to add, update, review, or fix unit tests for embedded C or C++ production modules using CppUTest and CppUMock. The goal is to test module logic on the host machine while keeping hardware, vendor HAL, RTOS, timing, and persistent side effects outside the unit boundary.

## When To Use

Use this skill when the user asks to:

- Add unit tests for an embedded C or C++ module.
- Update existing CppUTest tests for embedded production code.
- Make embedded code testable on a host machine.
- Replace hardware-dependent tests with host-based tests.
- Mock or fake HAL, RTOS, driver, timer, interrupt, storage, or bus dependencies.
- Diagnose failing CppUTest or CppUMock tests for embedded modules.

## When Not To Use

Do not use this skill when:

- The user asks for hardware-in-the-loop, board-level, integration, acceptance, or system tests.
- The test must validate actual vendor HAL behavior, silicon behavior, electrical behavior, peripheral timing, or register-level side effects on real hardware.
- The code under test is not embedded C/C++ and does not use or intend to use CppUTest.
- The user only wants a design discussion, code review, or explanation and explicitly does not want code changes.
- A simpler existing project-specific testing convention clearly supersedes this guidance.

## Workflow

### 1. Identify The Unit Under Test

1. Locate the production source and header files for the requested module.
2. Read the public API, internal helper functions, compile-time configuration, and existing tests.
3. Identify the smallest meaningful unit to test, usually one `.c`, `.cpp`, or cohesive module API.
4. Determine observable behavior through return values, output parameters, state transitions, callbacks, calls to dependencies, and error handling.
5. Avoid expanding the test scope to unrelated modules just because they are called by the unit.

### 2. Inspect Dependencies And Boundaries

Identify dependencies that cross the unit boundary, including:

- Vendor HAL calls.
- Memory-mapped hardware registers.
- Board support package functions.
- UART, SPI, I2C, GPIO, ADC, PWM, DMA, CAN, USB, Ethernet, and flash drivers.
- RTOS APIs, tasks, queues, mutexes, semaphores, event flags, and delays.
- Timers, clocks, tick counters, sleeps, watchdogs, and timeouts.
- Interrupt enable/disable calls, ISRs, critical sections, and deferred callbacks.
- Persistent storage, EEPROM, flash, NVM, files, and settings databases.
- Global state, singletons, static variables, compile-time configuration, and callbacks.

Classify each dependency as real, fake, or mocked before writing tests.

### 3. Separate Logic From Hardware-Dependent Code

Prefer tests that exercise production logic without executing hardware-dependent code. Do not test vendor HAL behavior directly.

Before changing production code:

1. Check whether the dependency can be replaced at link time with a test double.
2. Check whether an existing abstraction, weak symbol, function pointer, interface, or wrapper already exists.
3. Check whether test-only build flags or include paths can provide host-compatible headers.
4. Use existing seams before adding new seams.

Only change production code when required for testability. If changes are needed:

- Keep them minimal.
- Preserve production behavior.
- Prefer small dependency seams over broad rewrites.
- Avoid introducing backward-compatibility code unless there is a concrete need.
- Explain why the production change is necessary.

### 4. Decide What To Mock, Fake, Or Leave Real

Use this decision rule:

- Leave real: deterministic, fast, host-safe logic with no hardware, timing, threading, persistent, or process-global side effects.
- Fake: simple stateful dependencies where behavior matters more than call ordering, such as in-memory storage, ring buffers, queues, clocks, or configuration stores.
- Mock with CppUMock: boundary calls where arguments, call counts, ordering, callbacks, or error paths must be verified.

Mock or fake hardware registers, drivers, RTOS calls, timers, interrupts, persistent storage, UART/SPI/I2C/GPIO access, and callbacks when they cross the unit boundary.

Avoid over-mocking internal helper logic. Tests should verify externally meaningful behavior, not incidental implementation details.

### 5. Create Test Files

Follow the project's existing test layout and naming conventions. If there is no convention, use a clear host-test location such as:

- `tests/<module>_test.cpp`
- `test/<module>_test.cpp`
- `tests/unit/<module>_test.cpp`

For C modules tested from C++ test files, include production headers with `extern "C"`:

```cpp
extern "C" {
#include "module_under_test.h"
}
```

Include CppUTest headers as needed:

```cpp
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
```

### 6. Create TEST_GROUP And TEST Cases

Use one `TEST_GROUP` per module or closely related behavior area. Name tests after behavior, not implementation.

Good test names:

- `ReturnsErrorWhenInputIsNull`
- `StoresValueWhenDriverWriteSucceeds`
- `DoesNotStartTimerWhenAlreadyRunning`
- `InvokesCallbackOnCompleteMessage`

Avoid vague names:

- `Test1`
- `InitWorks`
- `HandlesStuff`

Cover normal behavior, edge cases, error paths, dependency failures, and relevant state transitions.

### 7. Write setup And teardown

Use `setup` to initialize only the state needed by each test:

- Reset module state.
- Initialize fake dependency state.
- Configure default mock return values only when useful.
- Clear global or static test-double state.

Use `teardown` to verify and clean up:

- Call `mock().checkExpectations()` when CppUMock expectations are used.
- Call `mock().clear()` to remove expectations, parameters, comparators, copiers, and plugin state as appropriate.
- Reset fake state that could leak into the next test.

Keep setup small. Prefer test-local setup when only one test needs it.

### 8. Add CppUMock Expectations When Needed

Use CppUMock for dependency calls that cross the unit boundary and need verification.

In tests, set expectations before calling the unit under test:

```cpp
mock().expectOneCall("driver_write")
    .withUnsignedIntParameter("address", 0x10)
    .withMemoryBufferParameter("data", expected, sizeof expected)
    .andReturnValue(0);
```

In the mocked C or C++ function, route the call through `mock()`:

```cpp
int driver_write(unsigned address, const uint8_t *data, size_t length)
{
    return mock().actualCall("driver_write")
        .withUnsignedIntParameter("address", address)
        .withMemoryBufferParameter("data", data, length)
        .returnIntValue();
}
```

Only check call ordering when order is part of the contract. Avoid brittle expectations for incidental implementation details.

### 9. Use CppUTest Assertions

Prefer type-specific assertions:

- `LONGS_EQUAL(expected, actual)` for signed integer values.
- `UNSIGNED_LONGS_EQUAL(expected, actual)` for unsigned integer values and bitmasks.
- `BYTES_EQUAL(expected, actual)` for byte-sized values.
- `STRCMP_EQUAL(expected, actual)` for C strings.
- `POINTERS_EQUAL(expected, actual)` for pointers.
- `CHECK(condition)` or `CHECK_TRUE(condition)` only when a more specific assertion is not available.

Assert externally visible results. Avoid asserting private implementation details unless there is no better observable behavior.

### 10. Update Build Files Only When Necessary

Before editing `CMakeLists.txt`, `Makefile`, or other build files:

1. Check whether the new test is automatically discovered.
2. Check existing test targets and source lists.
3. Prefer adding only the new test source or mock source required by the focused test.
4. Keep host-test build changes separate from target firmware build logic.
5. Do not add embedded vendor libraries to the host unit test target unless they are host-safe and necessary.

For CMake, add the test to the smallest relevant host test target. For Make, add only the required test, fake, mock, or production source files to the existing host test recipe.

### 11. Run Focused Tests

Run the narrowest available test command first, such as:

```sh
ctest --test-dir build --output-on-failure -R <module_or_test_name>
```

or:

```sh
make <focused-test-target>
```

or the project's existing CppUTest executable directly:

```sh
./build/tests/<test_binary> -v
```

If the focused test passes and the change touched shared test infrastructure or production seams, run the relevant broader unit test target when practical.

## Embedded-Specific Guidance

- Prefer host-based tests that run without a board, debugger, simulator, or vendor tooling.
- Do not test vendor HAL behavior directly. Test how the module responds to HAL success, failure, callbacks, and boundary conditions.
- Keep hardware access behind mocks, fakes, wrappers, interfaces, or link-time test doubles.
- Mock or fake memory-mapped registers instead of touching real addresses on the host.
- Mock or fake RTOS APIs. Do not create real host threads unless the project already has a deliberate host threading strategy.
- Mock or fake timers, ticks, delays, sleeps, and timeout sources so tests are deterministic and fast.
- Mock interrupt controls and ISR-triggered callbacks when they cross the unit boundary.
- Mock persistent storage and flash writes to avoid host side effects and to make failures injectable.
- Mock UART, SPI, I2C, GPIO, and driver access at the module boundary.
- Reset global state between tests. Embedded modules often retain static state that can leak across tests.
- Avoid changing production code unless required for testability. If a seam is needed, use the smallest seam that preserves production behavior.

## CppUTest Conventions

- Write tests in C++ files, usually with a `_test.cpp` suffix.
- Use `extern "C"` when including C production headers from C++ tests.
- Use `TEST_GROUP`, `TEST`, `setup`, and `teardown`.
- Prefer `LONGS_EQUAL`, `UNSIGNED_LONGS_EQUAL`, `BYTES_EQUAL`, `STRCMP_EQUAL`, and `POINTERS_EQUAL` over generic boolean checks.
- Use CppUMock only for unit-boundary interactions that need verification or injected return values.
- Call `mock().checkExpectations()` in `teardown` when expectations are used.
- Call `mock().clear()` in `teardown` to prevent mock state leaking between tests.
- Keep each test independent and deterministic.
- Do not rely on test execution order.

## Example C Module Test Template

```cpp
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "sensor_service.h"
}

extern "C" int adc_read_channel(unsigned channel, uint16_t *value)
{
    return mock().actualCall("adc_read_channel")
        .withUnsignedIntParameter("channel", channel)
        .withOutputParameter("value", value)
        .returnIntValue();
}

TEST_GROUP(SensorService)
{
    void setup() override
    {
        sensor_service_init();
    }

    void teardown() override
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(SensorService, ReturnsReadingWhenAdcSucceeds)
{
    uint16_t adcValue = 1234;

    mock().expectOneCall("adc_read_channel")
        .withUnsignedIntParameter("channel", 2)
        .withOutputParameterReturning("value", &adcValue, sizeof adcValue)
        .andReturnValue(0);

    uint16_t reading = 0;
    int result = sensor_service_read(2, &reading);

    LONGS_EQUAL(0, result);
    UNSIGNED_LONGS_EQUAL(1234, reading);
}

TEST(SensorService, ReturnsErrorWhenOutputPointerIsNull)
{
    int result = sensor_service_read(2, nullptr);

    LONGS_EQUAL(SENSOR_SERVICE_ERROR_INVALID_ARGUMENT, result);
}
```

Adjust names, types, return codes, and dependency functions to match the actual module. Do not copy the example dependencies into a project unless they exist or are intentionally introduced as test doubles.

## Finish Checklist

Before finishing, confirm that:

- The unit under test is clearly identified.
- Hardware, HAL, RTOS, timer, interrupt, storage, bus, and callback dependencies have been classified as real, fake, or mocked.
- Tests run on the host and do not require target hardware.
- Vendor HAL behavior is not tested directly.
- Production code changes, if any, are minimal and explained.
- C modules are included from C++ tests with `extern "C"`.
- Tests use `TEST_GROUP`, `TEST`, `setup`, and `teardown`.
- CppUMock expectations are checked and mocks are cleared in `teardown` when mocks are used.
- Assertions are type-specific where possible.
- Build files are changed only when required.
- The focused CppUTest command has been run, or any inability to run it is reported clearly.
- No unrelated files or user changes were modified.
