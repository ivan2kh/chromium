// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_UTILS_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_UTILS_H_

namespace base {
class TimeDelta;
}

class PrefRegistrySimple;
class PrefService;

namespace chromeos {
namespace quick_unlock {

// Enumeration specifiying the possible intervals before a strong auth
// (password) is required to use quick unlock. These values correspond to the
// policy items of QuickUnlockTimeout (policy ID 352) in policy_templates.json,
// and should be updated accordingly.
enum class PasswordConfirmationFrequency {
  SIX_HOURS = 0,
  TWELVE_HOURS = 1,
  DAY = 2,
  WEEK = 3
};

base::TimeDelta PasswordConfirmationFrequencyToTimeDelta(
    PasswordConfirmationFrequency frequency);

// Register quick unlock prefs.
void RegisterProfilePrefs(PrefRegistrySimple* registry);

// Returns true if PIN unlock is allowed by policy and the quick unlock feature
// flag is present.
bool IsPinEnabled(PrefService* pref_service);

// Returns true if the fingerprint unlock feature flag is present.
bool IsFingerprintEnabled();

// Forcibly enable quick-unlock for testing.
void EnableForTesting();

}  // namespace quick_unlock
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_UTILS_H_
