# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [1.0.3] - 2025-10-28

### Fixed

- Fix typo in log in the function `LLKERNEL_IMPL_copyToROM`.

## [1.0.2] - 2025-10-27

### Changed

- Optimize `LLKERNEL_IMPL_freeFeature` to only remove the subsector with the `feature_header` information.

### Fixed

- Fix LLKERNEL app reinstallation.
- Fix Kernel Flash Error 54 after an App Uninstall.
- Fix `LLKERNEL_IMPL_copyToROM` when data buffered from the previous call are needed.
- Fix `kernel_ram_buffer` alignment to match `LLKERNEL_RAM_ALIGN_SIZE`.
- Fix condition in `LLKERNEL_IMPL_getFeatureHandle`.
- Fix the call of `page_write` with a size greater than the page size.


## [1.0.1] - 2025-03-31

### Fixed

- Handle the case kernel_max_nb_dynamic_features is equal to zero by returning an error.
- Make LLKERNEL_IMPL_copyToROM return an error if there is an attempt to write a section that overlaps two feature slots.

### Changed

- Dependency of microej.h removed.

## [1.0.0] - 2025-03-24

### Added

- Initial revision based on the C module Generic template 0.1.0.

---
_Copyright 2025 MicroEJ Corp. All rights reserved._
_Use of this source code is governed by a BSD-style license that can be found with this software._
