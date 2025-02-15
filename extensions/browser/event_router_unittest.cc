// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/event_router.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/histogram_tester.h"
#include "base/values.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/testing_pref_store.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/event_listener_map.h"
#include "extensions/browser/extension_pref_value_map.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::DictionaryValue;
using base::ListValue;
using base::Value;

namespace extensions {

namespace {

// A simple mock to keep track of listener additions and removals.
class MockEventRouterObserver : public EventRouter::Observer {
 public:
  MockEventRouterObserver()
      : listener_added_count_(0),
        listener_removed_count_(0) {}
  virtual ~MockEventRouterObserver() {}

  int listener_added_count() const { return listener_added_count_; }
  int listener_removed_count() const { return listener_removed_count_; }
  const std::string& last_event_name() const { return last_event_name_; }

  void Reset() {
    listener_added_count_ = 0;
    listener_removed_count_ = 0;
    last_event_name_.clear();
  }

  // EventRouter::Observer overrides:
  void OnListenerAdded(const EventListenerInfo& details) override {
    listener_added_count_++;
    last_event_name_ = details.event_name;
  }

  void OnListenerRemoved(const EventListenerInfo& details) override {
    listener_removed_count_++;
    last_event_name_ = details.event_name;
  }

 private:
  int listener_added_count_;
  int listener_removed_count_;
  std::string last_event_name_;

  DISALLOW_COPY_AND_ASSIGN(MockEventRouterObserver);
};

typedef base::Callback<std::unique_ptr<EventListener>(
    const std::string&,           // event_name
    content::RenderProcessHost*,  // process
    base::DictionaryValue*        // filter (takes ownership)
    )>
    EventListenerConstructor;

std::unique_ptr<EventListener> CreateEventListenerForExtension(
    const std::string& extension_id,
    const std::string& event_name,
    content::RenderProcessHost* process,
    base::DictionaryValue* filter) {
  return EventListener::ForExtension(event_name, extension_id, process,
                                     base::WrapUnique(filter));
}

std::unique_ptr<EventListener> CreateEventListenerForURL(
    const GURL& listener_url,
    const std::string& event_name,
    content::RenderProcessHost* process,
    base::DictionaryValue* filter) {
  return EventListener::ForURL(event_name, listener_url, process,
                               base::WrapUnique(filter));
}

// Creates an extension.  If |component| is true, it is created as a component
// extension.  If |persistent| is true, it is created with a persistent
// background page; otherwise it is created with an event page.
scoped_refptr<Extension> CreateExtension(bool component, bool persistent) {
  ExtensionBuilder builder;
  std::unique_ptr<base::DictionaryValue> manifest =
      base::MakeUnique<base::DictionaryValue>();
  manifest->SetString("name", "foo");
  manifest->SetString("version", "1.0.0");
  manifest->SetInteger("manifest_version", 2);
  manifest->SetString("background.page", "background.html");
  manifest->SetBoolean("background.persistent", persistent);
  builder.SetManifest(std::move(manifest));
  if (component)
    builder.SetLocation(Manifest::Location::COMPONENT);

  return builder.Build();
}

std::unique_ptr<DictionaryValue> CreateHostSuffixFilter(
    const std::string& suffix) {
  std::unique_ptr<DictionaryValue> filter(new DictionaryValue());
  std::unique_ptr<ListValue> filter_list(new ListValue());
  std::unique_ptr<DictionaryValue> filter_dict(new DictionaryValue());

  filter_dict->Set("hostSuffix", base::MakeUnique<Value>(suffix));
  filter_list->Append(std::move(filter_dict));
  filter->Set("url", std::move(filter_list));

  return filter;
}

}  // namespace

class EventRouterTest : public ExtensionsTest {
 public:
  EventRouterTest() {}

 protected:
  // Tests adding and removing observers from EventRouter.
  void RunEventRouterObserverTest(const EventListenerConstructor& constructor);

  // Tests that the correct counts are recorded for the Extensions.Events
  // histograms.
  void ExpectHistogramCounts(int dispatch_count,
                             int component_count,
                             int persistent_count,
                             int suspended_count,
                             int component_suspended_count,
                             int running_count) {
    if (dispatch_count) {
      histogram_tester_.ExpectBucketCount("Extensions.Events.Dispatch",
                                          events::HistogramValue::FOR_TEST,
                                          dispatch_count);
    }
    if (component_count) {
      histogram_tester_.ExpectBucketCount(
          "Extensions.Events.DispatchToComponent",
          events::HistogramValue::FOR_TEST, component_count);
    }
    if (persistent_count) {
      histogram_tester_.ExpectBucketCount(
          "Extensions.Events.DispatchWithPersistentBackgroundPage",
          events::HistogramValue::FOR_TEST, persistent_count);
    }
    if (suspended_count) {
      histogram_tester_.ExpectBucketCount(
          "Extensions.Events.DispatchWithSuspendedEventPage",
          events::HistogramValue::FOR_TEST, suspended_count);
    }
    if (component_suspended_count) {
      histogram_tester_.ExpectBucketCount(
          "Extensions.Events.DispatchToComponentWithSuspendedEventPage",
          events::HistogramValue::FOR_TEST, component_suspended_count);
    }
    if (running_count) {
      histogram_tester_.ExpectBucketCount(
          "Extensions.Events.DispatchWithRunningEventPage",
          events::HistogramValue::FOR_TEST, running_count);
    }
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(EventRouterTest);
};

class EventRouterFilterTest : public ExtensionsTest {
 public:
  EventRouterFilterTest() {}

  void SetUp() override {
    ExtensionsTest::SetUp();

    render_process_host_.reset(
        new content::MockRenderProcessHost(browser_context()));

    // Set up all the dependencies of ExtensionPrefs.
    extension_pref_value_map_.reset(new ExtensionPrefValueMap());
    PrefServiceFactory factory;
    factory.set_user_prefs(new TestingPrefStore());
    factory.set_extension_prefs(new TestingPrefStore());
    user_prefs::PrefRegistrySyncable* pref_registry =
        new user_prefs::PrefRegistrySyncable();
    // Prefs should be registered before the PrefService is created.
    ExtensionPrefs::RegisterProfilePrefs(pref_registry);
    pref_service_ = factory.Create(pref_registry);

    std::unique_ptr<ExtensionPrefs> extension_prefs(ExtensionPrefs::Create(
        browser_context(), pref_service_.get(),
        browser_context()->GetPath().AppendASCII("Extensions"),
        extension_pref_value_map_.get(), false /* extensions_disabled */,
        std::vector<ExtensionPrefsObserver*>()));

    ExtensionPrefsFactory::GetInstance()->SetInstanceForTesting(
        browser_context(), std::move(extension_prefs));

    ASSERT_TRUE(EventRouter::Get(browser_context()));  // constructs EventRouter
  }

  void TearDown() override {
    render_process_host_.reset();
    ExtensionsTest::TearDown();
  }

  content::RenderProcessHost* render_process_host() const {
    return render_process_host_.get();
  }

  EventRouter* event_router() { return EventRouter::Get(browser_context()); }

  const DictionaryValue* GetFilteredEvents(const std::string& extension_id) {
    return event_router()->GetFilteredEvents(extension_id);
  }

  bool ContainsFilter(const std::string& extension_id,
                      const std::string& event_name,
                      const DictionaryValue& to_check) {
    const ListValue* filter_list = GetFilterList(extension_id, event_name);
    if (!filter_list) {
      ADD_FAILURE();
      return false;
    }

    for (size_t i = 0; i < filter_list->GetSize(); ++i) {
      const DictionaryValue* filter = nullptr;
      if (!filter_list->GetDictionary(i, &filter)) {
        ADD_FAILURE();
        return false;
      }
      if (filter->Equals(&to_check))
        return true;
    }
    return false;
  }

 private:
  const ListValue* GetFilterList(const std::string& extension_id,
                                 const std::string& event_name) {
    const base::DictionaryValue* filtered_events =
        GetFilteredEvents(extension_id);
    DictionaryValue::Iterator iter(*filtered_events);
    if (iter.key() != event_name)
      return nullptr;

    const base::ListValue* filter_list = nullptr;
    iter.value().GetAsList(&filter_list);
    return filter_list;
  }

  std::unique_ptr<content::RenderProcessHost> render_process_host_;
  std::unique_ptr<ExtensionPrefValueMap> extension_pref_value_map_;
  std::unique_ptr<PrefService> pref_service_;

  DISALLOW_COPY_AND_ASSIGN(EventRouterFilterTest);
};

TEST_F(EventRouterTest, GetBaseEventName) {
  // Normal event names are passed through unchanged.
  EXPECT_EQ("foo.onBar", EventRouter::GetBaseEventName("foo.onBar"));

  // Sub-events are converted to the part before the slash.
  EXPECT_EQ("foo.onBar", EventRouter::GetBaseEventName("foo.onBar/123"));
}

// Tests adding and removing observers from EventRouter.
void EventRouterTest::RunEventRouterObserverTest(
    const EventListenerConstructor& constructor) {
  EventRouter router(NULL, NULL);
  std::unique_ptr<EventListener> listener =
      constructor.Run("event_name", NULL, new base::DictionaryValue());

  // Add/remove works without any observers.
  router.OnListenerAdded(listener.get());
  router.OnListenerRemoved(listener.get());

  // Register observers that both match and don't match the event above.
  MockEventRouterObserver matching_observer;
  router.RegisterObserver(&matching_observer, "event_name");
  MockEventRouterObserver non_matching_observer;
  router.RegisterObserver(&non_matching_observer, "other");

  // Adding a listener notifies the appropriate observers.
  router.OnListenerAdded(listener.get());
  EXPECT_EQ(1, matching_observer.listener_added_count());
  EXPECT_EQ(0, non_matching_observer.listener_added_count());

  // Removing a listener notifies the appropriate observers.
  router.OnListenerRemoved(listener.get());
  EXPECT_EQ(1, matching_observer.listener_removed_count());
  EXPECT_EQ(0, non_matching_observer.listener_removed_count());

  // Adding the listener again notifies again.
  router.OnListenerAdded(listener.get());
  EXPECT_EQ(2, matching_observer.listener_added_count());
  EXPECT_EQ(0, non_matching_observer.listener_added_count());

  // Removing the listener again notifies again.
  router.OnListenerRemoved(listener.get());
  EXPECT_EQ(2, matching_observer.listener_removed_count());
  EXPECT_EQ(0, non_matching_observer.listener_removed_count());

  // Adding a listener with a sub-event notifies the main observer with
  // proper details.
  matching_observer.Reset();
  std::unique_ptr<EventListener> sub_event_listener =
      constructor.Run("event_name/1", NULL, new base::DictionaryValue());
  router.OnListenerAdded(sub_event_listener.get());
  EXPECT_EQ(1, matching_observer.listener_added_count());
  EXPECT_EQ(0, matching_observer.listener_removed_count());
  EXPECT_EQ("event_name/1", matching_observer.last_event_name());

  // Ditto for removing the listener.
  matching_observer.Reset();
  router.OnListenerRemoved(sub_event_listener.get());
  EXPECT_EQ(0, matching_observer.listener_added_count());
  EXPECT_EQ(1, matching_observer.listener_removed_count());
  EXPECT_EQ("event_name/1", matching_observer.last_event_name());
}

TEST_F(EventRouterTest, EventRouterObserverForExtensions) {
  RunEventRouterObserverTest(
      base::Bind(&CreateEventListenerForExtension, "extension_id"));
}

TEST_F(EventRouterTest, EventRouterObserverForURLs) {
  RunEventRouterObserverTest(
      base::Bind(&CreateEventListenerForURL, GURL("http://google.com/path")));
}

TEST_F(EventRouterTest, TestReportEvent) {
  EventRouter router(browser_context(), NULL);
  scoped_refptr<Extension> normal = test_util::CreateEmptyExtension("id1");
  router.ReportEvent(events::HistogramValue::FOR_TEST, normal.get(),
                     false /** did_enqueue */);
  ExpectHistogramCounts(1 /** Dispatch */, 0 /** DispatchToComponent */,
                        0 /** DispatchWithPersistentBackgroundPage */,
                        0 /** DispatchWithSuspendedEventPage */,
                        0 /** DispatchToComponentWithSuspendedEventPage */,
                        0 /** DispatchWithRunningEventPage */);

  scoped_refptr<Extension> component =
      CreateExtension(true /** component */, true /** persistent */);
  router.ReportEvent(events::HistogramValue::FOR_TEST, component.get(),
                     false /** did_enqueue */);
  ExpectHistogramCounts(2, 1, 1, 0, 0, 0);

  scoped_refptr<Extension> persistent = CreateExtension(false, true);
  router.ReportEvent(events::HistogramValue::FOR_TEST, persistent.get(),
                     false /** did_enqueue */);
  ExpectHistogramCounts(3, 1, 2, 0, 0, 0);

  scoped_refptr<Extension> event = CreateExtension(false, false);
  router.ReportEvent(events::HistogramValue::FOR_TEST, event.get(),
                     false /** did_enqueue */);
  ExpectHistogramCounts(4, 1, 2, 0, 0, 0);
  router.ReportEvent(events::HistogramValue::FOR_TEST, event.get(),
                     true /** did_enqueue */);
  ExpectHistogramCounts(5, 1, 2, 1, 0, 1);

  scoped_refptr<Extension> component_event = CreateExtension(true, false);
  router.ReportEvent(events::HistogramValue::FOR_TEST, component_event.get(),
                     false /** did_enqueue */);
  ExpectHistogramCounts(6, 2, 2, 1, 0, 2);
  router.ReportEvent(events::HistogramValue::FOR_TEST, component_event.get(),
                     true /** did_enqueue */);
  ExpectHistogramCounts(7, 3, 2, 2, 1, 2);
}

// Tests adding and removing events with filters.
TEST_F(EventRouterFilterTest, Basic) {
  // For the purpose of this test, "." is important in |event_name| as it
  // exercises the code path that uses |event_name| as a key in DictionaryValue.
  const std::string kEventName = "webNavigation.onBeforeNavigate";

  const std::string kExtensionId = "mbflcebpggnecokmikipoihdbecnjfoj";
  const std::string kHostSuffixes[] = {"foo.com", "bar.com", "baz.com"};
  std::vector<std::unique_ptr<DictionaryValue>> filters;
  for (size_t i = 0; i < arraysize(kHostSuffixes); ++i) {
    std::unique_ptr<base::DictionaryValue> filter =
        CreateHostSuffixFilter(kHostSuffixes[i]);
    EventRouter::Get(browser_context())
        ->AddFilteredEventListener(kEventName, render_process_host(),
                                   kExtensionId, *filter, true);
    filters.push_back(std::move(filter));
  }

  const base::DictionaryValue* filtered_events =
      GetFilteredEvents(kExtensionId);
  ASSERT_TRUE(filtered_events);
  ASSERT_EQ(1u, filtered_events->size());

  DictionaryValue::Iterator iter(*filtered_events);
  ASSERT_EQ(kEventName, iter.key());
  const base::ListValue* filter_list = nullptr;
  ASSERT_TRUE(iter.value().GetAsList(&filter_list));
  ASSERT_TRUE(filter_list);
  ASSERT_EQ(3u, filter_list->GetSize());

  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[0]));
  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[1]));
  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[2]));

  // Remove the second filter.
  event_router()->RemoveFilteredEventListener(kEventName, render_process_host(),
                                              kExtensionId, *filters[1], true);
  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[0]));
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[1]));
  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[2]));

  // Remove the first filter.
  event_router()->RemoveFilteredEventListener(kEventName, render_process_host(),
                                              kExtensionId, *filters[0], true);
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[0]));
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[1]));
  ASSERT_TRUE(ContainsFilter(kExtensionId, kEventName, *filters[2]));

  // Remove the third filter.
  event_router()->RemoveFilteredEventListener(kEventName, render_process_host(),
                                              kExtensionId, *filters[2], true);
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[0]));
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[1]));
  ASSERT_FALSE(ContainsFilter(kExtensionId, kEventName, *filters[2]));
}

}  // namespace extensions
