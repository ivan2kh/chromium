// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaintArtifactCompositor_h
#define PaintArtifactCompositor_h

#include "base/memory/ref_counted.h"
#include "platform/PlatformExport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/graphics/GraphicsLayerClient.h"
#include "platform/graphics/paint/PaintController.h"
#include "wtf/Noncopyable.h"
#include "wtf/PtrUtil.h"
#include "wtf/Vector.h"
#include <memory>

namespace cc {
class Layer;
}

namespace gfx {
class Vector2dF;
}

namespace blink {

class GeometryMapper;
class JSONObject;
class PaintArtifact;
class WebLayer;
struct PaintChunk;

// Responsible for managing compositing in terms of a PaintArtifact.
//
// Owns a subtree of the compositor layer tree, and updates it in response to
// changes in the paint artifact.
//
// PaintArtifactCompositor is the successor to PaintLayerCompositor, reflecting
// the new home of compositing decisions after paint in Slimming Paint v2.
class PLATFORM_EXPORT PaintArtifactCompositor {
  WTF_MAKE_NONCOPYABLE(PaintArtifactCompositor);

 public:
  ~PaintArtifactCompositor();

  static std::unique_ptr<PaintArtifactCompositor> create() {
    return WTF::wrapUnique(new PaintArtifactCompositor());
  }

  // Updates the layer tree to match the provided paint artifact.
  // If |storeDebugInfo| is true, stores detailed debugging information in
  // the layers that will be output as part of a call to layersAsJSON
  // (if LayerTreeIncludesDebugInfo is specified).
  void update(
      const PaintArtifact&,
      RasterInvalidationTrackingMap<const PaintChunk>* paintChunkInvalidations,
      bool storeDebugInfo,
      GeometryMapper&);

  // The root layer of the tree managed by this object.
  cc::Layer* rootLayer() const { return m_rootLayer.get(); }

  // Wraps rootLayer(), so that it can be attached as a child of another
  // WebLayer.
  WebLayer* getWebLayer() const { return m_webLayer.get(); }

  // Returns extra information recorded during unit tests.
  // While not part of the normal output of this class, this provides a simple
  // way of locating the layers of interest, since there are still a slew of
  // placeholder layers required.
  struct ExtraDataForTesting {
    Vector<scoped_refptr<cc::Layer>> contentLayers;
  };
  void enableExtraDataForTesting() { m_extraDataForTestingEnabled = true; }
  ExtraDataForTesting* getExtraDataForTesting() const {
    return m_extraDataForTesting.get();
  }

  void setTracksRasterInvalidations(bool);
  void resetTrackedRasterInvalidations();
  bool hasTrackedRasterInvalidations() const;

  std::unique_ptr<JSONObject> layersAsJSON(LayerTreeFlags) const;

#ifndef NDEBUG
  void showDebugData();
#endif

 private:
  // A pending layer is a collection of paint chunks that will end up in
  // the same cc::Layer.
  struct PLATFORM_EXPORT PendingLayer {
    PendingLayer(const PaintChunk& firstPaintChunk, bool chunkIsForeign);
    // Merge another pending layer after this one, appending all its paint
    // chunks after chunks in this layer, with appropriate space conversion
    // applied. The merged layer must have a property tree state that's deeper
    // than this layer, i.e. can "upcast" to this layer's state.
    void merge(const PendingLayer& guest, GeometryMapper&);
    bool canMerge(const PendingLayer& guest) const;
    // Mutate this layer's property tree state to a more general (shallower)
    // state, thus the name "upcast". The concrete effect of this is to
    // "decomposite" some of the properties, so that fewer properties will be
    // applied by the compositor, and more properties will be applied internally
    // to the chunks as Skia commands.
    void upcast(const PropertyTreeState&, GeometryMapper&);
    FloatRect bounds;
    Vector<const PaintChunk*> paintChunks;
    bool knownToBeOpaque;
    bool backfaceHidden;
    PropertyTreeState propertyTreeState;
    bool isForeign;
  };

  PaintArtifactCompositor();

  class ContentLayerClientImpl;

  // Collects the PaintChunks into groups which will end up in the same
  // cc layer. This is the entry point of the layerization algorithm.
  void collectPendingLayers(const PaintArtifact&,
                            Vector<PendingLayer>& pendingLayers,
                            GeometryMapper&);
  // This is the internal recursion of collectPendingLayers. This function
  // loops over the list of paint chunks, scoped by an isolated group
  // (i.e. effect node). Inside of the loop, chunks are tested for overlap
  // and merge compatibility. Subgroups are handled by recursion, and will
  // be tested for "decompositing" upon return.
  // Merge compatibility means consecutive chunks may be layerized into the
  // same backing (i.e. merged) if their property states don't cross
  // direct-compositing boundary.
  // Non-consecutive chunks that are nevertheless compatible may still be
  // merged, if reordering of the chunks won't affect the ultimate result.
  // This is determined by overlap testing such that chunks can be safely
  // reordered if their effective bounds in screen space can't overlap.
  // The recursion only tests merge & overlap for chunks scoped by the same
  // group. This is where "decompositing" came in. Upon returning from a
  // recursion, the layerization of the subgroup may be tested for merge &
  // overlap with other chunks in the parent group, if grouping requirement
  // can be satisfied (and the effect node has no direct reason).
  static void layerizeGroup(const PaintArtifact&,
                            Vector<PendingLayer>& pendingLayers,
                            GeometryMapper&,
                            const EffectPaintPropertyNode&,
                            Vector<PaintChunk>::const_iterator& chunkCursor);
  static bool mightOverlap(const PendingLayer&,
                           const PendingLayer&,
                           GeometryMapper&);
  static bool canDecompositeEffect(const EffectPaintPropertyNode*,
                                   const PendingLayer&);

  // Builds a leaf layer that represents a single paint chunk.
  // Note: cc::Layer API assumes the layer bounds start at (0, 0), but the
  // bounding box of a paint chunk does not necessarily start at (0, 0) (and
  // could even be negative). Internally the generated layer translates the
  // paint chunk to align the bounding box to (0, 0) and return the actual
  // origin of the paint chunk in the |layerOffset| outparam.
  scoped_refptr<cc::Layer> compositedLayerForPendingLayer(
      const PaintArtifact&,
      const PendingLayer&,
      gfx::Vector2dF& layerOffset,
      Vector<std::unique_ptr<ContentLayerClientImpl>>& newContentLayerClients,
      RasterInvalidationTrackingMap<const PaintChunk>*,
      bool storeDebugInfo,
      GeometryMapper&);

  // Finds a client among the current vector of clients that matches the paint
  // chunk's id, or otherwise allocates a new one.
  std::unique_ptr<ContentLayerClientImpl> clientForPaintChunk(
      const PaintChunk&,
      const PaintArtifact&);

  scoped_refptr<cc::Layer> m_rootLayer;
  std::unique_ptr<WebLayer> m_webLayer;
  Vector<std::unique_ptr<ContentLayerClientImpl>> m_contentLayerClients;

  bool m_extraDataForTestingEnabled = false;
  std::unique_ptr<ExtraDataForTesting> m_extraDataForTesting;
  friend class StubChromeClientForSPv2;

  bool m_isTrackingRasterInvalidations;
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           ForeignLayerPassesThrough);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MergeSimpleChunks);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           Merge2DTransform);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           Merge2DTransformDirectAncestor);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MergeTransformOrigin);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MergeClip);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MergeOpacity);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MergeNested);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           ClipPushedUp);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           EffectPushedUp);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           EffectAndClipPushedUp);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           ClipAndEffectNoTransform);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           TwoClips);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           TwoEffects);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           TwoTransformsClipBetween);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           OverlapTransform);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           MightOverlap);

  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           PendingLayer);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           PendingLayerWithGeometry);
  FRIEND_TEST_ALL_PREFIXES(PaintArtifactCompositorTestWithPropertyTrees,
                           PendingLayerKnownOpaque_DISABLED);
};

}  // namespace blink

#endif  // PaintArtifactCompositor_h
