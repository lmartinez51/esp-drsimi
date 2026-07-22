#pragma once

namespace semantic {

/**
 * @brief Standalone deterministic regression test suite for IntentCanonicalizer.
 * Validates alias resolution for LaunchApplication, Power, Volume, Mute, Media, and Input controls.
 * @return true if all assertion test cases pass, false if any alias fails.
 */
bool RunIntentCanonicalizerTests();

} // namespace semantic
