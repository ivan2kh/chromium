// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/first_run/first_run_handler.h"

#include "base/bind.h"
#include "base/values.h"
#include "content/public/browser/web_ui.h"

namespace chromeos {

FirstRunHandler::FirstRunHandler()
    : is_initialized_(false),
      is_finalizing_(false) {
}

bool FirstRunHandler::IsInitialized() {
  return is_initialized_;
}

void FirstRunHandler::SetBackgroundVisible(bool visible) {
  web_ui()->CallJavascriptFunctionUnsafe("cr.FirstRun.setBackgroundVisible",
                                         base::Value(visible));
}

void FirstRunHandler::AddRectangularHole(int x, int y, int width, int height) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "cr.FirstRun.addRectangularHole", base::Value(x), base::Value(y),
      base::Value(width), base::Value(height));
}

void FirstRunHandler::AddRoundHole(int x, int y, float radius) {
  web_ui()->CallJavascriptFunctionUnsafe("cr.FirstRun.addRoundHole",
                                         base::Value(x), base::Value(y),
                                         base::Value(radius));
}

void FirstRunHandler::RemoveBackgroundHoles() {
  web_ui()->CallJavascriptFunctionUnsafe("cr.FirstRun.removeHoles");
}

void FirstRunHandler::ShowStepPositioned(const std::string& name,
                                         const StepPosition& position) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "cr.FirstRun.showStep", base::Value(name), *position.AsValue());
}

void FirstRunHandler::ShowStepPointingTo(const std::string& name,
                                         int x,
                                         int y,
                                         int offset) {
  std::unique_ptr<base::Value> null = base::Value::CreateNullValue();
  base::ListValue point_with_offset;
  point_with_offset.AppendInteger(x);
  point_with_offset.AppendInteger(y);
  point_with_offset.AppendInteger(offset);
  web_ui()->CallJavascriptFunctionUnsafe(
      "cr.FirstRun.showStep", base::Value(name), *null, point_with_offset);
}

void FirstRunHandler::HideCurrentStep() {
  web_ui()->CallJavascriptFunctionUnsafe("cr.FirstRun.hideCurrentStep");
}

void FirstRunHandler::Finalize() {
  is_finalizing_ = true;
  web_ui()->CallJavascriptFunctionUnsafe("cr.FirstRun.finalize");
}

bool FirstRunHandler::IsFinalizing() {
  return is_finalizing_;
}

void FirstRunHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("initialized",
      base::Bind(&FirstRunHandler::HandleInitialized, base::Unretained(this)));
  web_ui()->RegisterMessageCallback("nextButtonClicked",
      base::Bind(&FirstRunHandler::HandleNextButtonClicked,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("helpButtonClicked",
      base::Bind(&FirstRunHandler::HandleHelpButtonClicked,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("stepShown",
      base::Bind(&FirstRunHandler::HandleStepShown,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("stepHidden",
      base::Bind(&FirstRunHandler::HandleStepHidden,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("finalized",
      base::Bind(&FirstRunHandler::HandleFinalized,
                 base::Unretained(this)));
}

void FirstRunHandler::HandleInitialized(const base::ListValue* args) {
  is_initialized_ = true;
  if (delegate())
    delegate()->OnActorInitialized();
}

void FirstRunHandler::HandleNextButtonClicked(const base::ListValue* args) {
  std::string step_name;
  CHECK(args->GetString(0, &step_name));
  if (delegate())
    delegate()->OnNextButtonClicked(step_name);
}

void FirstRunHandler::HandleHelpButtonClicked(const base::ListValue* args) {
  if (delegate())
    delegate()->OnHelpButtonClicked();
}

void FirstRunHandler::HandleStepShown(const base::ListValue* args) {
  std::string step_name;
  CHECK(args->GetString(0, &step_name));
  if (delegate())
    delegate()->OnStepShown(step_name);
}

void FirstRunHandler::HandleStepHidden(const base::ListValue* args) {
  std::string step_name;
  CHECK(args->GetString(0, &step_name));
  if (delegate())
    delegate()->OnStepHidden(step_name);
}

void FirstRunHandler::HandleFinalized(const base::ListValue* args) {
  is_finalizing_ = false;
  if (delegate())
    delegate()->OnActorFinalized();
}

}  // namespace chromeos
