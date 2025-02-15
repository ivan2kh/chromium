// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.MeteringRectangle;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Range;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.Surface;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * This class implements Video Capture using Camera2 API, introduced in Android
 * API 21 (L Release). Capture takes place in the current Looper, while pixel
 * download takes place in another thread used by ImageReader. A number of
 * static methods are provided to retrieve information on current system cameras
 * and their capabilities, using android.hardware.camera2.CameraManager.
 **/
@JNINamespace("media")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class VideoCaptureCamera2 extends VideoCapture {
    // Inner class to extend a CameraDevice state change listener.
    private class CrStateListener extends CameraDevice.StateCallback {
        @Override
        public void onOpened(CameraDevice cameraDevice) {
            mCameraDevice = cameraDevice;
            changeCameraStateAndNotify(CameraState.CONFIGURING);
            if (!createPreviewObjects()) {
                changeCameraStateAndNotify(CameraState.STOPPED);
                nativeOnError(mNativeVideoCaptureDeviceAndroid, "Error configuring camera");
            }
        }

        @Override
        public void onDisconnected(CameraDevice cameraDevice) {
            cameraDevice.close();
            mCameraDevice = null;
            changeCameraStateAndNotify(CameraState.STOPPED);
        }

        @Override
        public void onError(CameraDevice cameraDevice, int error) {
            cameraDevice.close();
            mCameraDevice = null;
            changeCameraStateAndNotify(CameraState.STOPPED);
            nativeOnError(mNativeVideoCaptureDeviceAndroid,
                    "Camera device error " + Integer.toString(error));
        }
    };

    // Inner class to extend a Capture Session state change listener.
    private class CrPreviewSessionListener extends CameraCaptureSession.StateCallback {
        private final CaptureRequest mPreviewRequest;
        CrPreviewSessionListener(CaptureRequest previewRequest) {
            mPreviewRequest = previewRequest;
        }

        @Override
        public void onConfigured(CameraCaptureSession cameraCaptureSession) {
            Log.d(TAG, "onConfigured");
            mPreviewSession = cameraCaptureSession;
            try {
                // This line triggers the preview. No |listener| is registered, so we will not get
                // notified of capture events, instead, CrImageReaderListener will trigger every
                // time a downloaded image is ready. Since |handler| is null, we'll work on the
                // current Thread Looper.
                mPreviewSession.setRepeatingRequest(mPreviewRequest, null, null);
            } catch (CameraAccessException | SecurityException | IllegalStateException
                    | IllegalArgumentException ex) {
                Log.e(TAG, "setRepeatingRequest: ", ex);
                return;
            }
            // Now wait for trigger on CrImageReaderListener.onImageAvailable();
            nativeOnStarted(mNativeVideoCaptureDeviceAndroid);
            changeCameraStateAndNotify(CameraState.STARTED);
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
            // TODO(mcasas): When signalling error, C++ will tear us down. Is there need for
            // cleanup?
            changeCameraStateAndNotify(CameraState.STOPPED);
            nativeOnError(mNativeVideoCaptureDeviceAndroid, "Camera session configuration error");
        }
    };

    // Internal class implementing an ImageReader listener for Preview frames. Gets pinged when a
    // new frame is been captured and downloads it to memory-backed buffers.
    private class CrImageReaderListener implements ImageReader.OnImageAvailableListener {
        @Override
        public void onImageAvailable(ImageReader reader) {
            try (Image image = reader.acquireLatestImage()) {
                if (image == null) return;

                if (image.getFormat() != ImageFormat.YUV_420_888 || image.getPlanes().length != 3) {
                    nativeOnError(mNativeVideoCaptureDeviceAndroid, "Unexpected image format: "
                            + image.getFormat() + " or #planes: " + image.getPlanes().length);
                    throw new IllegalStateException();
                }

                if (reader.getWidth() != image.getWidth()
                        || reader.getHeight() != image.getHeight()) {
                    nativeOnError(mNativeVideoCaptureDeviceAndroid, "ImageReader size ("
                            + reader.getWidth() + "x" + reader.getHeight()
                            + ") did not match Image size (" + image.getWidth() + "x"
                            + image.getHeight() + ")");
                    throw new IllegalStateException();
                }

                nativeOnI420FrameAvailable(mNativeVideoCaptureDeviceAndroid,
                        image.getPlanes()[0].getBuffer(), image.getPlanes()[0].getRowStride(),
                        image.getPlanes()[1].getBuffer(), image.getPlanes()[2].getBuffer(),
                        image.getPlanes()[1].getRowStride(), image.getPlanes()[1].getPixelStride(),
                        image.getWidth(), image.getHeight(), getCameraRotation(),
                        image.getTimestamp());
            } catch (IllegalStateException ex) {
                Log.e(TAG, "acquireLatestImage():", ex);
            }
        }
    };

    // Inner class to extend a Photo Session state change listener.
    // Error paths must signal notifyTakePhotoError().
    private class CrPhotoSessionListener extends CameraCaptureSession.StateCallback {
        private final CaptureRequest mPhotoRequest;
        private final long mCallbackId;
        CrPhotoSessionListener(CaptureRequest photoRequest, long callbackId) {
            mPhotoRequest = photoRequest;
            mCallbackId = callbackId;
        }

        @Override
        public void onConfigured(CameraCaptureSession session) {
            Log.d(TAG, "onConfigured");
            try {
                // This line triggers a single photo capture. No |listener| is registered, so we
                // will get notified via a CrPhotoSessionListener. Since |handler| is null, we'll
                // work on the current Thread Looper.
                session.capture(mPhotoRequest, null, null);
            } catch (CameraAccessException e) {
                Log.e(TAG, "capture() error");
                notifyTakePhotoError(mCallbackId);
                return;
            }
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession session) {
            Log.e(TAG, "failed configuring capture session");
            notifyTakePhotoError(mCallbackId);
            return;
        }
    };

    // Internal class implementing an ImageReader listener for encoded Photos.
    // Gets pinged when a new Image is been captured.
    private class CrPhotoReaderListener implements ImageReader.OnImageAvailableListener {
        private final long mCallbackId;
        CrPhotoReaderListener(long callbackId) {
            mCallbackId = callbackId;
        }

        private byte[] readCapturedData(Image image) {
            byte[] capturedData = null;
            try {
                capturedData = image.getPlanes()[0].getBuffer().array();
            } catch (UnsupportedOperationException ex) {
                // Try reading the pixels in a different way.
                final ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                capturedData = new byte[buffer.remaining()];
                buffer.get(capturedData);
            } finally {
                return capturedData;
            }
        }

        @Override
        public void onImageAvailable(ImageReader reader) {
            try (Image image = reader.acquireLatestImage()) {
                if (image == null) {
                    throw new IllegalStateException();
                }

                if (image.getFormat() != ImageFormat.JPEG) {
                    Log.e(TAG, "Unexpected image format: %d", image.getFormat());
                    throw new IllegalStateException();
                }

                final byte[] capturedData = readCapturedData(image);
                nativeOnPhotoTaken(mNativeVideoCaptureDeviceAndroid, mCallbackId, capturedData);

            } catch (IllegalStateException ex) {
                notifyTakePhotoError(mCallbackId);
                return;
            }

            if (createPreviewObjects()) return;

            nativeOnError(mNativeVideoCaptureDeviceAndroid, "Error restarting preview");
        }
    };

    // Inner Runnable to restart capture, must be run on |mContext| looper.
    private final Runnable mRestartCapture = new Runnable() {
        @Override
        public void run() {
            mPreviewSession.close(); // Asynchronously kill the CaptureSession.
            createPreviewObjects();
        }
    };

    private static final double kNanoSecondsToFps = 1.0E-9;
    private static final String TAG = "VideoCapture";

    // Map of the equivalent color temperature in Kelvin for the White Balance setting. The
    // values are a mixture of educated guesses and data from Android's Camera2 API. The
    // temperatures must be ordered increasingly.
    private static final SparseIntArray COLOR_TEMPERATURES_MAP;
    static {
        COLOR_TEMPERATURES_MAP = new SparseIntArray();
        COLOR_TEMPERATURES_MAP.append(2850, CameraMetadata.CONTROL_AWB_MODE_INCANDESCENT);
        COLOR_TEMPERATURES_MAP.append(2940, CameraMetadata.CONTROL_AWB_MODE_WARM_FLUORESCENT);
        COLOR_TEMPERATURES_MAP.append(3000, CameraMetadata.CONTROL_AWB_MODE_TWILIGHT);
        COLOR_TEMPERATURES_MAP.append(4230, CameraMetadata.CONTROL_AWB_MODE_FLUORESCENT);
        COLOR_TEMPERATURES_MAP.append(6000, CameraMetadata.CONTROL_AWB_MODE_CLOUDY_DAYLIGHT);
        COLOR_TEMPERATURES_MAP.append(6504, CameraMetadata.CONTROL_AWB_MODE_DAYLIGHT);
        COLOR_TEMPERATURES_MAP.append(7000, CameraMetadata.CONTROL_AWB_MODE_SHADE);
    };

    private static enum CameraState { OPENING, CONFIGURING, STARTED, STOPPED }

    private final Object mCameraStateLock = new Object();

    private CameraDevice mCameraDevice;
    private CameraCaptureSession mPreviewSession;
    private CaptureRequest mPreviewRequest;
    private Handler mMainHandler;
    private ImageReader mImageReader = null;

    private Range<Integer> mAeFpsRange;
    private CameraState mCameraState = CameraState.STOPPED;
    private final float mMaxZoom;
    private Rect mCropRect = new Rect();
    private int mPhotoWidth;
    private int mPhotoHeight;
    private int mFocusMode = AndroidMeteringMode.CONTINUOUS;
    private int mExposureMode = AndroidMeteringMode.CONTINUOUS;
    private MeteringRectangle mAreaOfInterest;
    private int mExposureCompensation;
    private int mWhiteBalanceMode = AndroidMeteringMode.CONTINUOUS;
    private int mColorTemperature = -1;
    private int mIso;
    private boolean mRedEyeReduction;
    private int mFillLightMode = AndroidFillLightMode.OFF;

    // Service function to grab CameraCharacteristics and handle exceptions.
    private static CameraCharacteristics getCameraCharacteristics(Context appContext, int id) {
        final CameraManager manager =
                (CameraManager) appContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            return manager.getCameraCharacteristics(Integer.toString(id));
        } catch (CameraAccessException ex) {
            Log.e(TAG, "getCameraCharacteristics: ", ex);
        }
        return null;
    }

    // {@link nativeOnPhotoTaken()} needs to be called back if there's any
    // problem after {@link takePhoto()} has returned true.
    private void notifyTakePhotoError(long callbackId) {
        nativeOnPhotoTaken(mNativeVideoCaptureDeviceAndroid, callbackId, new byte[0]);
    }

    private boolean createPreviewObjects() {
        Log.d(TAG, "createPreviewObjects");
        if (mCameraDevice == null) return false;

        // Create an ImageReader and plug a thread looper into it to have
        // readback take place on its own thread.
        mImageReader = ImageReader.newInstance(mCaptureFormat.getWidth(),
                mCaptureFormat.getHeight(), mCaptureFormat.getPixelFormat(), 2 /* maxImages */);
        HandlerThread thread = new HandlerThread("CameraPreview");
        thread.start();
        final Handler backgroundHandler = new Handler(thread.getLooper());
        final CrImageReaderListener imageReaderListener = new CrImageReaderListener();
        mImageReader.setOnImageAvailableListener(imageReaderListener, backgroundHandler);

        // The Preview template specifically means "high frame rate is given
        // priority over the highest-quality post-processing".
        CaptureRequest.Builder previewRequestBuilder = null;
        try {
            previewRequestBuilder =
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        } catch (CameraAccessException | IllegalArgumentException | SecurityException ex) {
            Log.e(TAG, "createCaptureRequest: ", ex);
            return false;
        }
        if (previewRequestBuilder == null) {
            Log.e(TAG, "previewRequestBuilder error");
            return false;
        }
        // Construct an ImageReader Surface and plug it into our CaptureRequest.Builder.
        previewRequestBuilder.addTarget(mImageReader.getSurface());

        // A series of configuration options in the PreviewBuilder
        previewRequestBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
        previewRequestBuilder.set(
                CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_FAST);
        previewRequestBuilder.set(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_FAST);
        previewRequestBuilder.set(CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE,
                CameraMetadata.CONTROL_VIDEO_STABILIZATION_MODE_ON);

        configureCommonCaptureSettings(previewRequestBuilder);

        List<Surface> surfaceList = new ArrayList<Surface>(1);
        // TODO(mcasas): release this Surface when not needed, https://crbug.com/643884.
        surfaceList.add(mImageReader.getSurface());

        mPreviewRequest = previewRequestBuilder.build();
        final CrPreviewSessionListener captureSessionListener =
                new CrPreviewSessionListener(mPreviewRequest);
        try {
            mCameraDevice.createCaptureSession(surfaceList, captureSessionListener, null);
        } catch (CameraAccessException | IllegalArgumentException | SecurityException ex) {
            Log.e(TAG, "createCaptureSession: ", ex);
            return false;
        }
        // Wait for trigger on CrPreviewSessionListener.onConfigured();
        return true;
    }

    private void configureCommonCaptureSettings(CaptureRequest.Builder requestBuilder) {
        // |mFocusMode| indicates if we're in auto/continuous, single-shot or manual mode.
        if (mFocusMode == AndroidMeteringMode.CONTINUOUS) {
            requestBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                    CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
        } else if (mFocusMode == AndroidMeteringMode.SINGLE_SHOT) {
            requestBuilder.set(CaptureRequest.CONTROL_AF_MODE,
                    CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
            requestBuilder.set(
                    CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);
        } else if (mFocusMode == AndroidMeteringMode.FIXED) {
            requestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF);
            // TODO(mcasas): Support controlling LENS_FOCUS_DISTANCE in this mode,
            // https://crbug.com/518807.
        }

        // |mExposureMode| and |mFillLightMode| interact to configure the AE and Flash modes. In a
        // nutshell, FLASH_MODE is only effective if the auto-exposure is ON/OFF, otherwise the
        // auto-exposure related flash control (ON_{AUTO,ALWAYS}_FLASH{_REDEYE) takes priority.
        if (mExposureMode == AndroidMeteringMode.NONE
                || mExposureMode == AndroidMeteringMode.FIXED) {
            requestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF);
        } else {
            requestBuilder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON);
            requestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, mAeFpsRange);
        }
        switch (mFillLightMode) {
            case AndroidFillLightMode.OFF:
                requestBuilder.set(CaptureRequest.FLASH_MODE, CameraMetadata.FLASH_MODE_OFF);
                break;
            case AndroidFillLightMode.AUTO:
                // Setting the AE to CONTROL_AE_MODE_ON_AUTO_FLASH[_REDEYE] overrides FLASH_MODE.
                requestBuilder.set(CaptureRequest.CONTROL_AE_MODE, mRedEyeReduction
                                ? CameraMetadata.CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE
                                : CameraMetadata.CONTROL_AE_MODE_ON_AUTO_FLASH);
                break;
            case AndroidFillLightMode.FLASH:
                // Setting the AE to CONTROL_AE_MODE_ON_ALWAYS_FLASH overrides FLASH_MODE.
                requestBuilder.set(CaptureRequest.CONTROL_AE_MODE,
                        CameraMetadata.CONTROL_AE_MODE_ON_ALWAYS_FLASH);
                break;
            case AndroidFillLightMode.TORCH:
                requestBuilder.set(
                        CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON);
                requestBuilder.set(CaptureRequest.FLASH_MODE, CameraMetadata.FLASH_MODE_TORCH);
                break;
            case AndroidFillLightMode.NONE:
                // NONE is only used for getting capabilities, to signify "no flash unit". Ignore.
        }
        requestBuilder.set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, mExposureCompensation);

        // White Balance mode AndroidMeteringMode.SINGLE_SHOT is not supported.
        if (mWhiteBalanceMode == AndroidMeteringMode.CONTINUOUS) {
            requestBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, false);
            requestBuilder.set(
                    CaptureRequest.CONTROL_AWB_MODE, CameraMetadata.CONTROL_AWB_MODE_AUTO);
            // TODO(mcasas): support different luminant color temperatures, e.g. DAYLIGHT, SHADE.
            // https://crbug.com/518807
        } else if (mWhiteBalanceMode == AndroidMeteringMode.NONE) {
            requestBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, false);
            requestBuilder.set(
                    CaptureRequest.CONTROL_AWB_MODE, CameraMetadata.CONTROL_AWB_MODE_OFF);
        } else if (mWhiteBalanceMode == AndroidMeteringMode.FIXED) {
            requestBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, true);
            if (mColorTemperature > 0) {
                final int colorSetting = getClosestWhiteBalance(mColorTemperature);
                if (colorSetting != -1) {
                    requestBuilder.set(CaptureRequest.CONTROL_AWB_MODE, colorSetting);
                }
            }
        }

        if (mAreaOfInterest != null) {
            MeteringRectangle[] array = {mAreaOfInterest};
            Log.d(TAG, "Area of interest %s", mAreaOfInterest.toString());
            requestBuilder.set(CaptureRequest.CONTROL_AF_REGIONS, array);
            requestBuilder.set(CaptureRequest.CONTROL_AE_REGIONS, array);
            requestBuilder.set(CaptureRequest.CONTROL_AWB_REGIONS, array);
        }

        if (!mCropRect.isEmpty()) {
            requestBuilder.set(CaptureRequest.SCALER_CROP_REGION, mCropRect);
        }

        if (mIso > 0) requestBuilder.set(CaptureRequest.SENSOR_SENSITIVITY, mIso);
    }

    private void changeCameraStateAndNotify(CameraState state) {
        synchronized (mCameraStateLock) {
            mCameraState = state;
            mCameraStateLock.notifyAll();
        }
    }

    // Finds the closest Size to (|width|x|height|) in |sizes|, and returns it or null.
    // Ignores |width| or |height| if either is zero (== don't care).
    private static Size findClosestSizeInArray(Size[] sizes, int width, int height) {
        if (sizes == null) return null;
        Size closestSize = null;
        int minDiff = Integer.MAX_VALUE;
        for (Size size : sizes) {
            final int diff = ((width > 0) ? Math.abs(size.getWidth() - width) : 0)
                    + ((height > 0) ? Math.abs(size.getHeight() - height) : 0);
            if (diff < minDiff) {
                minDiff = diff;
                closestSize = size;
            }
        }
        if (minDiff == Integer.MAX_VALUE) {
            Log.e(TAG, "Couldn't find resolution close to (%dx%d)", width, height);
            return null;
        }
        return closestSize;
    }

    private int getClosestWhiteBalance(int colorTemperature) {
        int minDiff = Integer.MAX_VALUE;
        int matchedTemperature = -1;

        for (int i = 0; i < COLOR_TEMPERATURES_MAP.size(); ++i) {
            final int diff = Math.abs(colorTemperature - COLOR_TEMPERATURES_MAP.keyAt(i));
            if (diff >= minDiff) continue;
            minDiff = diff;
            matchedTemperature = COLOR_TEMPERATURES_MAP.valueAt(i);
        }
        return matchedTemperature;
    }

    static boolean isLegacyDevice(Context appContext, int id) {
        final CameraCharacteristics cameraCharacteristics =
                getCameraCharacteristics(appContext, id);
        return cameraCharacteristics != null
                && cameraCharacteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
                == CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
    }

    static int getNumberOfCameras(Context appContext) {
        final CameraManager manager =
                (CameraManager) appContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            return manager.getCameraIdList().length;
        } catch (CameraAccessException | SecurityException ex) {
            // SecurityException is undocumented but seen in the wild: https://crbug/605424.
            Log.e(TAG, "getNumberOfCameras: getCameraIdList(): ", ex);
            return 0;
        }
    }

    static int getCaptureApiType(int id, Context appContext) {
        final CameraCharacteristics cameraCharacteristics =
                getCameraCharacteristics(appContext, id);
        if (cameraCharacteristics == null) {
            return VideoCaptureApi.UNKNOWN;
        }

        final int supportedHWLevel =
                cameraCharacteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);
        switch (supportedHWLevel) {
            case CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
                return VideoCaptureApi.ANDROID_API2_LEGACY;
            case CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
                return VideoCaptureApi.ANDROID_API2_FULL;
            case CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
                return VideoCaptureApi.ANDROID_API2_LIMITED;
            default:
                return VideoCaptureApi.ANDROID_API2_LEGACY;
        }
    }

    static String getName(int id, Context appContext) {
        final CameraCharacteristics cameraCharacteristics =
                getCameraCharacteristics(appContext, id);
        if (cameraCharacteristics == null) return null;
        final int facing = cameraCharacteristics.get(CameraCharacteristics.LENS_FACING);
        return "camera2 " + id + ", facing "
                + ((facing == CameraCharacteristics.LENS_FACING_FRONT) ? "front" : "back");
    }

    static VideoCaptureFormat[] getDeviceSupportedFormats(Context appContext, int id) {
        final CameraCharacteristics cameraCharacteristics =
                getCameraCharacteristics(appContext, id);
        if (cameraCharacteristics == null) return null;

        final int[] capabilities =
                cameraCharacteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);
        // Per-format frame rate via getOutputMinFrameDuration() is only available if the
        // property REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR is set.
        boolean minFrameDurationAvailable = false;
        for (int cap : capabilities) {
            if (cap == CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR) {
                minFrameDurationAvailable = true;
                break;
            }
        }

        ArrayList<VideoCaptureFormat> formatList = new ArrayList<VideoCaptureFormat>();
        final StreamConfigurationMap streamMap =
                cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        final int[] formats = streamMap.getOutputFormats();
        for (int format : formats) {
            final Size[] sizes = streamMap.getOutputSizes(format);
            if (sizes == null) continue;
            for (Size size : sizes) {
                double minFrameRate = 0.0f;
                if (minFrameDurationAvailable) {
                    final long minFrameDuration = streamMap.getOutputMinFrameDuration(format, size);
                    minFrameRate = (minFrameDuration == 0)
                            ? 0.0f
                            : (1.0 / kNanoSecondsToFps * minFrameDuration);
                } else {
                    // TODO(mcasas): find out where to get the info from in this case.
                    // Hint: perhaps using SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS.
                    minFrameRate = 0.0;
                }
                formatList.add(new VideoCaptureFormat(
                        size.getWidth(), size.getHeight(), (int) minFrameRate, 0));
            }
        }
        return formatList.toArray(new VideoCaptureFormat[formatList.size()]);
    }

    VideoCaptureCamera2(Context context, int id, long nativeVideoCaptureDeviceAndroid) {
        super(context, id, nativeVideoCaptureDeviceAndroid);
        final CameraCharacteristics cameraCharacteristics = getCameraCharacteristics(context, id);
        mMaxZoom =
                cameraCharacteristics.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);
    }

    @Override
    public boolean allocate(int width, int height, int frameRate) {
        Log.d(TAG, "allocate: requested (%d x %d) @%dfps", width, height, frameRate);
        synchronized (mCameraStateLock) {
            if (mCameraState == CameraState.OPENING || mCameraState == CameraState.CONFIGURING) {
                Log.e(TAG, "allocate() invoked while Camera is busy opening/configuring.");
                return false;
            }
        }
        final CameraCharacteristics cameraCharacteristics = getCameraCharacteristics(mContext, mId);
        final StreamConfigurationMap streamMap =
                cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

        // Find closest supported size.
        final Size[] supportedSizes = streamMap.getOutputSizes(ImageFormat.YUV_420_888);
        final Size closestSupportedSize = findClosestSizeInArray(supportedSizes, width, height);
        if (closestSupportedSize == null) {
            Log.e(TAG, "No supported resolutions.");
            return false;
        }
        final List<Range<Integer>> fpsRanges = Arrays.asList(cameraCharacteristics.get(
                CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES));
        if (fpsRanges.isEmpty()) {
            Log.e(TAG, "No supported framerate ranges.");
            return false;
        }
        final List<FramerateRange> framerateRanges =
                new ArrayList<FramerateRange>(fpsRanges.size());
        // On some legacy implementations FPS values are multiplied by 1000. Multiply by 1000
        // everywhere for consistency. Set fpsUnitFactor to 1 if fps ranges are already multiplied
        // by 1000.
        final int fpsUnitFactor = fpsRanges.get(0).getUpper() > 1000 ? 1 : 1000;
        for (Range<Integer> range : fpsRanges) {
            framerateRanges.add(new FramerateRange(
                    range.getLower() * fpsUnitFactor, range.getUpper() * fpsUnitFactor));
        }
        final FramerateRange aeFramerateRange =
                getClosestFramerateRange(framerateRanges, frameRate * 1000);
        mAeFpsRange = new Range<Integer>(
                aeFramerateRange.min / fpsUnitFactor, aeFramerateRange.max / fpsUnitFactor);
        Log.d(TAG, "allocate: matched (%d x %d) @[%d - %d]", closestSupportedSize.getWidth(),
                closestSupportedSize.getHeight(), mAeFpsRange.getLower(), mAeFpsRange.getUpper());

        // |mCaptureFormat| is also used to configure the ImageReader.
        mCaptureFormat = new VideoCaptureFormat(closestSupportedSize.getWidth(),
                closestSupportedSize.getHeight(), frameRate, ImageFormat.YUV_420_888);
        mCameraNativeOrientation =
                cameraCharacteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
        // TODO(mcasas): The following line is correct for N5 with prerelease Build,
        // but NOT for N7 with a dev Build. Figure out which one to support.
        mInvertDeviceOrientationReadings =
                cameraCharacteristics.get(CameraCharacteristics.LENS_FACING)
                == CameraCharacteristics.LENS_FACING_BACK;
        return true;
    }

    @Override
    public boolean startCapture() {
        Log.d(TAG, "startCapture");
        changeCameraStateAndNotify(CameraState.OPENING);
        final CameraManager manager =
                (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);

        if (!mUseBackgroundThreadForTesting) {
            mMainHandler = new Handler(mContext.getMainLooper());
        } else {
            // Usually we deliver frames on |mContext|s thread, but unit tests
            // occupy its Looper; deliver frames on a background thread instead.
            HandlerThread thread = new HandlerThread("CameraPicture");
            thread.start();
            mMainHandler = new Handler(thread.getLooper());
        }

        final CrStateListener stateListener = new CrStateListener();
        try {
            manager.openCamera(Integer.toString(mId), stateListener, mMainHandler);
        } catch (CameraAccessException | IllegalArgumentException | SecurityException ex) {
            Log.e(TAG, "allocate: manager.openCamera: ", ex);
            return false;
        }

        return true;
    }

    @Override
    public boolean stopCapture() {
        Log.d(TAG, "stopCapture");

        // With Camera2 API, the capture is started asynchronously, which will cause problem if
        // stopCapture comes too quickly. Without stopping the previous capture properly, the next
        // startCapture will fail and make Chrome no-responding. So wait camera to be STARTED.
        synchronized (mCameraStateLock) {
            while (mCameraState != CameraState.STARTED && mCameraState != CameraState.STOPPED) {
                try {
                    mCameraStateLock.wait();
                } catch (InterruptedException ex) {
                    Log.e(TAG, "CaptureStartedEvent: ", ex);
                }
            }
            if (mCameraState == CameraState.STOPPED) return true;
        }

        try {
            mPreviewSession.abortCaptures();
        } catch (CameraAccessException | IllegalStateException ex) {
            // Stopping a device whose CameraCaptureSession is closed is not an error: ignore this.
            Log.w(TAG, "abortCaptures: ", ex);
        }
        if (mCameraDevice == null) return false;
        mCameraDevice.close();

        if (mUseBackgroundThreadForTesting) mMainHandler.getLooper().quit();

        changeCameraStateAndNotify(CameraState.STOPPED);
        mCropRect = new Rect();
        return true;
    }

    @Override
    public PhotoCapabilities getPhotoCapabilities() {
        final CameraCharacteristics cameraCharacteristics = getCameraCharacteristics(mContext, mId);
        PhotoCapabilities.Builder builder = new PhotoCapabilities.Builder();

        int minIso = 0;
        int maxIso = 0;
        final Range<Integer> iso_range =
                cameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE);
        if (iso_range != null) {
            minIso = iso_range.getLower();
            maxIso = iso_range.getUpper();
        }
        builder.setMinIso(minIso).setMaxIso(maxIso).setStepIso(1);
        builder.setCurrentIso(mPreviewRequest.get(CaptureRequest.SENSOR_SENSITIVITY));

        final StreamConfigurationMap streamMap =
                cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        final Size[] supportedSizes = streamMap.getOutputSizes(ImageFormat.JPEG);
        int minWidth = Integer.MAX_VALUE;
        int minHeight = Integer.MAX_VALUE;
        int maxWidth = 0;
        int maxHeight = 0;
        for (Size size : supportedSizes) {
            if (size.getWidth() < minWidth) minWidth = size.getWidth();
            if (size.getHeight() < minHeight) minHeight = size.getHeight();
            if (size.getWidth() > maxWidth) maxWidth = size.getWidth();
            if (size.getHeight() > maxHeight) maxHeight = size.getHeight();
        }
        builder.setMinHeight(minHeight).setMaxHeight(maxHeight).setStepHeight(1);
        builder.setMinWidth(minWidth).setMaxWidth(maxWidth).setStepWidth(1);
        builder.setCurrentHeight((mPhotoHeight > 0) ? mPhotoHeight : mCaptureFormat.getHeight());
        builder.setCurrentWidth((mPhotoWidth > 0) ? mPhotoWidth : mCaptureFormat.getWidth());

        final float currentZoom =
                cameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)
                        .width()
                / (float) mPreviewRequest.get(CaptureRequest.SCALER_CROP_REGION).width();
        // There is no min-zoom per se, so clamp it to always 1.
        builder.setMinZoom(1.0).setMaxZoom(mMaxZoom);
        builder.setCurrentZoom(currentZoom).setStepZoom(0.1);

        final int focusMode = mPreviewRequest.get(CaptureRequest.CONTROL_AF_MODE);
        // Classify the Focus capabilities. In CONTINUOUS and SINGLE_SHOT, we can call
        // autoFocus(AutoFocusCallback) to configure region(s) to focus onto.
        int jniFocusMode = AndroidMeteringMode.NONE;
        if (focusMode == CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_VIDEO
                || focusMode == CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
            jniFocusMode = AndroidMeteringMode.CONTINUOUS;
        } else if (focusMode == CameraMetadata.CONTROL_AF_MODE_AUTO
                || focusMode == CameraMetadata.CONTROL_AF_MODE_MACRO) {
            jniFocusMode = AndroidMeteringMode.SINGLE_SHOT;
        } else if (focusMode == CameraMetadata.CONTROL_AF_MODE_OFF) {
            jniFocusMode = AndroidMeteringMode.FIXED;
        } else {
            assert jniFocusMode == CameraMetadata.CONTROL_AF_MODE_EDOF;
        }
        builder.setFocusMode(jniFocusMode);

        int jniExposureMode = AndroidMeteringMode.CONTINUOUS;
        if (mPreviewRequest.get(CaptureRequest.CONTROL_AE_MODE)
                == CameraMetadata.CONTROL_AE_MODE_OFF) {
            jniExposureMode = AndroidMeteringMode.NONE;
        }
        if (mPreviewRequest.get(CaptureRequest.CONTROL_AE_LOCK)) {
            jniExposureMode = AndroidMeteringMode.FIXED;
        }
        builder.setExposureMode(jniExposureMode);

        final float step =
                cameraCharacteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP)
                        .floatValue();
        builder.setStepExposureCompensation(step);
        final Range<Integer> exposureCompensationRange =
                cameraCharacteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE);
        builder.setMinExposureCompensation(exposureCompensationRange.getLower() * step);
        builder.setMaxExposureCompensation(exposureCompensationRange.getUpper() * step);
        builder.setCurrentExposureCompensation(
                mPreviewRequest.get(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION) * step);

        final int whiteBalanceMode = mPreviewRequest.get(CaptureRequest.CONTROL_AWB_MODE);
        if (whiteBalanceMode == CameraMetadata.CONTROL_AWB_MODE_OFF) {
            builder.setWhiteBalanceMode(AndroidMeteringMode.NONE);
        } else if (whiteBalanceMode == CameraMetadata.CONTROL_AWB_MODE_AUTO) {
            builder.setWhiteBalanceMode(AndroidMeteringMode.CONTINUOUS);
        } else {
            builder.setWhiteBalanceMode(AndroidMeteringMode.FIXED);
        }
        builder.setMinColorTemperature(COLOR_TEMPERATURES_MAP.keyAt(0));
        builder.setMaxColorTemperature(
                COLOR_TEMPERATURES_MAP.keyAt(COLOR_TEMPERATURES_MAP.size() - 1));
        final int index = COLOR_TEMPERATURES_MAP.indexOfValue(whiteBalanceMode);
        if (index >= 0) {
            builder.setCurrentColorTemperature(COLOR_TEMPERATURES_MAP.keyAt(index));
        }
        builder.setStepColorTemperature(1);

        if (!cameraCharacteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE)) {
            builder.setFillLightMode(AndroidFillLightMode.NONE);
        } else {
            // CONTROL_AE_MODE overrides FLASH_MODE control unless it's in ON or OFF states.
            switch (mPreviewRequest.get(CaptureRequest.CONTROL_AE_MODE)) {
                case CameraMetadata.CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE:
                    builder.setRedEyeReduction(true);
                    builder.setFillLightMode(AndroidFillLightMode.AUTO);
                    break;
                case CameraMetadata.CONTROL_AE_MODE_ON_AUTO_FLASH:
                    builder.setFillLightMode(AndroidFillLightMode.AUTO);
                    break;
                case CameraMetadata.CONTROL_AE_MODE_ON_ALWAYS_FLASH:
                    builder.setFillLightMode(AndroidFillLightMode.FLASH);
                    break;
                case CameraMetadata.CONTROL_AE_MODE_OFF:
                case CameraMetadata.CONTROL_AE_MODE_ON:
                    final Integer flashMode = mPreviewRequest.get(CaptureRequest.FLASH_MODE);
                    if (flashMode == CameraMetadata.FLASH_MODE_OFF) {
                        builder.setFillLightMode(AndroidFillLightMode.OFF);
                    } else if (flashMode == CameraMetadata.FLASH_MODE_SINGLE) {
                        builder.setFillLightMode(AndroidFillLightMode.FLASH);
                    } else if (flashMode == CameraMetadata.FLASH_MODE_TORCH) {
                        builder.setFillLightMode(AndroidFillLightMode.TORCH);
                    }
                    break;
                default:
                    builder.setFillLightMode(AndroidFillLightMode.NONE);
            }
        }

        return builder.build();
    }

    @Override
    public void setPhotoOptions(double zoom, int focusMode, int exposureMode, double width,
            double height, float[] pointsOfInterest2D, boolean hasExposureCompensation,
            double exposureCompensation, int whiteBalanceMode, double iso,
            boolean hasRedEyeReduction, boolean redEyeReduction, int fillLightMode,
            double colorTemperature) {
        final CameraCharacteristics cameraCharacteristics = getCameraCharacteristics(mContext, mId);
        final Rect canvas =
                cameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);

        if (zoom != 0) {
            final float normalizedZoom = Math.max(1.0f, Math.min((float) zoom, mMaxZoom));
            final float cropFactor = (normalizedZoom - 1) / (2 * normalizedZoom);

            mCropRect = new Rect(Math.round(canvas.width() * cropFactor),
                    Math.round(canvas.height() * cropFactor),
                    Math.round(canvas.width() * (1 - cropFactor)),
                    Math.round(canvas.height() * (1 - cropFactor)));
            Log.d(TAG, "zoom level %f, rectangle: %s", normalizedZoom, mCropRect.toString());
        }

        if (focusMode != AndroidMeteringMode.NOT_SET) mFocusMode = focusMode;
        if (exposureMode != AndroidMeteringMode.NOT_SET) mExposureMode = exposureMode;
        if (whiteBalanceMode != AndroidMeteringMode.NOT_SET) mWhiteBalanceMode = whiteBalanceMode;

        if (width > 0) mPhotoWidth = (int) Math.round(width);
        if (height > 0) mPhotoHeight = (int) Math.round(height);

        // Upon new |zoom| configuration, clear up the previous |mAreaOfInterest| if any.
        if (mAreaOfInterest != null && !mAreaOfInterest.getRect().isEmpty() && zoom > 0) {
            mAreaOfInterest = null;
        }
        // Also clear |mAreaOfInterest| if the user sets it as NONE.
        if (mFocusMode == AndroidMeteringMode.NONE || mExposureMode == AndroidMeteringMode.NONE) {
            mAreaOfInterest = null;
        }
        // Update |mAreaOfInterest| if the camera supports and there are |pointsOfInterest2D|.
        final boolean pointsOfInterestSupported =
                cameraCharacteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF) > 0
                || cameraCharacteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE) > 0
                || cameraCharacteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AWB) > 0;
        if (pointsOfInterestSupported && pointsOfInterest2D.length > 0) {
            assert pointsOfInterest2D.length == 1 : "Only 1 point of interest supported";
            assert pointsOfInterest2D[0] <= 1.0 && pointsOfInterest2D[0] >= 0.0;
            assert pointsOfInterest2D[1] <= 1.0 && pointsOfInterest2D[1] >= 0.0;
            // Calculate a Rect of 1/8 the |visibleRect| dimensions, and center it w.r.t. |canvas|.
            final Rect visibleRect = (mCropRect.isEmpty()) ? canvas : mCropRect;
            int centerX = Math.round(pointsOfInterest2D[0] * visibleRect.width());
            int centerY = Math.round(pointsOfInterest2D[1] * visibleRect.height());
            if (visibleRect.equals(mCropRect)) {
                centerX += (canvas.width() - visibleRect.width()) / 2;
                centerY += (canvas.height() - visibleRect.height()) / 2;
            }
            final int regionWidth = visibleRect.width() / 8;
            final int regionHeight = visibleRect.height() / 8;

            mAreaOfInterest = new MeteringRectangle(Math.max(0, centerX - regionWidth / 2),
                    Math.max(0, centerY - regionHeight / 2), regionWidth, regionHeight,
                    MeteringRectangle.METERING_WEIGHT_MAX);

            Log.d(TAG, "Calculating (%.2fx%.2f) wrt to %s (canvas being %s)", pointsOfInterest2D[0],
                    pointsOfInterest2D[1], visibleRect.toString(), canvas.toString());
            Log.d(TAG, "Area of interest %s", mAreaOfInterest.toString());
        }

        if (hasExposureCompensation) {
            mExposureCompensation = (int) Math.round(exposureCompensation
                    / cameraCharacteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP)
                              .floatValue());
        }
        if (iso > 0) mIso = (int) Math.round(iso);
        if (mWhiteBalanceMode == AndroidMeteringMode.FIXED && colorTemperature > 0) {
            mColorTemperature = (int) Math.round(colorTemperature);
        }
        if (fillLightMode != AndroidFillLightMode.NOT_SET) mFillLightMode = fillLightMode;

        final Handler mainHandler = new Handler(mContext.getMainLooper());
        mainHandler.removeCallbacks(mRestartCapture);
        mainHandler.post(mRestartCapture);
    }

    @Override
    public boolean takePhoto(final long callbackId) {
        Log.d(TAG, "takePhoto");
        if (mCameraDevice == null || mCameraState != CameraState.STARTED) return false;

        final CameraCharacteristics cameraCharacteristics = getCameraCharacteristics(mContext, mId);
        final StreamConfigurationMap streamMap =
                cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        final Size[] supportedSizes = streamMap.getOutputSizes(ImageFormat.JPEG);
        final Size closestSize = findClosestSizeInArray(supportedSizes, mPhotoWidth, mPhotoHeight);

        Log.d(TAG, "requested resolution: (%dx%d)", mPhotoWidth, mPhotoHeight);
        if (closestSize != null) {
            Log.d(TAG, " matched (%dx%d)", closestSize.getWidth(), closestSize.getHeight());
        }
        final ImageReader imageReader = ImageReader.newInstance(
                (closestSize != null) ? closestSize.getWidth() : mCaptureFormat.getWidth(),
                (closestSize != null) ? closestSize.getHeight() : mCaptureFormat.getHeight(),
                ImageFormat.JPEG, 1 /* maxImages */);

        HandlerThread thread = new HandlerThread("CameraPicture");
        thread.start();
        final Handler backgroundHandler = new Handler(thread.getLooper());

        final CrPhotoReaderListener photoReaderListener = new CrPhotoReaderListener(callbackId);
        imageReader.setOnImageAvailableListener(photoReaderListener, backgroundHandler);

        final List<Surface> surfaceList = new ArrayList<Surface>(1);
        // TODO(mcasas): release this Surface when not needed, https://crbug.com/643884.
        surfaceList.add(imageReader.getSurface());

        CaptureRequest.Builder photoRequestBuilder = null;
        try {
            photoRequestBuilder =
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
        } catch (CameraAccessException e) {
            Log.e(TAG, "mCameraDevice.createCaptureRequest() error");
            return false;
        }
        if (photoRequestBuilder == null) {
            Log.e(TAG, "photoRequestBuilder error");
            return false;
        }
        photoRequestBuilder.addTarget(imageReader.getSurface());
        photoRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, getCameraRotation());

        configureCommonCaptureSettings(photoRequestBuilder);

        final CaptureRequest photoRequest = photoRequestBuilder.build();
        final CrPhotoSessionListener sessionListener =
                new CrPhotoSessionListener(photoRequest, callbackId);
        try {
            mCameraDevice.createCaptureSession(surfaceList, sessionListener, backgroundHandler);
        } catch (CameraAccessException | IllegalArgumentException | SecurityException ex) {
            Log.e(TAG, "createCaptureSession: " + ex);
            return false;
        }
        return true;
    }

    @Override
    public void deallocate() {
        Log.d(TAG, "deallocate");
    }
}
