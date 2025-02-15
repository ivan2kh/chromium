/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/webgl/WebGLDebugShaders.h"

#include "bindings/core/v8/ExceptionState.h"
#include "modules/webgl/GLStringQuery.h"
#include "modules/webgl/WebGLRenderingContextBase.h"
#include "modules/webgl/WebGLShader.h"

namespace blink {

WebGLDebugShaders::WebGLDebugShaders(WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->extensionsUtil()->ensureExtensionEnabled(
      "GL_ANGLE_translated_shader_source");
}

WebGLExtensionName WebGLDebugShaders::name() const {
  return WebGLDebugShadersName;
}

WebGLDebugShaders* WebGLDebugShaders::create(
    WebGLRenderingContextBase* context) {
  return new WebGLDebugShaders(context);
}

String WebGLDebugShaders::getTranslatedShaderSource(WebGLShader* shader) {
  WebGLExtensionScopedContext scoped(this);
  if (scoped.isLost())
    return String();
  if (!scoped.context()->validateWebGLObject("getTranslatedShaderSource",
                                             shader))
    return "";
  GLStringQuery query(scoped.context()->contextGL());
  return query.Run<GLStringQuery::TranslatedShaderSourceANGLE>(
      shader->object());
}

bool WebGLDebugShaders::supported(WebGLRenderingContextBase* context) {
  return context->extensionsUtil()->supportsExtension(
      "GL_ANGLE_translated_shader_source");
}

const char* WebGLDebugShaders::extensionName() {
  return "WEBGL_debug_shaders";
}

}  // namespace blink
