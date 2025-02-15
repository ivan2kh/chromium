/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/layout/svg/SVGResourcesCache.h"

#include "core/HTMLNames.h"
#include "core/layout/svg/LayoutSVGResourceContainer.h"
#include "core/layout/svg/SVGResources.h"
#include "core/layout/svg/SVGResourcesCycleSolver.h"
#include "core/svg/SVGDocumentExtensions.h"
#include <memory>

namespace blink {

SVGResourcesCache::SVGResourcesCache() {}

SVGResourcesCache::~SVGResourcesCache() {}

void SVGResourcesCache::addResourcesFromLayoutObject(
    LayoutObject* object,
    const ComputedStyle& style) {
  ASSERT(object);
  ASSERT(!m_cache.contains(object));

  // Build a list of all resources associated with the passed LayoutObject.
  std::unique_ptr<SVGResources> newResources =
      SVGResources::buildResources(object, style);
  if (!newResources)
    return;

  // Put object in cache.
  SVGResources* resources =
      m_cache.set(object, std::move(newResources)).storedValue->value.get();

  // Run cycle-detection _afterwards_, so self-references can be caught as well.
  SVGResourcesCycleSolver solver(object, resources);
  solver.resolveCycles();

  // Walk resources and register the layout object as a client of each resource.
  HashSet<LayoutSVGResourceContainer*> resourceSet;
  resources->buildSetOfResources(resourceSet);

  for (auto* resourceContainer : resourceSet)
    resourceContainer->addClient(object);
}

void SVGResourcesCache::removeResourcesFromLayoutObject(LayoutObject* object) {
  std::unique_ptr<SVGResources> resources = m_cache.take(object);
  if (!resources)
    return;

  // Walk resources and unregister the layout object as a client of each
  // resource.
  HashSet<LayoutSVGResourceContainer*> resourceSet;
  resources->buildSetOfResources(resourceSet);

  for (auto* resourceContainer : resourceSet)
    resourceContainer->removeClient(object);
}

static inline SVGResourcesCache& resourcesCache(Document& document) {
  return document.accessSVGExtensions().resourcesCache();
}

SVGResources* SVGResourcesCache::cachedResourcesForLayoutObject(
    const LayoutObject* layoutObject) {
  ASSERT(layoutObject);
  return resourcesCache(layoutObject->document()).m_cache.at(layoutObject);
}

void SVGResourcesCache::clientLayoutChanged(LayoutObject* object) {
  SVGResources* resources = cachedResourcesForLayoutObject(object);
  if (!resources)
    return;

  // Invalidate the resources if either the LayoutObject itself changed,
  // or we have filter resources, which could depend on the layout of children.
  if (object->selfNeedsLayout() || resources->filter())
    resources->removeClientFromCache(object);
}

static inline bool layoutObjectCanHaveResources(LayoutObject* layoutObject) {
  ASSERT(layoutObject);
  return layoutObject->node() && layoutObject->node()->isSVGElement() &&
         !layoutObject->isSVGInlineText();
}

static inline bool isLayoutObjectOfResourceContainer(LayoutObject* layoutObject) {
  LayoutObject* current = layoutObject;
  while (current) {
    if (current->isSVGResourceContainer()) {
      return true;
    }
    current = current->parent();
  }
  return false;
}

void SVGResourcesCache::clientStyleChanged(LayoutObject* layoutObject,
                                           StyleDifference diff,
                                           const ComputedStyle& newStyle) {
  ASSERT(layoutObject);
  ASSERT(layoutObject->node());
  ASSERT(layoutObject->node()->isSVGElement());

  if (!diff.hasDifference() || !layoutObject->parent())
    return;

  // In this case the proper SVGFE*Element will decide whether the modified CSS
  // properties require
  // a relayout or paintInvalidation.
  if (layoutObject->isSVGResourceFilterPrimitive() && !diff.needsLayout())
    return;

  // Dynamic changes of CSS properties like 'clip-path' may require us to
  // recompute the associated resources for a LayoutObject.
  // TODO(fs): Avoid passing in a useless StyleDifference, but instead compare
  // oldStyle/newStyle to see which resources changed to be able to selectively
  // rebuild individual resources, instead of all of them.
  if (layoutObjectCanHaveResources(layoutObject)) {
    SVGResourcesCache& cache = resourcesCache(layoutObject->document());
    cache.removeResourcesFromLayoutObject(layoutObject);
    cache.addResourcesFromLayoutObject(layoutObject, newStyle);
  }

  // If this layoutObject is the child of ResourceContainer and it require
  // repainting that changes of CSS properties such as 'visibility',
  // request repainting.
  bool needsLayout = diff.needsFullPaintInvalidation() &&
                     isLayoutObjectOfResourceContainer(layoutObject);

  LayoutSVGResourceContainer::markForLayoutAndParentResourceInvalidation(
      layoutObject, needsLayout);
}

void SVGResourcesCache::clientWasAddedToTree(LayoutObject* layoutObject,
                                             const ComputedStyle& newStyle) {
  if (!layoutObject->node())
    return;
  LayoutSVGResourceContainer::markForLayoutAndParentResourceInvalidation(
      layoutObject, false);

  if (!layoutObjectCanHaveResources(layoutObject))
    return;
  SVGResourcesCache& cache = resourcesCache(layoutObject->document());
  cache.addResourcesFromLayoutObject(layoutObject, newStyle);
}

void SVGResourcesCache::clientWillBeRemovedFromTree(
    LayoutObject* layoutObject) {
  if (!layoutObject->node())
    return;
  LayoutSVGResourceContainer::markForLayoutAndParentResourceInvalidation(
      layoutObject, false);

  if (!layoutObjectCanHaveResources(layoutObject))
    return;
  SVGResourcesCache& cache = resourcesCache(layoutObject->document());
  cache.removeResourcesFromLayoutObject(layoutObject);
}

void SVGResourcesCache::clientDestroyed(LayoutObject* layoutObject) {
  ASSERT(layoutObject);

  SVGResources* resources = cachedResourcesForLayoutObject(layoutObject);
  if (resources)
    resources->removeClientFromCache(layoutObject);
  SVGResourcesCache& cache = resourcesCache(layoutObject->document());
  cache.removeResourcesFromLayoutObject(layoutObject);
}

}  // namespace blink
