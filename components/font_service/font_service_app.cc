// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/font_service/font_service_app.h"

#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connection.h"
#include "services/service_manager/public/cpp/interface_registry.h"
#include "services/service_manager/public/cpp/service_context.h"

static_assert(
    static_cast<uint32_t>(SkFontStyle::kUpright_Slant) ==
        static_cast<uint32_t>(font_service::mojom::TypefaceSlant::ROMAN),
    "Skia and font service flags must match");
static_assert(
    static_cast<uint32_t>(SkFontStyle::kItalic_Slant) ==
        static_cast<uint32_t>(font_service::mojom::TypefaceSlant::ITALIC),
    "Skia and font service flags must match");
static_assert(
    static_cast<uint32_t>(SkFontStyle::kOblique_Slant) ==
        static_cast<uint32_t>(font_service::mojom::TypefaceSlant::OBLIQUE),
    "Skia and font service flags must match");

namespace {

base::File GetFileForPath(const base::FilePath& path) {
  if (path.empty())
    return base::File();

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  LOG_IF(WARNING, !file.IsValid()) << "file not valid, path=" << path.value();
  return file;
}

}  // namespace

namespace font_service {

FontServiceApp::FontServiceApp() {}

FontServiceApp::~FontServiceApp() {}

void FontServiceApp::OnStart() {
  tracing_.Initialize(context()->connector(), context()->identity().name());
}

bool FontServiceApp::OnConnect(const service_manager::ServiceInfo& remote_info,
                               service_manager::InterfaceRegistry* registry) {
  registry->AddInterface(this);
  return true;
}

void FontServiceApp::Create(
    const service_manager::Identity& remote_identity,
    mojo::InterfaceRequest<mojom::FontService> request) {
  bindings_.AddBinding(this, std::move(request));
}

void FontServiceApp::MatchFamilyName(const std::string& family_name,
                                     mojom::TypefaceStylePtr requested_style,
                                     const MatchFamilyNameCallback& callback) {
  SkFontConfigInterface::FontIdentity result_identity;
  SkString result_family;
  SkFontStyle result_style;
  SkFontConfigInterface* fc =
      SkFontConfigInterface::GetSingletonDirectInterface();
  const bool r = fc->matchFamilyName(
      family_name.data(),
      SkFontStyle(requested_style->weight,
                  requested_style->width,
                  static_cast<SkFontStyle::Slant>(requested_style->slant)),
      &result_identity, &result_family, &result_style);

  if (!r) {
    mojom::TypefaceStylePtr style(mojom::TypefaceStyle::New());
    style->weight = SkFontStyle().weight();
    style->width = SkFontStyle().width();
    style->slant = static_cast<mojom::TypefaceSlant>(SkFontStyle().slant());
    callback.Run(nullptr, "", std::move(style));
    return;
  }

  // Stash away the returned path, so we can give it an ID (index)
  // which will later be given to us in a request to open the file.
  int index = FindOrAddPath(result_identity.fString);

  mojom::FontIdentityPtr identity(mojom::FontIdentity::New());
  identity->id = static_cast<uint32_t>(index);
  identity->ttc_index = result_identity.fTTCIndex;
  identity->str_representation = result_identity.fString.c_str();

  mojom::TypefaceStylePtr style(mojom::TypefaceStyle::New());
  style->weight = result_style.weight();
  style->width = result_style.width();
  style->slant = static_cast<mojom::TypefaceSlant>(result_style.slant());

  callback.Run(std::move(identity), result_family.c_str(), std::move(style));
}

void FontServiceApp::OpenStream(uint32_t id_number,
                                const OpenStreamCallback& callback) {
  base::File file;
  if (id_number < static_cast<uint32_t>(paths_.size())) {
    file = GetFileForPath(base::FilePath(paths_[id_number].c_str()));
  }

  callback.Run(std::move(file));
}

int FontServiceApp::FindOrAddPath(const SkString& path) {
  int count = paths_.size();
  for (int i = 0; i < count; ++i) {
    if (path == paths_[i])
      return i;
  }
  paths_.emplace_back(path);
  return count;
}

}  // namespace font_service
