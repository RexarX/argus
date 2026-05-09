SHELL := /bin/bash

.DEFAULT_GOAL := help

ROOT_DIR := $(abspath $(CURDIR))
SCRIPTS_DIR := $(ROOT_DIR)/scripts

PYTHON ?= python3

CMAKE_ARGS ?=
PLATFORM ?=

FILES ?=
CLANG_FORMAT_CFG ?=
CLANG_TIDY_CFG ?=
BUILD_DIRS ?=
EXCLUDE ?=

.PHONY: help format format-check lint build build-debug build-relwithdebinfo build-release test test-debug test-relwithdebinfo test-release

help:
	@printf "Argus workspace helpers\n"
	@printf "\n"
	@printf "Targets:\n"
	@printf "  make format                Format C/C++ files with clang-format\n"
	@printf "  make format-check          Check C/C++ formatting with clang-format\n"
	@printf "  make lint                  Lint C/C++ files with clang-tidy\n"
	@printf "  make help                  Show this help\n"
	@printf "\n"
	@printf "Variables:\n"
	@printf "  FILES             Space-separated files/dirs to process\n"
	@printf "  EXCLUDE           Space-separated files/dirs to exclude\n"
	@printf "  CLANG_FORMAT_CFG  Optional path to .clang-format\n"
	@printf "  CLANG_TIDY_CFG    Optional path to .clang-tidy\n"
	@printf "  BUILD_DIRS        Space-separated build dirs for lint\n"
	@printf "  PYTHON            Python executable (default: python3)\n"
	@printf "\n"
	@printf "Examples:\n"
	@printf "  make format FILES=\"core/src/assert.cpp\"\n"
	@printf "  make format-check CLANG_FORMAT_CFG=.clang-format\n"
	@printf "  make lint BUILD_DIRS=\"core/build\" FILES=\"core/src\"\n"

format:
	@args=(); \
	if [[ -n "$(CLANG_FORMAT_CFG)" ]]; then args+=("--config" "$(CLANG_FORMAT_CFG)"); fi; \
	if [[ -n "$(EXCLUDE)" ]]; then for e in $(EXCLUDE); do args+=("--exclude" "$$e"); done; fi; \
	if [[ -n "$(FILES)" ]]; then for f in $(FILES); do args+=("$$f"); done; fi; \
	"$(PYTHON)" "$(SCRIPTS_DIR)/format.py" "$${args[@]}"

format-check:
	@args=("--check"); \
	if [[ -n "$(CLANG_FORMAT_CFG)" ]]; then args+=("--config" "$(CLANG_FORMAT_CFG)"); fi; \
	if [[ -n "$(EXCLUDE)" ]]; then for e in $(EXCLUDE); do args+=("--exclude" "$$e"); done; fi; \
	if [[ -n "$(FILES)" ]]; then for f in $(FILES); do args+=("$$f"); done; fi; \
	"$(PYTHON)" "$(SCRIPTS_DIR)/format.py" "$${args[@]}"

lint:
	@args=(); \
	if [[ -n "$(CLANG_TIDY_CFG)" ]]; then args+=("--config" "$(CLANG_TIDY_CFG)"); fi; \
	if [[ -n "$(BUILD_DIRS)" ]]; then for d in $(BUILD_DIRS); do args+=("--build-dir" "$$d"); done; fi; \
	if [[ -n "$(EXCLUDE)" ]]; then for e in $(EXCLUDE); do args+=("--exclude" "$$e"); done; fi; \
	if [[ -n "$(FILES)" ]]; then for f in $(FILES); do args+=("$$f"); done; fi; \
	"$(PYTHON)" "$(SCRIPTS_DIR)/lint.py" "$${args[@]}"
