// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.bottombar.contextualsearch;

import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel.PanelState;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel.StateChangeReason;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchBlacklist.BlacklistReason;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchHeuristics;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchRankerLogger;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchRankerLoggerImpl;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchUma;
import org.chromium.chrome.browser.contextualsearch.QuickActionCategory;

import java.util.Locale;

/**
 * This class is responsible for all the logging related to Contextual Search.
 */
public class ContextualSearchPanelMetrics {
    private static final int MILLISECONDS_TO_NANOSECONDS = 1000000;

    // The Ranker logger to use to write Tap Suppression Ranker logs to UMA.
    private final ContextualSearchRankerLogger mTapSuppressionRankerLogger;

    // Flags for logging.
    private BlacklistReason mBlacklistReason;
    private boolean mDidSearchInvolvePromo;
    private boolean mWasSearchContentViewSeen;
    private boolean mIsPromoActive;
    private boolean mHasExpanded;
    private boolean mHasMaximized;
    private boolean mHasExitedPeeking;
    private boolean mHasExitedExpanded;
    private boolean mHasExitedMaximized;
    private boolean mIsSerpNavigation;
    private boolean mWasActivatedByTap;
    private boolean mIsSearchPanelFullyPreloaded;
    private boolean mWasIconSpriteAnimated;
    private boolean mWasPanelOpenedBeyondPeek;
    private boolean mWasSelectionPartOfUrl;
    private boolean mWasContextualCardsDataShown;
    private boolean mWasQuickActionShown;
    private int mQuickActionCategory;
    private boolean mWasQuickActionClicked;
    private boolean mWasSelectionAllCaps;
    private boolean mDidSelectionStartWithCapital;
    private char mSelectionFirstChar;
    private int mSelectionLength;
    // Whether any Tap suppression heuristic was satisfied when the panel was shown.
    private boolean mWasAnyHeuristicSatisfiedOnPanelShow;
    // Time when the panel was triggered (not reset by a chained search).
    // Panel transitions are animated so mPanelTriggerTimeNs will be less than mFirstPeekTimeNs.
    private long mPanelTriggerTimeFromTapNs;
    // Time when the panel peeks into view (not reset by a chained search).
    // Used to log total time the panel is showing (not closed).
    private long mFirstPeekTimeNs;
    // Time when the panel contents come into view (when the panel is opened).
    // Used to log preload effectiveness info -- additional time needed to fully render the
    // content in the overlay.
    private long mContentFirstViewTimeNs;
    // Time when a search request was started. Reset by chained searches.
    // Used to log the time it takes for a Search Result to become available.
    private long mSearchRequestStartTimeNs;
    // Time when the panel was opened beyond peeked. Reset when the panel is closed.
    // Used to log how long the panel was open.
    private long mPanelOpenedBeyondPeekTimeNs;
    // The current set of heuristics that should be logged with results seen when the panel closes.
    private ContextualSearchHeuristics mResultsSeenExperiments;
    // The current set of heuristics to be logged through ranker with results seen when the panel
    // closes.
    private ContextualSearchHeuristics mRankerLogExperiments;

    /**
     * Constructs an object to track metrics for the Contextual Search Overlay Panel.
     */
    ContextualSearchPanelMetrics() {
        mTapSuppressionRankerLogger = new ContextualSearchRankerLoggerImpl();
    }

    /**
     * Log information when the panel's state has changed.
     * @param fromState The state the panel is transitioning from.
     * @param toState The state that the panel is transitioning to.
     * @param reason The reason for the state change.
     */
    public void onPanelStateChanged(PanelState fromState, PanelState toState,
            StateChangeReason reason) {
        // Note: the logging within this function includes the promo, unless specifically
        // excluded.
        boolean isStartingSearch = isStartingNewContextualSearch(toState, reason);
        boolean isEndingSearch = isEndingContextualSearch(fromState, toState, isStartingSearch);
        boolean isChained = isStartingSearch && isOngoingContextualSearch(fromState);
        boolean isSameState = fromState == toState;
        boolean isFirstExitFromPeeking = fromState == PanelState.PEEKED && !mHasExitedPeeking
                && (!isSameState || isStartingSearch);
        boolean isFirstExitFromExpanded = fromState == PanelState.EXPANDED && !mHasExitedExpanded
                && !isSameState;
        boolean isFirstExitFromMaximized = fromState == PanelState.MAXIMIZED && !mHasExitedMaximized
                && !isSameState;
        boolean isFirstSearchView = isFirstExitFromPeeking && toState != PanelState.CLOSED;
        boolean isContentVisible =
                toState == PanelState.MAXIMIZED || toState == PanelState.EXPANDED;
        boolean isExitingPanelOpenedBeyondPeeked = mWasPanelOpenedBeyondPeek && !isContentVisible;
        // This variable is needed for logging and gets reset in an isStartingSearch block below,
        // so a local copy is created before the reset.
        boolean isSearchPanelFullyPreloaded = mIsSearchPanelFullyPreloaded;

        if (toState == PanelState.CLOSED && mPanelTriggerTimeFromTapNs != 0
                && reason == StateChangeReason.BASE_PAGE_SCROLL) {
            long durationMs =
                    (System.nanoTime() - mPanelTriggerTimeFromTapNs) / MILLISECONDS_TO_NANOSECONDS;
            ContextualSearchUma.logDurationBetweenTriggerAndScroll(
                    durationMs, mWasSearchContentViewSeen);
            mTapSuppressionRankerLogger.log(
                    ContextualSearchRankerLogger.Feature.DURATION_BEFORE_SCROLL_MS, durationMs);
        }

        if (isEndingSearch) {
            long durationMs = (System.nanoTime() - mFirstPeekTimeNs) / MILLISECONDS_TO_NANOSECONDS;
            ContextualSearchUma.logPanelViewDurationAction(durationMs);
            if (!mDidSearchInvolvePromo) {
                // Measure duration only when the promo is not involved.
                ContextualSearchUma.logDuration(mWasSearchContentViewSeen, isChained, durationMs);
            }
            if (mIsPromoActive) {
                // The user is exiting still in the promo, without choosing an option.
                ContextualSearchUma.logPromoSeen(mWasSearchContentViewSeen, mWasActivatedByTap);
            } else {
                ContextualSearchUma.logResultsSeen(mWasSearchContentViewSeen, mWasActivatedByTap);
            }

            if (mWasSelectionPartOfUrl) {
                ContextualSearchUma.logResultsSeenSelectionIsUrl(mWasSearchContentViewSeen,
                        mWasActivatedByTap);
            }

            if (mWasContextualCardsDataShown) {
                ContextualSearchUma.logContextualCardsResultsSeen(mWasSearchContentViewSeen);
            }
            if (mWasQuickActionShown) {
                ContextualSearchUma.logQuickActionResultsSeen(mWasSearchContentViewSeen,
                        mQuickActionCategory);
                ContextualSearchUma.logQuickActionClicked(mWasQuickActionClicked,
                        mQuickActionCategory);
                mTapSuppressionRankerLogger.log(
                        ContextualSearchRankerLogger.Feature.OUTCOME_WAS_QUICK_ACTION_CLICKED,
                        mWasQuickActionClicked);
            }

            if (mWasSelectionAllCaps && mWasActivatedByTap) {
                ContextualSearchUma.logAllCapsResultsSeen(mWasSearchContentViewSeen);
            } else if (mDidSelectionStartWithCapital && mWasActivatedByTap) {
                ContextualSearchUma.logStartedWithCapitalResultsSeen(mWasSearchContentViewSeen);
            }

            ContextualSearchUma.logBlacklistSeen(mBlacklistReason, mWasSearchContentViewSeen);

            ContextualSearchUma.logIconSpriteAnimated(mWasIconSpriteAnimated,
                    mWasSearchContentViewSeen, mWasActivatedByTap);

            if (mResultsSeenExperiments != null) {
                mResultsSeenExperiments.logResultsSeen(
                        mWasSearchContentViewSeen, mWasActivatedByTap);
                mResultsSeenExperiments = null;
            }

            if (mWasActivatedByTap) {
                boolean wasAnySuppressionHeuristicSatisfied =
                        mWasAnyHeuristicSatisfiedOnPanelShow || mWasSelectionPartOfUrl;
                ContextualSearchUma.logAnyTapSuppressionHeuristicSatisfied(
                        mWasSearchContentViewSeen, wasAnySuppressionHeuristicSatisfied);
                // Log all the experiments to the Ranker logger.
                if (mRankerLogExperiments != null) {
                    writeSelectionFeaturesToRanker();
                    mTapSuppressionRankerLogger.logOutcome(mWasSearchContentViewSeen);
                    mRankerLogExperiments.logRankerTapSuppression(mTapSuppressionRankerLogger);
                    mTapSuppressionRankerLogger.writeLogAndReset();
                    mRankerLogExperiments = null;
                }

                ContextualSearchUma.logSelectionLengthResultsSeen(
                        mWasSearchContentViewSeen, mSelectionLength);
            }
        }

        if (isExitingPanelOpenedBeyondPeeked) {
            assert mPanelOpenedBeyondPeekTimeNs != 0;
            long durationPanelOpen = (System.nanoTime() - mPanelOpenedBeyondPeekTimeNs)
                    / MILLISECONDS_TO_NANOSECONDS;
            ContextualSearchUma.logPanelOpenDuration(durationPanelOpen);
            mPanelOpenedBeyondPeekTimeNs = 0;
            mWasPanelOpenedBeyondPeek = false;
        }

        if (isStartingSearch) {
            mFirstPeekTimeNs = System.nanoTime();
            mContentFirstViewTimeNs = 0;
            mIsSearchPanelFullyPreloaded = false;
            mWasActivatedByTap = reason == StateChangeReason.TEXT_SELECT_TAP;
            mBlacklistReason = BlacklistReason.NONE;
            if (mWasActivatedByTap && mResultsSeenExperiments != null) {
                mWasAnyHeuristicSatisfiedOnPanelShow =
                        mResultsSeenExperiments.isAnyConditionSatisfiedForAggregrateLogging();
            } else {
                mWasAnyHeuristicSatisfiedOnPanelShow = false;
            }
        }
        if (isFirstSearchView) {
            onSearchPanelFirstView();
        }

        // Log state changes. We only log the first transition to a state within a contextual
        // search. Note that when a user clicks on a link on the search content view, this will
        // trigger a transition to MAXIMIZED (SERP_NAVIGATION) followed by a transition to
        // CLOSED (TAB_PROMOTION). For the purpose of logging, the reason for the second transition
        // is reinterpreted to SERP_NAVIGATION, in order to distinguish it from a tab promotion
        // caused when tapping on the Search Bar when the Panel is maximized.
        StateChangeReason reasonForLogging =
                mIsSerpNavigation ? StateChangeReason.SERP_NAVIGATION : reason;
        if (isStartingSearch || isEndingSearch
                || (!mHasExpanded && toState == PanelState.EXPANDED)
                || (!mHasMaximized && toState == PanelState.MAXIMIZED)) {
            ContextualSearchUma.logFirstStateEntry(fromState, toState, reasonForLogging);
        }
        // Note: CLOSED / UNDEFINED state exits are detected when a search that is not chained is
        // starting.
        if ((isStartingSearch && !isChained) || isFirstExitFromPeeking || isFirstExitFromExpanded
                || isFirstExitFromMaximized) {
            ContextualSearchUma.logFirstStateExit(fromState, toState, reasonForLogging);
        }
        // Log individual user actions so they can be sequenced.
        ContextualSearchUma.logPanelStateUserAction(toState, reasonForLogging);

        // We can now modify the state.
        if (isFirstExitFromPeeking) {
            mHasExitedPeeking = true;
        } else if (isFirstExitFromExpanded) {
            mHasExitedExpanded = true;
        } else if (isFirstExitFromMaximized) {
            mHasExitedMaximized = true;
        }

        if (toState == PanelState.EXPANDED) {
            mHasExpanded = true;
        } else if (toState == PanelState.MAXIMIZED) {
            mHasMaximized = true;
        }
        if (reason == StateChangeReason.SERP_NAVIGATION) {
            mIsSerpNavigation = true;
        }

        if (isEndingSearch) {
            if (mHasMaximized || mHasExpanded) {
                ContextualSearchUma.logSerpLoadedOnClose(isSearchPanelFullyPreloaded);
            }
            mDidSearchInvolvePromo = false;
            mWasSearchContentViewSeen = false;
            mHasExpanded = false;
            mHasMaximized = false;
            mHasExitedPeeking = false;
            mHasExitedExpanded = false;
            mHasExitedMaximized = false;
            mIsSerpNavigation = false;
            mWasSelectionPartOfUrl = false;
            mWasContextualCardsDataShown = false;
            mWasQuickActionShown = false;
            mQuickActionCategory = QuickActionCategory.NONE;
            mWasQuickActionClicked = false;
            mWasSelectionAllCaps = false;
            mDidSelectionStartWithCapital = false;
            mWasAnyHeuristicSatisfiedOnPanelShow = false;
            mPanelTriggerTimeFromTapNs = 0;
        }
    }

    /**
     * Sets the reason why the current selection was blacklisted.
     * @param reason The given reason.
     */
    public void setBlacklistReason(BlacklistReason reason) {
        mBlacklistReason = reason;
    }

    /**
     * Sets that the contextual search involved the promo.
     */
    public void setDidSearchInvolvePromo() {
        mDidSearchInvolvePromo = true;
    }

    /**
     * Sets that the Search Content View was seen.
     */
    public void setWasSearchContentViewSeen() {
        mWasSearchContentViewSeen = true;
        mWasPanelOpenedBeyondPeek = true;
        mPanelOpenedBeyondPeekTimeNs = System.nanoTime();
    }

    /**
     * Sets whether the promo is active.
     */
    public void setIsPromoActive(boolean shown) {
        mIsPromoActive = shown;
    }

    /**
     * @param wasIconSpriteAnimated Whether the search provider icon sprite was animated.
     */
    public void setWasIconSpriteAnimated(boolean wasIconSpriteAnimated) {
        mWasIconSpriteAnimated = wasIconSpriteAnimated;
    }

    /**
     * @param wasPartOfUrl Whether the selected text was part of a URL.
     */
    public void setWasSelectionPartOfUrl(boolean wasPartOfUrl) {
        mWasSelectionPartOfUrl = wasPartOfUrl;
    }

    /**
     * @param wasContextualCardsDataShown Whether Contextual Cards data was shown in the Contextual
     *                                    Search Bar.
     */
    public void setWasContextualCardsDataShown(boolean wasContextualCardsDataShown) {
        mWasContextualCardsDataShown = wasContextualCardsDataShown;
    }

    /**
     * @param wasQuickActionShown Whether a quick action was shown in the Contextual Search Bar.
     * @param quickActionCategory The {@link QuickActionCategory} for the quick action.
     */
    public void setWasQuickActionShown(boolean wasQuickActionShown, int quickActionCategory) {
        mWasQuickActionShown = wasQuickActionShown;
        if (mWasQuickActionShown) mQuickActionCategory = quickActionCategory;
    }

    /**
     * Sets |mWasQuickActionClicked| to true.
     */
    public void setWasQuickActionClicked() {
        mWasQuickActionClicked = true;
    }

    /**
     * Should be called when the panel first starts showing due to a tap.
     */
    public void onPanelTriggeredFromTap() {
        mPanelTriggerTimeFromTapNs = System.nanoTime();
    }

    /**
     * @param selection The text that is selected when a selection is established.
     */
    public void onSelectionEstablished(String selection) {
        mSelectionLength = selection.length();
        // In some locales, there is no concept of an upper or lower case letter. Account for this
        // by checking that the selected text is not equivalent to selection#toLowerCase().
        mWasSelectionAllCaps = selection.equals(selection.toUpperCase(Locale.getDefault()))
                && !selection.equals(selection.toLowerCase(Locale.getDefault()));
        mSelectionFirstChar = selection.charAt(0);
        String firstChar = String.valueOf(mSelectionFirstChar);
        mDidSelectionStartWithCapital = firstChar.equals(
                firstChar.toUpperCase(Locale.getDefault()))
                && !firstChar.equals(firstChar.toLowerCase(Locale.getDefault()));
    }

    /**
     * Writes the set of selection features that we've collected for Ranker to its log.
     */
    private void writeSelectionFeaturesToRanker() {
        mTapSuppressionRankerLogger.log(
                ContextualSearchRankerLogger.Feature.SELECTION_LENGTH, mSelectionLength);
        mTapSuppressionRankerLogger.log(
                ContextualSearchRankerLogger.Feature.SELECTION_FIRST_CHAR, mSelectionFirstChar);
        mTapSuppressionRankerLogger.log(
                ContextualSearchRankerLogger.Feature.SELECTION_WAS_ALL_CAPS, mWasSelectionAllCaps);
    }

    /**
     * Called to record the time when a search request started, for resolve and prefetch timing.
     */
    public void onSearchRequestStarted() {
        mSearchRequestStartTimeNs = System.nanoTime();
    }

    /**
     * Called when a Search Term has been resolved.
     */
    public void onSearchTermResolved() {
        long durationMs =
                (System.nanoTime() - mSearchRequestStartTimeNs) / MILLISECONDS_TO_NANOSECONDS;
        ContextualSearchUma.logSearchTermResolutionDuration(durationMs);
    }

    /**
     * Records timing information when the search results have fully loaded.
     * @param wasPrefetch Whether the request was prefetch-enabled.
     */
    public void onSearchResultsLoaded(boolean wasPrefetch) {
        if (mHasExpanded || mHasMaximized) {
            // Already opened, log how long it took.
            assert mContentFirstViewTimeNs != 0;
            long durationMs =
                    (System.nanoTime() - mContentFirstViewTimeNs) / MILLISECONDS_TO_NANOSECONDS;
            logSearchPanelLoadDuration(wasPrefetch, durationMs);
        }

        // Not yet opened, wait till an open to log.
        mIsSearchPanelFullyPreloaded = true;
    }

    /**
     * Called after the panel has navigated to prefetched Search Results.
     * This is the point where the search result starts to render in the panel.
     */
    public void onPanelNavigatedToPrefetchedSearch(boolean didResolve) {
        long durationMs =
                (System.nanoTime() - mSearchRequestStartTimeNs) / MILLISECONDS_TO_NANOSECONDS;
        ContextualSearchUma.logPrefetchedSearchNavigatedDuration(durationMs, didResolve);
    }

    /**
     * Sets the experiments to log with results seen.
     * @param resultsSeenExperiments The experiments to log when the panel results are known.
     */
    public void setResultsSeenExperiments(ContextualSearchHeuristics resultsSeenExperiments) {
        mResultsSeenExperiments = resultsSeenExperiments;
    }

    /**
     * Sets the experiments to log through Ranker with results seen.
     * @param rankerLogExperiments The experiments to log through ranker when the panel results
     *        are known.
     */
    public void setRankerLogExperiments(ContextualSearchHeuristics rankerLogExperiments) {
        mRankerLogExperiments = rankerLogExperiments;
    }

    /**
     * Records timing information when the search panel has been viewed for the first time.
     */
    private void onSearchPanelFirstView() {
        if (mIsSearchPanelFullyPreloaded) {
            // Already fully pre-loaded, record a wait of 0 milliseconds.
            logSearchPanelLoadDuration(true, 0);
        } else {
            // Start a loading timer.
            mContentFirstViewTimeNs = System.nanoTime();
        }
    }

    /**
     * Determine whether a new contextual search is starting.
     * @param toState The contextual search state that will be transitioned to.
     * @param reason The reason for the search state transition.
     * @return Whether a new contextual search is starting.
     */
    private boolean isStartingNewContextualSearch(PanelState toState, StateChangeReason reason) {
        return toState == PanelState.PEEKED
                && (reason == StateChangeReason.TEXT_SELECT_TAP
                        || reason == StateChangeReason.TEXT_SELECT_LONG_PRESS);
    }

    /**
     * Determine whether a contextual search is ending.
     * @param fromState The contextual search state that will be transitioned from.
     * @param toState The contextual search state that will be transitioned to.
     * @param isStartingSearch Whether a new contextual search is starting.
     * @return Whether a contextual search is ending.
     */
    private boolean isEndingContextualSearch(PanelState fromState, PanelState toState,
            boolean isStartingSearch) {
        return isOngoingContextualSearch(fromState)
                && (toState == PanelState.CLOSED || isStartingSearch);
    }

    /**
     * @param fromState The state the panel is transitioning from.
     * @return Whether there is an ongoing contextual search.
     */
    private boolean isOngoingContextualSearch(PanelState fromState) {
        return fromState != PanelState.UNDEFINED && fromState != PanelState.CLOSED;
    }

    /**
     * Logs the duration the user waited for the search panel to fully load, once it was opened.
     * @param wasPrefetch Whether the load included prefetch.
     * @param durationMs The duration to log.
     */
    private void logSearchPanelLoadDuration(boolean wasPrefetch, long durationMs) {
        ContextualSearchUma.logSearchPanelLoadDuration(wasPrefetch, durationMs);
    }
}

