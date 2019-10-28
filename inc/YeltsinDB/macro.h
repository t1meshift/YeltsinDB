#pragma once

/**
 * @file macro.h
 * @brief The macros used in the engine.
 */

/** @brief Return `err` if `x` is null. */
#define THROW_IF_NULL(x, err) if (!(x)) return (err)