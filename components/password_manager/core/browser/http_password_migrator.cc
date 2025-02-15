// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/http_password_migrator.h"

#include "base/memory/weak_ptr.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_store.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace password_manager {

namespace {

// Helper method that allows us to pass WeakPtrs to |PasswordStoreConsumer|
// obtained via |GetWeakPtr|. This is not possible otherwise.
void OnHSTSQueryResultHelper(
    const base::WeakPtr<PasswordStoreConsumer>& migrator,
    bool is_hsts) {
  if (migrator) {
    static_cast<HttpPasswordMigrator*>(migrator.get())
        ->OnHSTSQueryResult(is_hsts);
  }
}

}  // namespace

HttpPasswordMigrator::HttpPasswordMigrator(const GURL& https_origin,
                                           const PasswordManagerClient* client,
                                           Consumer* consumer)
    : client_(client), consumer_(consumer) {
  DCHECK(client_);
  DCHECK(https_origin.is_valid());
  DCHECK(https_origin.SchemeIs(url::kHttpsScheme)) << https_origin;

  GURL::Replacements rep;
  rep.SetSchemeStr(url::kHttpScheme);
  GURL http_origin = https_origin.ReplaceComponents(rep);
  PasswordStore::FormDigest form(autofill::PasswordForm::SCHEME_HTML,
                                 http_origin.GetOrigin().spec(), http_origin);
  client_->GetPasswordStore()->GetLogins(form, this);
  client_->PostHSTSQueryForHost(
      https_origin, base::Bind(&OnHSTSQueryResultHelper, GetWeakPtr()));
}

HttpPasswordMigrator::~HttpPasswordMigrator() = default;

void HttpPasswordMigrator::OnGetPasswordStoreResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) {
  DCHECK(thread_checker_.CalledOnValidThread());
  results_ = std::move(results);
  got_password_store_results_ = true;

  if (got_hsts_query_result_)
    ProcessPasswordStoreResults();
}

void HttpPasswordMigrator::OnHSTSQueryResult(bool is_hsts) {
  DCHECK(thread_checker_.CalledOnValidThread());
  mode_ = is_hsts ? MigrationMode::MOVE : MigrationMode::COPY;
  got_hsts_query_result_ = true;

  if (got_password_store_results_)
    ProcessPasswordStoreResults();
}

void HttpPasswordMigrator::ProcessPasswordStoreResults() {
  // Android and PSL matches are ignored.
  results_.erase(
      std::remove_if(results_.begin(), results_.end(),
                     [](const std::unique_ptr<autofill::PasswordForm>& form) {
                       return form->is_affiliation_based_match ||
                              form->is_public_suffix_match;
                     }),
      results_.end());

  // Add the new credentials to the password store. The HTTP forms are
  // removed iff |mode_| == MigrationMode::MOVE.
  for (const auto& form : results_) {
    autofill::PasswordForm new_form = *form;

    GURL::Replacements rep;
    rep.SetSchemeStr(url::kHttpsScheme);
    new_form.origin = form->origin.ReplaceComponents(rep);
    new_form.signon_realm = new_form.origin.spec();
    // If |action| is not HTTPS then it's most likely obsolete. Otherwise, it
    // may still be valid.
    if (!form->action.SchemeIs(url::kHttpsScheme))
      new_form.action = new_form.origin;
    new_form.form_data = autofill::FormData();
    new_form.generation_upload_status = autofill::PasswordForm::NO_SIGNAL_SENT;
    new_form.skip_zero_click = false;
    client_->GetPasswordStore()->AddLogin(new_form);

    if (mode_ == MigrationMode::MOVE)
      client_->GetPasswordStore()->RemoveLogin(*form);
    *form = std::move(new_form);
  }

  if (!results_.empty()) {
    // Only log data if there was at least one migrated password.
    metrics_util::LogCountHttpMigratedPasswords(results_.size());
    metrics_util::LogHttpPasswordMigrationMode(
        mode_ == MigrationMode::MOVE
            ? metrics_util::HTTP_PASSWORD_MIGRATION_MODE_MOVE
            : metrics_util::HTTP_PASSWORD_MIGRATION_MODE_COPY);
  }

  if (consumer_)
    consumer_->ProcessMigratedForms(std::move(results_));
}

}  // namespace password_manager
