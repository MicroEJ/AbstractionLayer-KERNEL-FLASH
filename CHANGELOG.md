# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
