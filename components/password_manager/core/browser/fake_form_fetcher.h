// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FAKE_FORM_FETCHER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FAKE_FORM_FETCHER_H_

#include <set>
#include <vector>

#include "base/macros.h"
#include "components/password_manager/core/browser/form_fetcher.h"
#include "components/password_manager/core/browser/statistics_table.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

struct InteractionsStats;

// Test implementation of FormFetcher useful for simple fakes and as a base for
// mocks.
class FakeFormFetcher : public FormFetcher {
 public:
  FakeFormFetcher();

  ~FakeFormFetcher() override;

  // Registers consumers to be notified when results are set. Unlike the
  // production version, assumes that results have not arrived yet, i.e., one
  // has to first call AddConsumer and then SetNonFederated.
  void AddConsumer(Consumer* consumer) override;

  // Returns State::WAITING if Fetch() was called after any Set* calls, and
  // State::NOT_WAITING otherwise.
  State GetState() const override;

  // Statistics for recent password bubble usage.
  const std::vector<InteractionsStats>& GetInteractionsStats() const override;

  void set_stats(const std::vector<InteractionsStats>& stats) {
    state_ = State::NOT_WAITING;
    stats_ = stats;
  }

  const std::vector<const autofill::PasswordForm*>& GetFederatedMatches()
      const override;

  void set_federated(
      const std::vector<const autofill::PasswordForm*>& federated) {
    state_ = State::NOT_WAITING;
    federated_ = federated;
  }

  void SetNonFederated(
      const std::vector<const autofill::PasswordForm*>& non_federated,
      size_t filtered_count);

  // Only sets the internal state to WAITING, no call to PasswordStore.
  void Fetch() override;

  // A no-op, returns null.
  std::unique_ptr<FormFetcher> Clone() override;

 private:
  std::set<Consumer*> consumers_;
  State state_ = State::NOT_WAITING;
  std::vector<InteractionsStats> stats_;
  std::vector<const autofill::PasswordForm*> federated_;

  DISALLOW_COPY_AND_ASSIGN(FakeFormFetcher);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FAKE_FORM_FETCHER_H_
