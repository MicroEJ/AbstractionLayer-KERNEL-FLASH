<!--
	Markdown
-->

![ARCH](https://shields.microej.com/endpoint?url=https://repository.microej.com/packages/badges/arch_8.3.json)

# Abstraction Layer Kernel Flash

# Overview

MicroEJ Kernel Low Level API implementation to install features in an External Flash.

This module implements the generic logic of `LLKERNEL_impl` Low Level API to allocate features in flash. However,
the functions called to use the External Flash operations are specific to the target and must be implemented by the user.
See [Usage section](#usage) for more information.

Here is a list of useful MicroEJ documentation pages for the feature installation:

- [Multi-sandbox VEE Porting Guide](https://docs.microej.com/en/latest/VEEPortingGuide/multiSandbox.html#multi-sandbox)
- [Feature installation flow](https://docs.microej.com/en/latest/VEEPortingGuide/multiSandbox.html#feature-installation)
- [LLKERNEL: Multi-Sandbox appendix](https://docs.microej.com/en/latest/VEEPortingGuide/appendix/llapi.html#llkernel-multi-sandbox)

This implementation has a default configuration file, [LLKERNEL_flash_configuration.h](src/main/c/inc/LLKERNEL_flash_configuration.h).
If you want to update a configuration, please edit or create the file `veeport_configuration.h`. See [Usage section](#usage) for more information.

> [!NOTE]
> The use of this module to target an Internal Flash is not recommended since the amount of write cycles on Internal Flash is significantly lower than on External Flash memories.


# Usage

1. These sources can be included in the VEE Port with the method you prefer, by using this repository as a submodule or by doing a copy of the sources in the VEE Port repository.

2. Implement the functions listed in the file [flash_controller.h](src/main/c/inc/flash_controller.h) by following the functions documentation.

3. The configuration file [LLKERNEL_flash_configuration.h](src/main/c/inc/LLKERNEL_flash_configuration.h) stores default values of the abstraction layer configuration. If you want to update a configuration please edit or create the file `veeport_configuration.h` and set the desired value. This setting overwrites the content of [LLKERNEL_flash_configuration.h](src/main/c/inc/LLKERNEL_flash_configuration.h). If your VEE Port does not print logs using printf, the trace redirection macro `LLKERNEL_TRACE` can be updated in `veeport_configuration.h`.


# Requirements

N/A

# Validation

This Abstraction Layer implementation can be validated in the target Board Support Package using the `llkernel` tests of the [AbstractionLayer-Tests](https://github.com/MicroEJ/AbstractionLayer-Tests/tree/master/tests/llkernel) project.

Here is a non exhaustive list of tested environments:

- Hardware
  - e.g. STM32U5G9J-DK2
- Compilers / Integrated Development Environments:
  - e.g. IAR Embedded Workbench 9.50.1
- Passed the [llkernel C tests](https://github.com/MicroEJ/AbstractionLayer-Tests/tree/master/tests/llkernel) version 1.2.0

# MISRA Compliance

This Abstraction Layer implementation is MISRA-compliant (MISRA C:2012) with some noted exception.
It has been verified with Cppcheck v2.13. Here is the list of deviations from MISRA standard:

| Deviation  | Category  | Justification                                                                                                   |
|:----------:|:---------:|:--------------------------------------------------------------------------------------------------------------- |
|  Rule 2.5  |  Required | Macro defined for optional log usage.                                                                           |
|  Rule 5.5  |  Required | Macro with same name generated in intern/LLKERNEL_impl.h.                                                       |
|  Rule 8.4  |  Required | A compatible declaration is defined in headers provided by the VEE Port.                                        |
|  Rule 8.7  |  Advisory | API function that can be used in another file.                                                                  |
|  Rule 8.9  |  Advisory | Buffer located in the section .bss.kf.heap must be global.                                                      |
|  Rule 10.3 |  Required | False positive, this is a 8 bits pointer that stores a 32 bits address.                                         |
|  Rule 11.3 |  Required | Cast used by many C framework to factorize code.                                                                |
|  Rule 11.4 |  Advisory | Used when coding BSP C source code (drivers, etc.)                                                              |
|  Rule 11.5 |  Advisory | Used for code genericity/abstraction.                                                                           |
|  Rule 17.7 |  Required | Unused non-void returned type, the use of returned values of debug traces is not necessary.                     |
|  Rule 18.4 |  Advisory | Points after the + operation.                                                                                   |
|  Rule 21.6 |  Required | Used for printf usage.                                                                                          |


# Dependencies

- MicroEJ Architecture 8.1 or higher.

# Source

N/A

# Restrictions

None.

---
_Copyright 2025 MicroEJ Corp. All rights reserved._
_Use of this source code is governed by a BSD-style license that can be found with this software._
