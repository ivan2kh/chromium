// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_THREAT_DETAILS_H_
#define CHROME_BROWSER_SAFE_BROWSING_THREAT_DETAILS_H_

// A class that encapsulates the detailed threat reports sent when
// users opt-in to do so from the safe browsing warning page.

// An instance of this class is generated when a safe browsing warning page
// is shown (SafeBrowsingBlockingPage).

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "components/safe_browsing/common/safebrowsing_types.h"
#include "components/safe_browsing/csd.pb.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/base/completion_callback.h"

namespace net {
class URLRequestContextGetter;
}

class Profile;
struct SafeBrowsingHostMsg_ThreatDOMDetails_Node;

namespace safe_browsing {

// Maps a URL to its Resource.
class ThreatDetailsCacheCollector;
class ThreatDetailsRedirectsCollector;
class ThreatDetailsFactory;

using ResourceMap =
    base::hash_map<std::string,
                   std::unique_ptr<ClientSafeBrowsingReportRequest::Resource>>;

// Maps a key of an HTML element to its corresponding HTMLElement proto message.
// HTML Element keys have the form "<frame_id>-<node_id>", where |frame_id| is
// the FrameTree NodeID of the render frame containing the element, and
// |node_id| is a seqeuntial ID for the element generated by the renderer.
using ElementMap = base::hash_map<std::string, std::unique_ptr<HTMLElement>>;

// Maps a URL to some HTML Elements. Used to maintain parent/child relationship
// for HTML Elements across IFrame boundaries.
// The key is the string URL set as the src attribute of an iframe. The value is
// the HTMLElement proto that represents the iframe element with that URL.
// The HTMLElement protos are not owned by this map.
using UrlToDomElementMap = base::hash_map<std::string, HTMLElement*>;

// Maps a URL to some Element IDs. Used to maintain parent/child relationship
// for HTML Elements across IFrame boundaries.
// The key is the string URL of a render frame. The value is the set of Element
// IDs that are at the top-level of this render frame.
using UrlToChildIdsMap = base::hash_map<std::string, std::unordered_set<int>>;

class ThreatDetails : public base::RefCountedThreadSafe<
                          ThreatDetails,
                          content::BrowserThread::DeleteOnUIThread>,
                      public content::WebContentsObserver {
 public:
  typedef security_interstitials::UnsafeResource UnsafeResource;

  // Constructs a new ThreatDetails instance, using the factory.
  static ThreatDetails* NewThreatDetails(BaseUIManager* ui_manager,
                                         content::WebContents* web_contents,
                                         const UnsafeResource& resource);

  // Makes the passed |factory| the factory used to instanciate
  // SafeBrowsingBlockingPage objects. Useful for tests.
  static void RegisterFactory(ThreatDetailsFactory* factory) {
    factory_ = factory;
  }

  // The SafeBrowsingBlockingPage calls this from the IO thread when
  // the user is leaving the blocking page and has opted-in to sending
  // the report. We start the redirection urls collection from history service
  // in UI thread; then do cache collection back in IO thread. We also record
  // if the user did proceed with the warning page, and how many times user
  // visited this page before. When we are done, we send the report.
  void FinishCollection(bool did_proceed, int num_visits);

  void OnCacheCollectionReady();

  void OnRedirectionCollectionReady();

  // content::WebContentsObserver implementation.
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 protected:
  friend class ThreatDetailsFactoryImpl;
  friend class TestThreatDetailsFactory;

  ThreatDetails(BaseUIManager* ui_manager,
                content::WebContents* web_contents,
                const UnsafeResource& resource);

  ~ThreatDetails() override;

  // Called on the IO thread with the DOM details.
  virtual void AddDOMDetails(
      const int frame_tree_node_id,
      const GURL& frame_last_committed_url,
      const std::vector<SafeBrowsingHostMsg_ThreatDOMDetails_Node>& params);

  Profile* profile_;

  // The report protocol buffer.
  std::unique_ptr<ClientSafeBrowsingReportRequest> report_;

  // Used to get a pointer to the HTTP cache.
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

 private:
  friend class base::RefCountedThreadSafe<ThreatDetails>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<ThreatDetails>;

  // Starts the collection of the report.
  void StartCollection();

  // Whether the url is "public" so we can add it to the report.
  bool IsReportableUrl(const GURL& url) const;

  // Finds an existing Resource for the given url, or creates a new one if not
  // found, and adds it to |resources_|. Returns the found/created resource.
  ClientSafeBrowsingReportRequest::Resource* FindOrCreateResource(
      const GURL& url);

  // Finds an existing HTMLElement for a given key, or creates a new one if not
  // found and adds it to |elements_|. Returns the found/created element.
  HTMLElement* FindOrCreateElement(const std::string& element_key);

  // Adds a Resource to resources_ with the given parent-child
  // relationship. |parent| and |tagname| can be empty, |children| can be NULL.
  // Returns the Resource that was affected, or null if no work was done.
  ClientSafeBrowsingReportRequest::Resource* AddUrl(
      const GURL& url,
      const GURL& parent,
      const std::string& tagname,
      const std::vector<GURL>* children);

  // Message handler.
  void OnReceivedThreatDOMDetails(
      content::RenderFrameHost* sender,
      const std::vector<SafeBrowsingHostMsg_ThreatDOMDetails_Node>& params);

  void AddRedirectUrlList(const std::vector<GURL>& urls);

  // Adds an HTML Element to the DOM structure.
  // |frame_tree_node_id| is the unique ID of the render frame the element came
  // from. |frame_url| is the URL that the render frame was handling.
  // |element_node_id| is a unique ID of the element within the render frame.
  // |tag_name| is the tag of the element. |parent_element_node_id| is the
  // unique ID of the parent element with the render frame. |attributes|
  // containes the names and values of the element's attributes.|resource| is
  // set if this element is a resource.
  void AddDomElement(const int frame_tree_node_id,
                     const std::string& frame_url,
                     const int element_node_id,
                     const std::string& tag_name,
                     const int parent_element_node_id,
                     const std::vector<AttributeNameValue>& attributes,
                     const ClientSafeBrowsingReportRequest::Resource* resource);

  scoped_refptr<BaseUIManager> ui_manager_;

  const UnsafeResource resource_;

  // For every Url we collect we create a Resource message. We keep
  // them in a map so we can avoid duplicates.
  ResourceMap resources_;

  // Store all HTML elements collected, keep them in a map for easy lookup.
  ElementMap elements_;

  // For each iframe element encountered we map the src of the iframe to the
  // iframe element. This is used when we receive elements from a different
  // frame whose document URL matches the src of an iframe in this map. We can
  // then add all elements from the subframe as children of the iframe element
  // stored here.
  UrlToDomElementMap iframe_src_to_element_map_;

  // When getting a set of elements from a render frame, we store the frame's
  // URL and a collection of all the top-level elements in that frame. When we
  // later encounter the parent iframe with the same src URL, we can add all of
  // these elements as children of that iframe.
  UrlToChildIdsMap document_url_to_children_map_;

  // Result from the cache extractor.
  bool cache_result_;

  // Whether user did proceed with the safe browsing blocking page or
  // not.
  bool did_proceed_;

  // How many times this user has visited this page before.
  int num_visits_;

  // Keeps track of whether we have an ambiguous DOM in this report. This can
  // happen when the HTML Elements returned by a render frame can't be
  // associated with a parent Element in the parent frame.
  bool ambiguous_dom_;

  // The factory used to instanciate SafeBrowsingBlockingPage objects.
  // Usefull for tests, so they can provide their own implementation of
  // SafeBrowsingBlockingPage.
  static ThreatDetailsFactory* factory_;

  // Used to collect details from the HTTP Cache.
  scoped_refptr<ThreatDetailsCacheCollector> cache_collector_;

  // Used to collect redirect urls from the history service
  scoped_refptr<ThreatDetailsRedirectsCollector> redirects_collector_;

  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, HistoryServiceUrls);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, HttpsResourceSanitization);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, HTTPCacheNoEntries);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, HTTPCache);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, ThreatDOMDetails_AmbiguousDOM);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, ThreatDOMDetails_MultipleFrames);
  FRIEND_TEST_ALL_PREFIXES(ThreatDetailsTest, ThreatDOMDetails);

  DISALLOW_COPY_AND_ASSIGN(ThreatDetails);
};

// Factory for creating ThreatDetails.  Useful for tests.
class ThreatDetailsFactory {
 public:
  virtual ~ThreatDetailsFactory() {}

  virtual ThreatDetails* CreateThreatDetails(
      BaseUIManager* ui_manager,
      content::WebContents* web_contents,
      const SafeBrowsingUIManager::UnsafeResource& unsafe_resource) = 0;
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_THREAT_DETAILS_H_
