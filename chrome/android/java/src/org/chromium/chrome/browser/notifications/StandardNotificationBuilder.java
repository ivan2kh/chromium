// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.content.Context;
import android.os.Build;

import org.chromium.chrome.browser.AppHooks;

/**
 * Builds a notification using the standard Notification.BigTextStyle layout.
 */
public class StandardNotificationBuilder extends NotificationBuilderBase {
    private final Context mContext;

    public StandardNotificationBuilder(Context context) {
        super(context.getResources());
        mContext = context;
    }

    @Override
    public Notification build() {
        // Note: this is not a NotificationCompat builder so be mindful of the
        // API level of methods you call on the builder.
        // TODO(crbug.com/697104) We should probably use a Compat builder.
        ChromeNotificationBuilder builder = AppHooks.get().createChromeNotificationBuilder(
                false /* preferCompat */, NotificationConstants.CATEGORY_ID_SITES,
                mContext.getString(org.chromium.chrome.R.string.notification_category_sites),
                NotificationConstants.CATEGORY_GROUP_ID_GENERAL,
                mContext.getString(
                        org.chromium.chrome.R.string.notification_category_group_general));

        builder.setContentTitle(mTitle);
        builder.setContentText(mBody);
        builder.setSubText(mOrigin);
        builder.setTicker(mTickerText);
        if (mImage != null) {
            Notification.BigPictureStyle style =
                    new Notification.BigPictureStyle().bigPicture(mImage);
            if (Build.VERSION.CODENAME.equals("N")
                    || Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                // Android N doesn't show content text when expanded, so duplicate body text as a
                // summary for the big picture.
                style.setSummaryText(mBody);
            }
            builder.setStyle(style);
        } else {
            // If there is no image, let the body text wrap only multiple lines when expanded.
            builder.setStyle(new Notification.BigTextStyle().bigText(mBody));
        }
        builder.setLargeIcon(getNormalizedLargeIcon());
        setSmallIconOnBuilder(builder, mSmallIconId, mSmallIconBitmap);
        builder.setContentIntent(mContentIntent);
        builder.setDeleteIntent(mDeleteIntent);
        for (Action action : mActions) {
            addActionToBuilder(builder, action);
        }
        if (mSettingsAction != null) {
            addActionToBuilder(builder, mSettingsAction);
        }
        builder.setDefaults(mDefaults);
        builder.setVibrate(mVibratePattern);
        builder.setWhen(mTimestamp);
        builder.setOnlyAlertOnce(!mRenotify);
        setGroupOnBuilder(builder, mOrigin);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Public versions only supported since L, and createPublicNotification requires L+.
            builder.setPublicVersion(createPublicNotification(mContext));
        }
        return builder.build();
    }
}
