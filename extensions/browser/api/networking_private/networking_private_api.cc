// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_private/networking_private_api.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "components/onc/onc_constants.h"
#include "extensions/browser/api/networking_private/networking_private_delegate.h"
#include "extensions/browser/api/networking_private/networking_private_delegate_factory.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/common/api/networking_private.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/features/feature_provider.h"

namespace extensions {

namespace {

const int kDefaultNetworkListLimit = 1000;

const char kPrivateOnlyError[] = "Requires networkingPrivate API access.";

NetworkingPrivateDelegate* GetDelegate(
    content::BrowserContext* browser_context) {
  return NetworkingPrivateDelegateFactory::GetForBrowserContext(
      browser_context);
}

bool HasPrivateNetworkingAccess(const Extension* extension,
                                Feature::Context context,
                                const GURL& source_url) {
  return ExtensionAPI::GetSharedInstance()
      ->IsAvailable("networkingPrivate", extension, context, source_url,
                    CheckAliasStatus::NOT_ALLOWED)
      .is_available();
}

bool CanChangeSharedConfig(const Extension* extension,
                           Feature::Context context) {
#if defined(OS_CHROMEOS)
  return context == Feature::WEBUI_CONTEXT;
#else
  return true;
#endif
}

}  // namespace

namespace private_api = api::networking_private;

namespace networking_private {

// static
const char kErrorAccessToSharedConfig[] = "Error.CannotChangeSharedConfig";
const char kErrorInvalidNetworkGuid[] = "Error.InvalidNetworkGuid";
const char kErrorInvalidNetworkOperation[] = "Error.InvalidNetworkOperation";
const char kErrorNetworkUnavailable[] = "Error.NetworkUnavailable";
const char kErrorEncryptionError[] = "Error.EncryptionError";
const char kErrorNotReady[] = "Error.NotReady";
const char kErrorNotSupported[] = "Error.NotSupported";
const char kErrorPolicyControlled[] = "Error.PolicyControlled";
const char kErrorSimLocked[] = "Error.SimLocked";

}  // namespace networking_private

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetPropertiesFunction

NetworkingPrivateGetPropertiesFunction::
    ~NetworkingPrivateGetPropertiesFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetPropertiesFunction::Run() {
  std::unique_ptr<private_api::GetProperties::Params> params =
      private_api::GetProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetProperties(
          params->network_guid,
          base::Bind(&NetworkingPrivateGetPropertiesFunction::Success, this),
          base::Bind(&NetworkingPrivateGetPropertiesFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetPropertiesFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  Respond(OneArgument(std::move(result)));
}

void NetworkingPrivateGetPropertiesFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetManagedPropertiesFunction

NetworkingPrivateGetManagedPropertiesFunction::
    ~NetworkingPrivateGetManagedPropertiesFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetManagedPropertiesFunction::Run() {
  std::unique_ptr<private_api::GetManagedProperties::Params> params =
      private_api::GetManagedProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetManagedProperties(
          params->network_guid,
          base::Bind(&NetworkingPrivateGetManagedPropertiesFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateGetManagedPropertiesFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetManagedPropertiesFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  Respond(OneArgument(std::move(result)));
}

void NetworkingPrivateGetManagedPropertiesFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetStateFunction

NetworkingPrivateGetStateFunction::~NetworkingPrivateGetStateFunction() {
}

ExtensionFunction::ResponseAction NetworkingPrivateGetStateFunction::Run() {
  std::unique_ptr<private_api::GetState::Params> params =
      private_api::GetState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetState(params->network_guid,
                 base::Bind(&NetworkingPrivateGetStateFunction::Success, this),
                 base::Bind(&NetworkingPrivateGetStateFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetStateFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  Respond(OneArgument(std::move(result)));
}

void NetworkingPrivateGetStateFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateSetPropertiesFunction

NetworkingPrivateSetPropertiesFunction::
    ~NetworkingPrivateSetPropertiesFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateSetPropertiesFunction::Run() {
  std::unique_ptr<private_api::SetProperties::Params> params =
      private_api::SetProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  std::unique_ptr<base::DictionaryValue> properties_dict(
      params->properties.ToValue());

  GetDelegate(browser_context())
      ->SetProperties(
          params->network_guid, std::move(properties_dict),
          CanChangeSharedConfig(extension(), source_context_type()),
          base::Bind(&NetworkingPrivateSetPropertiesFunction::Success, this),
          base::Bind(&NetworkingPrivateSetPropertiesFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateSetPropertiesFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateSetPropertiesFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateCreateNetworkFunction

NetworkingPrivateCreateNetworkFunction::
    ~NetworkingPrivateCreateNetworkFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateCreateNetworkFunction::Run() {
  std::unique_ptr<private_api::CreateNetwork::Params> params =
      private_api::CreateNetwork::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  if (params->shared &&
      !CanChangeSharedConfig(extension(), source_context_type())) {
    return RespondNow(Error(networking_private::kErrorAccessToSharedConfig));
  }

  std::unique_ptr<base::DictionaryValue> properties_dict(
      params->properties.ToValue());

  GetDelegate(browser_context())
      ->CreateNetwork(
          params->shared, std::move(properties_dict),
          base::Bind(&NetworkingPrivateCreateNetworkFunction::Success, this),
          base::Bind(&NetworkingPrivateCreateNetworkFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateCreateNetworkFunction::Success(const std::string& guid) {
  Respond(ArgumentList(private_api::CreateNetwork::Results::Create(guid)));
}

void NetworkingPrivateCreateNetworkFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateForgetNetworkFunction

NetworkingPrivateForgetNetworkFunction::
    ~NetworkingPrivateForgetNetworkFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateForgetNetworkFunction::Run() {
  std::unique_ptr<private_api::ForgetNetwork::Params> params =
      private_api::ForgetNetwork::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->ForgetNetwork(
          params->network_guid,
          base::Bind(&NetworkingPrivateForgetNetworkFunction::Success, this),
          base::Bind(&NetworkingPrivateForgetNetworkFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateForgetNetworkFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateForgetNetworkFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetNetworksFunction

NetworkingPrivateGetNetworksFunction::~NetworkingPrivateGetNetworksFunction() {
}

ExtensionFunction::ResponseAction NetworkingPrivateGetNetworksFunction::Run() {
  std::unique_ptr<private_api::GetNetworks::Params> params =
      private_api::GetNetworks::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string network_type = private_api::ToString(params->filter.network_type);
  const bool configured_only =
      params->filter.configured ? *params->filter.configured : false;
  const bool visible_only =
      params->filter.visible ? *params->filter.visible : false;
  const int limit =
      params->filter.limit ? *params->filter.limit : kDefaultNetworkListLimit;

  GetDelegate(browser_context())
      ->GetNetworks(
          network_type, configured_only, visible_only, limit,
          base::Bind(&NetworkingPrivateGetNetworksFunction::Success, this),
          base::Bind(&NetworkingPrivateGetNetworksFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetNetworksFunction::Success(
    std::unique_ptr<base::ListValue> network_list) {
  return Respond(OneArgument(std::move(network_list)));
}

void NetworkingPrivateGetNetworksFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetVisibleNetworksFunction

NetworkingPrivateGetVisibleNetworksFunction::
    ~NetworkingPrivateGetVisibleNetworksFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetVisibleNetworksFunction::Run() {
  std::unique_ptr<private_api::GetVisibleNetworks::Params> params =
      private_api::GetVisibleNetworks::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  // getVisibleNetworks is deprecated - allow it only for apps with
  // networkingPrivate permissions, i.e. apps that might have started using it
  // before its deprecation.
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::string network_type = private_api::ToString(params->network_type);
  const bool configured_only = false;
  const bool visible_only = true;

  GetDelegate(browser_context())
      ->GetNetworks(
          network_type, configured_only, visible_only, kDefaultNetworkListLimit,
          base::Bind(&NetworkingPrivateGetVisibleNetworksFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateGetVisibleNetworksFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetVisibleNetworksFunction::Success(
    std::unique_ptr<base::ListValue> network_properties_list) {
  Respond(OneArgument(std::move(network_properties_list)));
}

void NetworkingPrivateGetVisibleNetworksFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetEnabledNetworkTypesFunction

NetworkingPrivateGetEnabledNetworkTypesFunction::
    ~NetworkingPrivateGetEnabledNetworkTypesFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetEnabledNetworkTypesFunction::Run() {
  // getEnabledNetworkTypes is deprecated - allow it only for apps with
  // networkingPrivate permissions, i.e. apps that might have started using it
  // before its deprecation.
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<base::ListValue> enabled_networks_onc_types(
      GetDelegate(browser_context())->GetEnabledNetworkTypes());
  if (!enabled_networks_onc_types)
    return RespondNow(Error(networking_private::kErrorNotSupported));
  std::unique_ptr<base::ListValue> enabled_networks_list(new base::ListValue);
  for (base::ListValue::iterator iter = enabled_networks_onc_types->begin();
       iter != enabled_networks_onc_types->end(); ++iter) {
    std::string type;
    if (!(*iter)->GetAsString(&type))
      NOTREACHED();
    if (type == ::onc::network_type::kEthernet) {
      enabled_networks_list->AppendString(
          private_api::ToString(private_api::NETWORK_TYPE_ETHERNET));
    } else if (type == ::onc::network_type::kWiFi) {
      enabled_networks_list->AppendString(
          private_api::ToString(private_api::NETWORK_TYPE_WIFI));
    } else if (type == ::onc::network_type::kWimax) {
      enabled_networks_list->AppendString(
          private_api::ToString(private_api::NETWORK_TYPE_WIMAX));
    } else if (type == ::onc::network_type::kCellular) {
      enabled_networks_list->AppendString(
          private_api::ToString(private_api::NETWORK_TYPE_CELLULAR));
    } else {
      LOG(ERROR) << "networkingPrivate: Unexpected type: " << type;
    }
  }
  return RespondNow(OneArgument(std::move(enabled_networks_list)));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetDeviceStatesFunction

NetworkingPrivateGetDeviceStatesFunction::
    ~NetworkingPrivateGetDeviceStatesFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetDeviceStatesFunction::Run() {
  std::unique_ptr<NetworkingPrivateDelegate::DeviceStateList> device_states(
      GetDelegate(browser_context())->GetDeviceStateList());
  if (!device_states)
    return RespondNow(Error(networking_private::kErrorNotSupported));

  std::unique_ptr<base::ListValue> device_state_list(new base::ListValue);
  for (const auto& properties : *device_states)
    device_state_list->Append(properties->ToValue());
  return RespondNow(OneArgument(std::move(device_state_list)));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateEnableNetworkTypeFunction

NetworkingPrivateEnableNetworkTypeFunction::
    ~NetworkingPrivateEnableNetworkTypeFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateEnableNetworkTypeFunction::Run() {
  std::unique_ptr<private_api::EnableNetworkType::Params> params =
      private_api::EnableNetworkType::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  if (!GetDelegate(browser_context())
           ->EnableNetworkType(private_api::ToString(params->network_type))) {
    return RespondNow(Error(networking_private::kErrorNotSupported));
  }
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateDisableNetworkTypeFunction

NetworkingPrivateDisableNetworkTypeFunction::
    ~NetworkingPrivateDisableNetworkTypeFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateDisableNetworkTypeFunction::Run() {
  std::unique_ptr<private_api::DisableNetworkType::Params> params =
      private_api::DisableNetworkType::Params::Create(*args_);

  if (!GetDelegate(browser_context())
           ->DisableNetworkType(private_api::ToString(params->network_type))) {
    return RespondNow(Error(networking_private::kErrorNotSupported));
  }
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateRequestNetworkScanFunction

NetworkingPrivateRequestNetworkScanFunction::
    ~NetworkingPrivateRequestNetworkScanFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateRequestNetworkScanFunction::Run() {
  if (!GetDelegate(browser_context())->RequestScan())
    return RespondNow(Error(networking_private::kErrorNotSupported));
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateStartConnectFunction

NetworkingPrivateStartConnectFunction::
    ~NetworkingPrivateStartConnectFunction() {
}

ExtensionFunction::ResponseAction NetworkingPrivateStartConnectFunction::Run() {
  std::unique_ptr<private_api::StartConnect::Params> params =
      private_api::StartConnect::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartConnect(
          params->network_guid,
          base::Bind(&NetworkingPrivateStartConnectFunction::Success, this),
          base::Bind(&NetworkingPrivateStartConnectFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateStartConnectFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateStartConnectFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateStartDisconnectFunction

NetworkingPrivateStartDisconnectFunction::
    ~NetworkingPrivateStartDisconnectFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateStartDisconnectFunction::Run() {
  std::unique_ptr<private_api::StartDisconnect::Params> params =
      private_api::StartDisconnect::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartDisconnect(
          params->network_guid,
          base::Bind(&NetworkingPrivateStartDisconnectFunction::Success, this),
          base::Bind(&NetworkingPrivateStartDisconnectFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateStartDisconnectFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateStartDisconnectFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateStartActivateFunction

NetworkingPrivateStartActivateFunction::
    ~NetworkingPrivateStartActivateFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateStartActivateFunction::Run() {
  std::unique_ptr<private_api::StartActivate::Params> params =
      private_api::StartActivate::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartActivate(
          params->network_guid, params->carrier ? *params->carrier : "",
          base::Bind(&NetworkingPrivateStartActivateFunction::Success, this),
          base::Bind(&NetworkingPrivateStartActivateFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateStartActivateFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateStartActivateFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateVerifyDestinationFunction

NetworkingPrivateVerifyDestinationFunction::
    ~NetworkingPrivateVerifyDestinationFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateVerifyDestinationFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<private_api::VerifyDestination::Params> params =
      private_api::VerifyDestination::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->VerifyDestination(
          params->properties,
          base::Bind(&NetworkingPrivateVerifyDestinationFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateVerifyDestinationFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateVerifyDestinationFunction::Success(bool result) {
  Respond(
      ArgumentList(private_api::VerifyDestination::Results::Create(result)));
}

void NetworkingPrivateVerifyDestinationFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateVerifyAndEncryptCredentialsFunction

NetworkingPrivateVerifyAndEncryptCredentialsFunction::
    ~NetworkingPrivateVerifyAndEncryptCredentialsFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateVerifyAndEncryptCredentialsFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<private_api::VerifyAndEncryptCredentials::Params> params =
      private_api::VerifyAndEncryptCredentials::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->VerifyAndEncryptCredentials(
          params->network_guid, params->properties,
          base::Bind(
              &NetworkingPrivateVerifyAndEncryptCredentialsFunction::Success,
              this),
          base::Bind(
              &NetworkingPrivateVerifyAndEncryptCredentialsFunction::Failure,
              this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateVerifyAndEncryptCredentialsFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      private_api::VerifyAndEncryptCredentials::Results::Create(result)));
}

void NetworkingPrivateVerifyAndEncryptCredentialsFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateVerifyAndEncryptDataFunction

NetworkingPrivateVerifyAndEncryptDataFunction::
    ~NetworkingPrivateVerifyAndEncryptDataFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateVerifyAndEncryptDataFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<private_api::VerifyAndEncryptData::Params> params =
      private_api::VerifyAndEncryptData::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->VerifyAndEncryptData(
          params->properties, params->data,
          base::Bind(&NetworkingPrivateVerifyAndEncryptDataFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateVerifyAndEncryptDataFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateVerifyAndEncryptDataFunction::Success(
    const std::string& result) {
  Respond(
      ArgumentList(private_api::VerifyAndEncryptData::Results::Create(result)));
}

void NetworkingPrivateVerifyAndEncryptDataFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateSetWifiTDLSEnabledStateFunction

NetworkingPrivateSetWifiTDLSEnabledStateFunction::
    ~NetworkingPrivateSetWifiTDLSEnabledStateFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateSetWifiTDLSEnabledStateFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<private_api::SetWifiTDLSEnabledState::Params> params =
      private_api::SetWifiTDLSEnabledState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->SetWifiTDLSEnabledState(
          params->ip_or_mac_address, params->enabled,
          base::Bind(&NetworkingPrivateSetWifiTDLSEnabledStateFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateSetWifiTDLSEnabledStateFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateSetWifiTDLSEnabledStateFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      private_api::SetWifiTDLSEnabledState::Results::Create(result)));
}

void NetworkingPrivateSetWifiTDLSEnabledStateFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetWifiTDLSStatusFunction

NetworkingPrivateGetWifiTDLSStatusFunction::
    ~NetworkingPrivateGetWifiTDLSStatusFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetWifiTDLSStatusFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<private_api::GetWifiTDLSStatus::Params> params =
      private_api::GetWifiTDLSStatus::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetWifiTDLSStatus(
          params->ip_or_mac_address,
          base::Bind(&NetworkingPrivateGetWifiTDLSStatusFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateGetWifiTDLSStatusFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetWifiTDLSStatusFunction::Success(
    const std::string& result) {
  Respond(
      ArgumentList(private_api::GetWifiTDLSStatus::Results::Create(result)));
}

void NetworkingPrivateGetWifiTDLSStatusFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetCaptivePortalStatusFunction

NetworkingPrivateGetCaptivePortalStatusFunction::
    ~NetworkingPrivateGetCaptivePortalStatusFunction() {
}

ExtensionFunction::ResponseAction
NetworkingPrivateGetCaptivePortalStatusFunction::Run() {
  std::unique_ptr<private_api::GetCaptivePortalStatus::Params> params =
      private_api::GetCaptivePortalStatus::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetCaptivePortalStatus(
          params->network_guid,
          base::Bind(&NetworkingPrivateGetCaptivePortalStatusFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateGetCaptivePortalStatusFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateGetCaptivePortalStatusFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(private_api::GetCaptivePortalStatus::Results::Create(
      private_api::ParseCaptivePortalStatus(result))));
}

void NetworkingPrivateGetCaptivePortalStatusFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateUnlockCellularSimFunction

NetworkingPrivateUnlockCellularSimFunction::
    ~NetworkingPrivateUnlockCellularSimFunction() {}

ExtensionFunction::ResponseAction
NetworkingPrivateUnlockCellularSimFunction::Run() {
  std::unique_ptr<private_api::UnlockCellularSim::Params> params =
      private_api::UnlockCellularSim::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->UnlockCellularSim(
          params->network_guid, params->pin, params->puk ? *params->puk : "",
          base::Bind(&NetworkingPrivateUnlockCellularSimFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateUnlockCellularSimFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateUnlockCellularSimFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateUnlockCellularSimFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateSetCellularSimStateFunction

NetworkingPrivateSetCellularSimStateFunction::
    ~NetworkingPrivateSetCellularSimStateFunction() {}

ExtensionFunction::ResponseAction
NetworkingPrivateSetCellularSimStateFunction::Run() {
  std::unique_ptr<private_api::SetCellularSimState::Params> params =
      private_api::SetCellularSimState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->SetCellularSimState(
          params->network_guid, params->sim_state.require_pin,
          params->sim_state.current_pin,
          params->sim_state.new_pin ? *params->sim_state.new_pin : "",
          base::Bind(&NetworkingPrivateSetCellularSimStateFunction::Success,
                     this),
          base::Bind(&NetworkingPrivateSetCellularSimStateFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingPrivateSetCellularSimStateFunction::Success() {
  Respond(NoArguments());
}

void NetworkingPrivateSetCellularSimStateFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingPrivateGetGlobalPolicyFunction

NetworkingPrivateGetGlobalPolicyFunction::
    ~NetworkingPrivateGetGlobalPolicyFunction() {}

ExtensionFunction::ResponseAction
NetworkingPrivateGetGlobalPolicyFunction::Run() {
  std::unique_ptr<base::DictionaryValue> policy_dict(
      GetDelegate(browser_context())->GetGlobalPolicy());
  DCHECK(policy_dict);
  // private_api::GlobalPolicy is a subset of the global policy dictionary
  // (by definition), so use the api setter/getter to generate the subset.
  std::unique_ptr<private_api::GlobalPolicy> policy(
      private_api::GlobalPolicy::FromValue(*policy_dict));
  DCHECK(policy);
  return RespondNow(
      ArgumentList(private_api::GetGlobalPolicy::Results::Create(*policy)));
}

}  // namespace extensions
