/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/web/WebInputElement.h"

#include "core/HTMLNames.h"
#include "core/InputTypeNames.h"
#include "core/dom/shadow/ElementShadow.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/html/HTMLDataListElement.h"
#include "core/html/HTMLDataListOptionsCollection.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/shadow/ShadowElementNames.h"
#include "core/html/shadow/TextControlInnerElements.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "public/platform/WebString.h"
#include "public/web/WebElementCollection.h"
#include "public/web/WebOptionElement.h"
#include "wtf/PassRefPtr.h"

namespace blink {

bool WebInputElement::isTextField() const {
  return constUnwrap<HTMLInputElement>()->isTextField();
}

bool WebInputElement::isText() const {
  return constUnwrap<HTMLInputElement>()->isTextField() &&
         constUnwrap<HTMLInputElement>()->type() != InputTypeNames::number;
}

bool WebInputElement::isEmailField() const {
  return constUnwrap<HTMLInputElement>()->type() == InputTypeNames::email;
}

bool WebInputElement::isPasswordField() const {
  return constUnwrap<HTMLInputElement>()->type() == InputTypeNames::password;
}

bool WebInputElement::isImageButton() const {
  return constUnwrap<HTMLInputElement>()->type() == InputTypeNames::image;
}

bool WebInputElement::isRadioButton() const {
  return constUnwrap<HTMLInputElement>()->type() == InputTypeNames::radio;
}

bool WebInputElement::isCheckbox() const {
  return constUnwrap<HTMLInputElement>()->type() == InputTypeNames::checkbox;
}

int WebInputElement::maxLength() const {
  int maxLen = constUnwrap<HTMLInputElement>()->maxLength();
  return maxLen == -1 ? defaultMaxLength() : maxLen;
}

void WebInputElement::setActivatedSubmit(bool activated) {
  unwrap<HTMLInputElement>()->setActivatedSubmit(activated);
}

int WebInputElement::size() const {
  return constUnwrap<HTMLInputElement>()->size();
}

void WebInputElement::setEditingValue(const WebString& value) {
  unwrap<HTMLInputElement>()->setEditingValue(value);
}

bool WebInputElement::isValidValue(const WebString& value) const {
  return constUnwrap<HTMLInputElement>()->isValidValue(value);
}

void WebInputElement::setChecked(bool nowChecked, bool sendEvents) {
  unwrap<HTMLInputElement>()->setChecked(
      nowChecked, sendEvents ? DispatchInputAndChangeEvent : DispatchNoEvent);
}

bool WebInputElement::isChecked() const {
  return constUnwrap<HTMLInputElement>()->checked();
}

bool WebInputElement::isMultiple() const {
  return constUnwrap<HTMLInputElement>()->multiple();
}

WebVector<WebOptionElement> WebInputElement::filteredDataListOptions() const {
  return WebVector<WebOptionElement>(
      constUnwrap<HTMLInputElement>()->filteredDataListOptions());
}

WebString WebInputElement::localizeValue(const WebString& proposedValue) const {
  return constUnwrap<HTMLInputElement>()->localizeValue(proposedValue);
}

int WebInputElement::defaultMaxLength() {
  return std::numeric_limits<int>::max();
}

void WebInputElement::setShouldRevealPassword(bool value) {
  unwrap<HTMLInputElement>()->setShouldRevealPassword(value);
}

bool WebInputElement::shouldRevealPassword() const {
  return constUnwrap<HTMLInputElement>()->shouldRevealPassword();
}

WebInputElement::WebInputElement(HTMLInputElement* elem)
    : WebFormControlElement(elem) {}

DEFINE_WEB_NODE_TYPE_CASTS(WebInputElement,
                           isHTMLInputElement(constUnwrap<Node>()));

WebInputElement& WebInputElement::operator=(HTMLInputElement* elem) {
  m_private = elem;
  return *this;
}

WebInputElement::operator HTMLInputElement*() const {
  return toHTMLInputElement(m_private.get());
}

WebInputElement* toWebInputElement(WebElement* webElement) {
  if (!isHTMLInputElement(*webElement->unwrap<Element>()))
    return 0;

  return static_cast<WebInputElement*>(webElement);
}
}  // namespace blink
