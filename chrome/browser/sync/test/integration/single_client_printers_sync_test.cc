// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/printers_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "chromeos/printing/printer_configuration.h"

using printers_helper::AddPrinter;
using printers_helper::CreateTestPrinter;
using printers_helper::EditPrinterDescription;
using printers_helper::GetVerifierPrinterCount;
using printers_helper::GetVerifierPrinterStore;
using printers_helper::GetPrinterCount;
using printers_helper::GetPrinterStore;
using printers_helper::ProfileContainsSamePrintersAsVerifier;
using printers_helper::RemovePrinter;

class SingleClientPrintersSyncTest : public SyncTest {
 public:
  SingleClientPrintersSyncTest() : SyncTest(SINGLE_CLIENT) {}
  ~SingleClientPrintersSyncTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientPrintersSyncTest);
};

// Verify that printers aren't added with a sync call.
IN_PROC_BROWSER_TEST_F(SingleClientPrintersSyncTest, NoPrinters) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  EXPECT_TRUE(ProfileContainsSamePrintersAsVerifier(0));
}

// Verify syncing doesn't randomly remove a printer.
IN_PROC_BROWSER_TEST_F(SingleClientPrintersSyncTest, SingleNewPrinter) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_EQ(0, GetVerifierPrinterCount());

  AddPrinter(GetPrinterStore(0), printers_helper::CreateTestPrinter(0));
  AddPrinter(GetVerifierPrinterStore(), printers_helper::CreateTestPrinter(0));
  ASSERT_EQ(1, GetPrinterCount(0));
  ASSERT_EQ(1, GetVerifierPrinterCount());

  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  EXPECT_EQ(1, GetVerifierPrinterCount());
  EXPECT_TRUE(ProfileContainsSamePrintersAsVerifier(0));
}

// Verify editing a printer doesn't add it.
// Flaky on ChromeOS. http://crbug.com/701999
#if defined(OS_CHROMEOS)
#define MAYBE_EditPrinter DISABLED_EditPrinter
#else
#define MAYBE_EditPrinter EditPrinter
#endif
IN_PROC_BROWSER_TEST_F(SingleClientPrintersSyncTest, MAYBE_EditPrinter) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddPrinter(GetPrinterStore(0), printers_helper::CreateTestPrinter(0));
  AddPrinter(GetVerifierPrinterStore(), printers_helper::CreateTestPrinter(0));

  ASSERT_TRUE(
      EditPrinterDescription(GetPrinterStore(0), 0, "Updated description"));

  EXPECT_EQ(1, GetPrinterCount(0));
  EXPECT_EQ(1, GetVerifierPrinterCount());
  EXPECT_FALSE(ProfileContainsSamePrintersAsVerifier(0));
}

// Verify that removing a printer works.
IN_PROC_BROWSER_TEST_F(SingleClientPrintersSyncTest, RemovePrinter) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddPrinter(GetPrinterStore(0), printers_helper::CreateTestPrinter(0));
  EXPECT_EQ(1, GetPrinterCount(0));

  RemovePrinter(GetPrinterStore(0), 0);
  EXPECT_EQ(0, GetPrinterCount(0));
}

// Verify that merging data added before sync works.
// crbug.com/689662
#if defined(OS_CHROMEOS)
#define MAYBE_AddBeforeSetup DISABLED_AddBeforeSetup
#else
#define MAYBE_AddBeforeSetup AddBeforeSetup
#endif
IN_PROC_BROWSER_TEST_F(SingleClientPrintersSyncTest, MAYBE_AddBeforeSetup) {
  ASSERT_TRUE(SetupClients());

  AddPrinter(GetPrinterStore(0), printers_helper::CreateTestPrinter(0));
  EXPECT_EQ(1, GetPrinterCount(0));

  EXPECT_TRUE(SetupSync()) << "SetupSync() failed.";
}
