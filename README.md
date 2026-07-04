# Axzl Project

## Logging

A spdlog-like logging API is implemented. Axzl classes take "Log" objects.
Log objects enqueue entries to a queue; the queue is emptied from another (low priority) thread.

In general, logging is sent to the system log (pretty much systemd these days).
Logging can be configured to print directly to stdout. Some systems this prints to
systemd-journald anyways.



## Errors

Axzl classifies errors in three ways:

- Recoverable / expected errors
  - std::expected (or an included polyfill) is used
- Unrecoverable errors (e.g. memory exhausted)
  - Either an exception, a log entry, or ignored (see Unrecoverable Error Policy)
- Programmer / Logic errors
  - assert(s), std::abort, std::terminate, etc
  - Get development attention to fix immediately

It is recommended to compile Axzl library with exceptions enabled. Exceptions are designed
for unrecoverable errors and, when used correctly, offer many benefits.

Axzl won't preclude code used (internally to Axzl) from propagating thrown exceptions (e.g. std::out_of_range). Axzl will not utilize try / catch statements, as Axzl won't handle
unrecoverable errors. Based on the the configured Unrecoverable Error Policy, Axzl may
throw exceptions when unrecoverable errors occur. Axzl is capable of working in either
 an exceptions enabled or disabled environment.

### Unrecoverable Error Policy

When unrecoverable errors are encountered as part of Axzl code, Axzl::Throw() will be be invoked and will select one of the following options (based on configuration).

- Exception
- Log error
- Log, flush log, exception
- Ignore

Unrecoverable Error Policy is utilized via
- CMake option -DAXZL_THROW
- Environment variable

#### Scenario 1
When an unrecoverable error occurs:
- "Exception" option is selected yet
- Axzl is compiled with "-fno-exceptions"

: Axzl::Throw will terminate the application (as expected).

"Log" error option should be selected for no exception environments ("-fno-exceptions").

#### Scenario 2

When an unrecoverable error occurs:
- "Exception" option is selected yet
- Axzl is compiled with exception support

: Axzl::Throw will throw an exception





**Should we use an combination of compiled definition or environment var for exception policy?

Exceptions are often disabled in codebases (-fno-exceptions).
Axzl can be compiled with exceptions disabled; in this case, errors are logged and
no exceptions are thrown.


