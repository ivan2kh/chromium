// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/formatter.h"

#include <limits.h>

#include <memory>
#include <vector>

#include "base/logging.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "third_party/icu/source/i18n/unicode/msgfmt.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {

UI_BASE_EXPORT bool formatter_force_fallback = false;

struct Pluralities {
  int id;
  const char* const fallback_one;
  const char* const fallback_other;
};

static const Pluralities IDS_ELAPSED_SHORT_SEC = {
  IDS_TIME_ELAPSED_SECS,
  "one{# sec ago}",
  " other{# secs ago}"
};

static const Pluralities IDS_ELAPSED_LONG_SEC = {
    IDS_TIME_ELAPSED_LONG_SECS, "one{# second ago}", " other{# seconds ago}"};

static const Pluralities IDS_ELAPSED_SHORT_MIN = {
  IDS_TIME_ELAPSED_MINS,
  "one{# min ago}",
  " other{# mins ago}"
};

static const Pluralities IDS_ELAPSED_LONG_MIN = {
    IDS_TIME_ELAPSED_LONG_MINS, "one{# minute ago}", " other{# minutes ago}"};

static const Pluralities IDS_ELAPSED_HOUR = {
  IDS_TIME_ELAPSED_HOURS,
  "one{# hour ago}",
  " other{# hours ago}"
};
static const Pluralities IDS_ELAPSED_DAY = {
  IDS_TIME_ELAPSED_DAYS,
  "one{# day ago}",
  " other{# days ago}"
};

static const Pluralities IDS_REMAINING_SHORT_SEC = {
  IDS_TIME_REMAINING_SECS,
  "one{# sec left}",
  " other{# secs left}"
};
static const Pluralities IDS_REMAINING_SHORT_MIN = {
  IDS_TIME_REMAINING_MINS,
  "one{# min left}",
  " other{# mins left}"
};

static const Pluralities IDS_REMAINING_LONG_SEC = {
  IDS_TIME_REMAINING_LONG_SECS,
  "one{# second left}",
  " other{# seconds left}"
};
static const Pluralities IDS_REMAINING_LONG_MIN = {
  IDS_TIME_REMAINING_LONG_MINS,
  "one{# minute left}",
  " other{# minutes left}"
};
static const Pluralities IDS_REMAINING_HOUR = {
  IDS_TIME_REMAINING_HOURS,
  "one{# hour left}",
  " other{# hours left}"
};
static const Pluralities IDS_REMAINING_DAY = {
  IDS_TIME_REMAINING_DAYS,
  "one{# day left}",
  " other{# days left}"
};

static const Pluralities IDS_DURATION_SHORT_SEC = {
  IDS_TIME_SECS,
  "one{# sec}",
  " other{# secs}"
};
static const Pluralities IDS_DURATION_SHORT_MIN = {
  IDS_TIME_MINS,
  "one{# min}",
  " other{# mins}"
};

static const Pluralities IDS_LONG_SEC = {
  IDS_TIME_LONG_SECS,
  "one{# second}",
  " other{# seconds}"
};
static const Pluralities IDS_LONG_MIN = {
  IDS_TIME_LONG_MINS,
  "one{# minute}",
  " other{# minutes}"
};
static const Pluralities IDS_DURATION_HOUR = {
  IDS_TIME_HOURS,
  "one{# hour}",
  " other{# hours}"
};
static const Pluralities IDS_DURATION_DAY = {
  IDS_TIME_DAYS,
  "one{# day}",
  " other{# days}"
};

static const Pluralities IDS_LONG_MIN_1ST = {
  IDS_TIME_LONG_MINS_1ST,
  "one{# minute and }",
  " other{# minutes and }"
};
static const Pluralities IDS_LONG_SEC_2ND = {
  IDS_TIME_LONG_SECS_2ND,
  "one{# second}",
  " other{# seconds}"
};
static const Pluralities IDS_DURATION_HOUR_1ST = {
  IDS_TIME_HOURS_1ST,
  "one{# hour and }",
  " other{# hours and }"
};
static const Pluralities IDS_LONG_MIN_2ND = {
  IDS_TIME_LONG_MINS_2ND,
  "one{# minute}",
  " other{# minutes}"
};
static const Pluralities IDS_DURATION_DAY_1ST = {
  IDS_TIME_DAYS_1ST,
  "one{# day and }",
  " other{# days and }"
};
static const Pluralities IDS_DURATION_HOUR_2ND = {
  IDS_TIME_HOURS_2ND,
  "one{# hour}",
  " other{# hours}"
};

namespace {

std::unique_ptr<icu::PluralRules> BuildPluralRules() {
  UErrorCode err = U_ZERO_ERROR;
  std::unique_ptr<icu::PluralRules> rules(
      icu::PluralRules::forLocale(icu::Locale::getDefault(), err));
  if (U_FAILURE(err)) {
    err = U_ZERO_ERROR;
    icu::UnicodeString fallback_rules("one: n is 1", -1, US_INV);
    rules.reset(icu::PluralRules::createRules(fallback_rules, err));
    DCHECK(U_SUCCESS(err));
  }
  return rules;
}

void FormatNumberInPlural(const icu::MessageFormat& format, int number,
                          icu::UnicodeString* result, UErrorCode* err) {
  if (U_FAILURE(*err)) return;
  icu::Formattable formattable(number);
  icu::FieldPosition ignore(icu::FieldPosition::DONT_CARE);
  format.format(&formattable, 1, *result, ignore, *err);
  DCHECK(U_SUCCESS(*err));
  return;
}

}  // namespace

Formatter::Formatter(const Pluralities& sec_pluralities,
                     const Pluralities& min_pluralities,
                     const Pluralities& hour_pluralities,
                     const Pluralities& day_pluralities) {
  simple_format_[UNIT_SEC] = InitFormat(sec_pluralities);
  simple_format_[UNIT_MIN] = InitFormat(min_pluralities);
  simple_format_[UNIT_HOUR] = InitFormat(hour_pluralities);
  simple_format_[UNIT_DAY] = InitFormat(day_pluralities);
}

Formatter::Formatter(const Pluralities& sec_pluralities,
                     const Pluralities& min_pluralities,
                     const Pluralities& hour_pluralities,
                     const Pluralities& day_pluralities,
                     const Pluralities& min_sec_pluralities1,
                     const Pluralities& min_sec_pluralities2,
                     const Pluralities& hour_min_pluralities1,
                     const Pluralities& hour_min_pluralities2,
                     const Pluralities& day_hour_pluralities1,
                     const Pluralities& day_hour_pluralities2) {
  simple_format_[UNIT_SEC] = InitFormat(sec_pluralities);
  simple_format_[UNIT_MIN] = InitFormat(min_pluralities);
  simple_format_[UNIT_HOUR] = InitFormat(hour_pluralities);
  simple_format_[UNIT_DAY] = InitFormat(day_pluralities);
  detailed_format_[TWO_UNITS_MIN_SEC][0] = InitFormat(min_sec_pluralities1);
  detailed_format_[TWO_UNITS_MIN_SEC][1] = InitFormat(min_sec_pluralities2);
  detailed_format_[TWO_UNITS_HOUR_MIN][0] = InitFormat(hour_min_pluralities1);
  detailed_format_[TWO_UNITS_HOUR_MIN][1] = InitFormat(hour_min_pluralities2);
  detailed_format_[TWO_UNITS_DAY_HOUR][0] = InitFormat(day_hour_pluralities1);
  detailed_format_[TWO_UNITS_DAY_HOUR][1] = InitFormat(day_hour_pluralities2);
}

void Formatter::Format(Unit unit,
                       int value,
                       icu::UnicodeString* formatted_string) const {
  DCHECK(simple_format_[unit]);
  DCHECK(formatted_string->isEmpty() == TRUE);
  UErrorCode error = U_ZERO_ERROR;
  FormatNumberInPlural(*simple_format_[unit],
                        value, formatted_string, &error);
  DCHECK(U_SUCCESS(error)) << "Error in icu::PluralFormat::format().";
  return;
}

void Formatter::Format(TwoUnits units,
                       int value_1,
                       int value_2,
                       icu::UnicodeString* formatted_string) const {
  DCHECK(detailed_format_[units][0])
      << "Detailed() not implemented for your (format, length) combination!";
  DCHECK(detailed_format_[units][1])
      << "Detailed() not implemented for your (format, length) combination!";
  DCHECK(formatted_string->isEmpty() == TRUE);
  UErrorCode error = U_ZERO_ERROR;
  FormatNumberInPlural(*detailed_format_[units][0], value_1,
                       formatted_string, &error);
  DCHECK(U_SUCCESS(error));
  FormatNumberInPlural(*detailed_format_[units][1], value_2,
                        formatted_string, &error);
  DCHECK(U_SUCCESS(error));
  return;
}

std::unique_ptr<icu::MessageFormat> Formatter::CreateFallbackFormat(
    const icu::PluralRules& rules,
    const Pluralities& pluralities) const {
  icu::UnicodeString pattern("{NUMBER, plural, ");
  if (rules.isKeyword(UNICODE_STRING_SIMPLE("one")))
    pattern += icu::UnicodeString(pluralities.fallback_one);
  pattern += icu::UnicodeString(pluralities.fallback_other);
  pattern.append(UChar(0x7du));  // "}" = U+007D

  UErrorCode error = U_ZERO_ERROR;
  std::unique_ptr<icu::MessageFormat> format(
      new icu::MessageFormat(pattern, error));
  DCHECK(U_SUCCESS(error));
  return format;
}

std::unique_ptr<icu::MessageFormat> Formatter::InitFormat(
    const Pluralities& pluralities) {
  if (!formatter_force_fallback) {
    base::string16 pattern = l10n_util::GetStringUTF16(pluralities.id);
    UErrorCode error = U_ZERO_ERROR;
    std::unique_ptr<icu::MessageFormat> format(new icu::MessageFormat(
        icu::UnicodeString(FALSE, pattern.data(), pattern.length()), error));
    DCHECK(U_SUCCESS(error));
    if (format.get())
      return format;
  }

  std::unique_ptr<icu::PluralRules> rules(BuildPluralRules());
  return CreateFallbackFormat(*rules, pluralities);
}

const Formatter* FormatterContainer::Get(TimeFormat::Format format,
                                         TimeFormat::Length length) const {
  return formatter_[format][length].get();
}

FormatterContainer::FormatterContainer() {
  Initialize();
}

FormatterContainer::~FormatterContainer() {
}

void FormatterContainer::Initialize() {
  formatter_[TimeFormat::FORMAT_ELAPSED][TimeFormat::LENGTH_SHORT].reset(
      new Formatter(IDS_ELAPSED_SHORT_SEC,
                    IDS_ELAPSED_SHORT_MIN,
                    IDS_ELAPSED_HOUR,
                    IDS_ELAPSED_DAY));
  formatter_[TimeFormat::FORMAT_ELAPSED][TimeFormat::LENGTH_LONG].reset(
      new Formatter(IDS_ELAPSED_LONG_SEC, IDS_ELAPSED_LONG_MIN,
                    IDS_ELAPSED_HOUR, IDS_ELAPSED_DAY));
  formatter_[TimeFormat::FORMAT_REMAINING][TimeFormat::LENGTH_SHORT].reset(
      new Formatter(IDS_REMAINING_SHORT_SEC,
                    IDS_REMAINING_SHORT_MIN,
                    IDS_REMAINING_HOUR,
                    IDS_REMAINING_DAY));
  formatter_[TimeFormat::FORMAT_REMAINING][TimeFormat::LENGTH_LONG].reset(
      new Formatter(IDS_REMAINING_LONG_SEC,
                    IDS_REMAINING_LONG_MIN,
                    IDS_REMAINING_HOUR,
                    IDS_REMAINING_DAY));
  formatter_[TimeFormat::FORMAT_DURATION][TimeFormat::LENGTH_SHORT].reset(
      new Formatter(IDS_DURATION_SHORT_SEC,
                    IDS_DURATION_SHORT_MIN,
                    IDS_DURATION_HOUR,
                    IDS_DURATION_DAY));
  formatter_[TimeFormat::FORMAT_DURATION][TimeFormat::LENGTH_LONG].reset(
      new Formatter(IDS_LONG_SEC,
                    IDS_LONG_MIN,
                    IDS_DURATION_HOUR,
                    IDS_DURATION_DAY,
                    IDS_LONG_MIN_1ST,
                    IDS_LONG_SEC_2ND,
                    IDS_DURATION_HOUR_1ST,
                    IDS_LONG_MIN_2ND,
                    IDS_DURATION_DAY_1ST,
                    IDS_DURATION_HOUR_2ND));
}

void FormatterContainer::Shutdown() {
  for (int format = 0; format < TimeFormat::FORMAT_COUNT; ++format) {
    for (int length = 0; length < TimeFormat::LENGTH_COUNT; ++length) {
      formatter_[format][length].reset();
    }
  }
}

}  // namespace ui
