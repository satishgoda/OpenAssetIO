// SPDX-License-Identifier: Apache-2.0
// Copyright 2013-2025 The Foundry Visionmongers Ltd
#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <openassetio/Context.hpp>
#include <openassetio/EntityReference.hpp>
#include <openassetio/access.hpp>
#include <openassetio/errors/BatchElementError.hpp>
#include <openassetio/errors/exceptions.hpp>
#include <openassetio/hostApi/Manager.hpp>
#include <openassetio/managerApi/HostSession.hpp>
#include <openassetio/managerApi/ManagerInterface.hpp>
#include <openassetio/trait/TraitsData.hpp>
#include <openassetio/trait/collection.hpp>

#include "../_openassetio.hpp"

namespace {
using openassetio::EntityReferences;
using openassetio::hostApi::Manager;
namespace trait = openassetio::trait;

void validateTraitsDatas(const trait::TraitsDatas& traitsDatas) {
  // Pybind has no built-in way to assert that a collection
  // does not contain any `None` elements, so we must add our
  // own check here.
  if (std::any_of(traitsDatas.begin(), traitsDatas.end(),
                  std::logical_not<trait::TraitsDataPtr>{})) {
    throw openassetio::errors::InputValidationException{"Traits data cannot be None"};
  }
}

py::list pyBoolListFromUintVector(const std::vector<Manager::BoolAsUint>& boolAsUints) {
  const py::gil_scoped_acquire gil{};
  py::list pyResult;
  for (const Manager::BoolAsUint boolAsUint : boolAsUints) {
    pyResult.append(static_cast<bool>(boolAsUint));
  }
  return pyResult;
}
}  // namespace

void registerManager(const py::module& mod) {
  namespace access = openassetio::access;
  namespace trait = openassetio::trait;
  using openassetio::ContextConstPtr;
  using openassetio::EntityReference;
  using openassetio::EntityReferences;
  using openassetio::errors::BatchElementError;
  using openassetio::hostApi::Manager;
  using openassetio::hostApi::ManagerPtr;
  using openassetio::managerApi::HostSessionPtr;
  using openassetio::managerApi::ManagerInterfacePtr;

  py::class_<Manager, ManagerPtr> pyManager{mod, "Manager", py::is_final()};

  // BatchElementErrorPolicy tags for tag dispatch overload resolution
  // idiom.
  py::class_<Manager::BatchElementErrorPolicyTag> pyBatchElementErrorPolicyTag{
      pyManager, "BatchElementErrorPolicyTag"};
  // NOLINTNEXTLINE(bugprone-unused-raii)
  py::class_<Manager::BatchElementErrorPolicyTag::Exception>{pyBatchElementErrorPolicyTag,
                                                             "Exception"};
  // NOLINTNEXTLINE(bugprone-unused-raii)
  py::class_<Manager::BatchElementErrorPolicyTag::Variant>{pyBatchElementErrorPolicyTag,
                                                           "Variant"};

  pyBatchElementErrorPolicyTag
      .def_readonly_static("kException", &Manager::BatchElementErrorPolicyTag::kException)
      .def_readonly_static("kVariant", &Manager::BatchElementErrorPolicyTag::kVariant);

  py::enum_<Manager::Capability>{pyManager, "Capability"}
      .value("kStatefulContexts", Manager::Capability::kStatefulContexts)
      .value("kCustomTerminology", Manager::Capability::kCustomTerminology)
      .value("kResolution", Manager::Capability::kResolution)
      .value("kPublishing", Manager::Capability::kPublishing)
      .value("kRelationshipQueries", Manager::Capability::kRelationshipQueries)
      .value("kExistenceQueries", Manager::Capability::kExistenceQueries)
      .value("kDefaultEntityReferences", Manager::Capability::kDefaultEntityReferences);

  pyManager
      .def(py::init(RetainCommonPyArgs::forFn<&Manager::make>()),
           py::arg("managerInterface").none(false), py::arg("hostSession").none(false))
      .def("identifier", &Manager::identifier, py::call_guard<py::gil_scoped_release>{})
      .def("displayName", &Manager::displayName, py::call_guard<py::gil_scoped_release>{})
      .def("info", &Manager::info, py::call_guard<py::gil_scoped_release>{})
      .def("settings", &Manager::settings, py::call_guard<py::gil_scoped_release>{})
      .def("initialize", &Manager::initialize, py::arg("managerSettings"),
           py::call_guard<py::gil_scoped_release>{})
      .def("flushCaches", &Manager::flushCaches, py::call_guard<py::gil_scoped_release>{})
      .def("managementPolicy",
           py::overload_cast<const trait::TraitSet&, access::PolicyAccess, const ContextConstPtr&>(
               &Manager::managementPolicy),
           py::arg("traitSet"), py::arg("policyAccess"), py::arg("context").none(false),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "managementPolicy",
          py::overload_cast<const trait::TraitSets&, access::PolicyAccess, const ContextConstPtr&>(
              &Manager::managementPolicy),
          py::arg("traitSets"), py::arg("policyAccess"), py::arg("context").none(false),
          py::call_guard<py::gil_scoped_release>{})
      .def("createContext", &Manager::createContext, py::call_guard<py::gil_scoped_release>{})
      .def("createChildContext", &Manager::createChildContext,
           py::arg("parentContext").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("persistenceTokenForContext", &Manager::persistenceTokenForContext,
           py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("contextFromPersistenceToken", &Manager::contextFromPersistenceToken, py::arg("token"),
           py::call_guard<py::gil_scoped_release>{})
      .def("isEntityReferenceString", &Manager::isEntityReferenceString, py::arg("someString"),
           py::call_guard<py::gil_scoped_release>{})
      .def("createEntityReference", &Manager::createEntityReference,
           py::arg("entityReferenceString"), py::call_guard<py::gil_scoped_release>{})
      .def("createEntityReferenceIfValid", &Manager::createEntityReferenceIfValid,
           py::arg("entityReferenceString"), py::call_guard<py::gil_scoped_release>{})
      .def(
          "defaultEntityReference",
          [](Manager& self, const trait::TraitSet& traitSet,
             const access::DefaultEntityAccess defaultEntityReferenceAccess,
             const ContextConstPtr& context) {
            return self.defaultEntityReference(traitSet, defaultEntityReferenceAccess, context);
          },
          py::arg("traitSet"), py::arg("defaultEntityReferenceAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def(
          "defaultEntityReference",
          [](Manager& self, const trait::TraitSets& traitSets,
             const access::DefaultEntityAccess defaultEntityReferenceAccess,
             const ContextConstPtr& context) {
            return self.defaultEntityReference(traitSets, defaultEntityReferenceAccess, context);
          },
          py::arg("traitSets"), py::arg("defaultEntityReferenceAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("defaultEntityReference",
           py::overload_cast<const trait::TraitSet&, access::DefaultEntityAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::defaultEntityReference),
           py::arg("traitSet"), py::arg("defaultEntityReferenceAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("defaultEntityReference",
           py::overload_cast<const trait::TraitSets&, access::DefaultEntityAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::defaultEntityReference),
           py::arg("traitSets"), py::arg("defaultEntityReferenceAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("defaultEntityReference",
           py::overload_cast<const trait::TraitSets&, access::DefaultEntityAccess,
                             const ContextConstPtr&,
                             const Manager::DefaultEntityReferenceSuccessCallback&,
                             const Manager::BatchElementErrorCallback&>(
               &Manager::defaultEntityReference),
           py::arg("traitSets"), py::arg("defaultEntityReferenceAccess"),
           py::arg("context").none(false), py::arg("successCallback"), py::arg("errorCallback"),
           py::call_guard<py::gil_scoped_release>{})
      .def("defaultEntityReference",
           py::overload_cast<const trait::TraitSet&, access::DefaultEntityAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::defaultEntityReference),
           py::arg("traitSet"), py::arg("defaultEntityReferenceAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("defaultEntityReference",
           py::overload_cast<const trait::TraitSets&, access::DefaultEntityAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::defaultEntityReference),
           py::arg("traitSets"), py::arg("defaultEntityReferenceAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "entityExists",
          [](Manager& self, const EntityReference& entityReference,
             const ContextConstPtr& context) {
            return self.entityExists(entityReference, context);
          },
          py::arg("entityReference"), py::arg("context").none(false),
          py::call_guard<py::gil_scoped_release>{})
      .def("entityExists",
           py::overload_cast<const EntityReference&, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::entityExists),
           py::arg("entityReference"), py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("entityExists",
           py::overload_cast<const EntityReference&, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::entityExists),
           py::arg("entityReference"), py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "entityExists",
          [](Manager& self, const EntityReferences& entityReferences,
             const ContextConstPtr& context) {
            return pyBoolListFromUintVector(self.entityExists(entityReferences, context));
          },
          py::arg("entityReferences"), py::arg("context").none(false),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "entityExists",
          [](Manager& self, const EntityReferences& entityReferences,
             const ContextConstPtr& context,
             const Manager::BatchElementErrorPolicyTag::Exception& errorPolicyTag) {
            return pyBoolListFromUintVector(
                self.entityExists(entityReferences, context, errorPolicyTag));
          },
          py::arg("entityReferences"), py::arg("context").none(false), py::arg("errorPolicyTag"),
          py::call_guard<py::gil_scoped_release>{})
      .def("entityExists",
           py::overload_cast<const EntityReferences&, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::entityExists),
           py::arg("entityReferences"), py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("entityExists",
           py::overload_cast<const EntityReferences&, const ContextConstPtr&,
                             const Manager::ExistsSuccessCallback&,
                             const Manager::BatchElementErrorCallback&>(&Manager::entityExists),
           py::arg("entityReferences"), py::arg("context").none(false), py::arg("successCallback"),
           py::arg("errorCallback"), py::call_guard<py::gil_scoped_release>{})
      .def("entityTraits",
           py::overload_cast<const EntityReferences&, access::EntityTraitsAccess,
                             const ContextConstPtr&, const Manager::EntityTraitsSuccessCallback&,
                             const Manager::BatchElementErrorCallback&>(&Manager::entityTraits),
           py::arg("entityReferences"), py::arg("entityTraitsAccess"),
           py::arg("context").none(false), py::arg("successCallback"), py::arg("errorCallback"),
           py::call_guard<py::gil_scoped_release>{})
      .def("entityTraits",
           py::overload_cast<const EntityReference&, access::EntityTraitsAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::entityTraits),
           py::arg("entityReference"), py::arg("entityTraitsAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("entityTraits",
           py::overload_cast<const EntityReference&, access::EntityTraitsAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::entityTraits),
           py::arg("entityReference"), py::arg("entityTraitsAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "entityTraits",
          // Note: Technically we shouldn't need this overload, see
          // similar comment for `resolve`.
          [](Manager& self, const EntityReference& entityReference,
             const access::EntityTraitsAccess entityTraitsAccess, const ContextConstPtr& context) {
            return self.entityTraits(entityReference, entityTraitsAccess, context);
          },
          py::arg("entityReference"), py::arg("entityTraitsAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("entityTraits",
           py::overload_cast<const EntityReferences&, access::EntityTraitsAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::entityTraits),
           py::arg("entityReferences"), py::arg("entityTraitsAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("entityTraits",
           py::overload_cast<const EntityReferences&, access::EntityTraitsAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::entityTraits),
           py::arg("entityReferences"), py::arg("entityTraitsAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "entityTraits",
          // Note: Technically we shouldn't need this overload, see
          // similar comment for `resolve`.
          [](Manager& self, const EntityReferences& entityReferences,
             const access::EntityTraitsAccess entityTraitsAccess, const ContextConstPtr& context) {
            return self.entityTraits(entityReferences, entityTraitsAccess, context);
          },
          py::arg("entityReferences"), py::arg("entityTraitsAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("hasCapability", &Manager::hasCapability, py::arg("capability"),
           py::call_guard<py::gil_scoped_release>{})
      .def("updateTerminology", &Manager::updateTerminology, py::arg("terms"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "resolve",
          py::overload_cast<const EntityReferences&, const trait::TraitSet&, access::ResolveAccess,
                            const ContextConstPtr&, const Manager::ResolveSuccessCallback&,
                            const Manager::BatchElementErrorCallback&>(&Manager::resolve),
          py::arg("entityReferences"), py::arg("traitSet"), py::arg("resolveAccess"),
          py::arg("context").none(false), py::arg("successCallback"), py::arg("errorCallback"),
          py::call_guard<py::gil_scoped_release>{})
      .def("resolve",
           py::overload_cast<const EntityReference&, const trait::TraitSet&, access::ResolveAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::resolve),
           py::arg("entityReference"), py::arg("traitSet"), py::arg("resolveAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("resolve",
           py::overload_cast<const EntityReference&, const trait::TraitSet&, access::ResolveAccess,
                             const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::resolve),
           py::arg("entityReference"), py::arg("traitSet"), py::arg("resolveAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "resolve",
          // Note: Technically we shouldn't need this overload, since we
          // can use a similar trick to C++ to default the appropriate
          // overload's tag parameter, e.g.
          // `py::arg("errorPolicyTag") = {}`. However, this causes a
          // memory leak in pybind11 v2.9.2.
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitSet& traitSet, const access::ResolveAccess resolveAccess,
             const ContextConstPtr& context) {
            return self.resolve(entityReference, traitSet, resolveAccess, context);
          },
          py::arg("entityReference"), py::arg("traitSet"), py::arg("resolveAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("resolve",
           py::overload_cast<const EntityReferences&, const trait::TraitSet&,
                             access::ResolveAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::resolve),
           py::arg("entityReferences"), py::arg("traitSet"), py::arg("resolveAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("resolve",
           py::overload_cast<const EntityReferences&, const trait::TraitSet&,
                             access::ResolveAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::resolve),
           py::arg("entityReferences"), py::arg("traitSet"), py::arg("resolveAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "resolve",
          // Note: Technically we shouldn't need this overload, see
          // similar comment for other `resolve` overload.
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitSet& traitSet, const access::ResolveAccess resolveAccess,
             const ContextConstPtr& context) {
            return self.resolve(entityReferences, traitSet, resolveAccess, context);
          },
          py::arg("entityReferences"), py::arg("traitSet"), py::arg("resolveAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def("getWithRelationship",
           py::overload_cast<const EntityReferences&, const trait::TraitsDataPtr&, std::size_t,
                             access::RelationsAccess, const ContextConstPtr&,
                             const Manager::RelationshipQuerySuccessCallback&,
                             const Manager::BatchElementErrorCallback&, const trait::TraitSet&>(
               &Manager::getWithRelationship),
           py::arg("entityReferences"), py::arg("relationshipTraitsData").none(false),
           py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
           py::arg("successCallback"), py::arg("errorCallback"),
           py::arg("resultTraitSet") = trait::TraitSet{}, py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationship",
          // Note: Technically we shouldn't need this overload, see
          // similar comment for `resolve`.
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDataPtr& relationshipTraitsData, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const trait::TraitSet& resultTraitSet) {
            return self.getWithRelationship(entityReference, relationshipTraitsData, pageSize,
                                            relationsAccess, context, resultTraitSet);
          },
          py::arg("entityReference"), py::arg("relationshipTraitsData").none(false),
          py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
          py::arg("resultTraitSet"), py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationship",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDataPtr& relationshipTraitsData, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const trait::TraitSet& resultTraitSet) {
            return self.getWithRelationship(entityReferences, relationshipTraitsData, pageSize,
                                            relationsAccess, context, resultTraitSet);
          },
          py::arg("entityReferences"), py::arg("relationshipTraitsData").none(false),
          py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
          py::arg("resultTraitSet"), py::call_guard<py::gil_scoped_release>{})
      .def("getWithRelationship",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&, std::size_t,
                             access::RelationsAccess, const ContextConstPtr&,
                             const trait::TraitSet&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::getWithRelationship),
           py::arg("entityReference"), py::arg("relationshipTraitsData").none(false),
           py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
           py::arg("resultTraitSet"), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("getWithRelationship",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&, std::size_t,
                             access::RelationsAccess, const ContextConstPtr&,
                             const trait::TraitSet&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::getWithRelationship),
           py::arg("entityReference"), py::arg("relationshipTraitsData").none(false),
           py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
           py::arg("resultTraitSet"), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("getWithRelationship",
           py::overload_cast<const EntityReferences&, const trait::TraitsDataPtr&, std::size_t,
                             access::RelationsAccess, const ContextConstPtr&,
                             const trait::TraitSet&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::getWithRelationship),
           py::arg("entityReferences"), py::arg("relationshipTraitsData").none(false),
           py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
           py::arg("resultTraitSet"), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("getWithRelationship",
           py::overload_cast<const EntityReferences&, const trait::TraitsDataPtr&, std::size_t,
                             access::RelationsAccess, const ContextConstPtr&,
                             const trait::TraitSet&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::getWithRelationship),
           py::arg("entityReferences"), py::arg("relationshipTraitsData").none(false),
           py::arg("pageSize"), py::arg("relationsAccess"), py::arg("context").none(false),
           py::arg("resultTraitSet"), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationships",
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDatas& relationshipTraitsDatas, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const Manager::RelationshipQuerySuccessCallback& successCallback,
             const Manager::BatchElementErrorCallback& errorCallback,
             const trait::TraitSet& resultTraitSet) {
            validateTraitsDatas(relationshipTraitsDatas);
            self.getWithRelationships(entityReference, relationshipTraitsDatas, pageSize,
                                      relationsAccess, context, successCallback, errorCallback,
                                      resultTraitSet);
          },
          py::arg("entityReference"), py::arg("relationshipTraitsDatas"), py::arg("pageSize"),
          py::arg("relationsAccess"), py::arg("context").none(false), py::arg("successCallback"),
          py::arg("errorCallback"), py::arg("resultTraitSet") = trait::TraitSet{},
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationships",
          // Note: Technically we shouldn't need this overload, see
          // similar comment for `resolve`.
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDatas& relationshipTraitsDatas, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const trait::TraitSet& resultTraitSet) {
            validateTraitsDatas(relationshipTraitsDatas);
            return self.getWithRelationships(entityReference, relationshipTraitsDatas, pageSize,
                                             relationsAccess, context, resultTraitSet);
          },
          py::arg("entityReference"), py::arg("relationshipTraitsDatas"), py::arg("pageSize"),
          py::arg("relationsAccess"), py::arg("context").none(false), py::arg("resultTraitSet"),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationships",
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDatas& relationshipTraitsDatas, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const trait::TraitSet& resultTraitSet,
             const Manager::BatchElementErrorPolicyTag::Exception& errorPolicyTag) {
            validateTraitsDatas(relationshipTraitsDatas);
            return self.getWithRelationships(entityReference, relationshipTraitsDatas, pageSize,
                                             relationsAccess, context, resultTraitSet,
                                             errorPolicyTag);
          },
          py::arg("entityReference"), py::arg("relationshipTraitsDatas"), py::arg("pageSize"),
          py::arg("relationsAccess"), py::arg("context").none(false), py::arg("resultTraitSet"),
          py::arg("errorPolicyTag"), py::call_guard<py::gil_scoped_release>{})
      .def(
          "getWithRelationships",
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDatas& relationshipTraitsDatas, const std::size_t pageSize,
             const access::RelationsAccess relationsAccess, const ContextConstPtr& context,
             const trait::TraitSet& resultTraitSet,
             const Manager::BatchElementErrorPolicyTag::Variant& errorPolicyTag) {
            validateTraitsDatas(relationshipTraitsDatas);
            return self.getWithRelationships(entityReference, relationshipTraitsDatas, pageSize,
                                             relationsAccess, context, resultTraitSet,
                                             errorPolicyTag);
          },
          py::arg("entityReference"), py::arg("relationshipTraitsDatas"), py::arg("pageSize"),
          py::arg("relationsAccess"), py::arg("context").none(false), py::arg("resultTraitSet"),
          py::arg("errorPolicyTag"), py::call_guard<py::gil_scoped_release>{})
      .def(
          "preflight",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& traitsHints,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::PreflightSuccessCallback& successCallback,
             const Manager::BatchElementErrorCallback& errorCallback) {
            validateTraitsDatas(traitsHints);
            self.preflight(entityReferences, traitsHints, publishingAccess, context,
                           successCallback, errorCallback);
          },
          py::arg("entityReferences"), py::arg("traitsHints"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("successCallback"), py::arg("errorCallback"),
          py::call_guard<py::gil_scoped_release>{})
      .def("preflight",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&,
                             access::PublishingAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::preflight),
           py::arg("entityReference"), py::arg("traitsHint").none(false), py::arg("publishAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("preflight",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&,
                             access::PublishingAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::preflight),
           py::arg("entityReference"), py::arg("traitsHint").none(false), py::arg("publishAccess"),
           py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "preflight",
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDataPtr& traitsHint,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context) {
            return self.preflight(entityReference, traitsHint, publishingAccess, context);
          },
          py::arg("entityReference"), py::arg("traitsHint").none(false), py::arg("publishAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def(
          "preflight",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& traitsHints,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::BatchElementErrorPolicyTag::Exception& tag) {
            validateTraitsDatas(traitsHints);
            return self.preflight(entityReferences, traitsHints, publishingAccess, context, tag);
          },
          py::arg("entityReferences"), py::arg("traitsHints"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("errorPolicyTag"),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "preflight",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& traitsHints,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::BatchElementErrorPolicyTag::Variant& tag) {
            validateTraitsDatas(traitsHints);
            return self.preflight(entityReferences, traitsHints, publishingAccess, context, tag);
          },
          py::arg("entityReferences"), py::arg("traitsHints"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("errorPolicyTag"),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "preflight",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& traitsHints,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context) {
            validateTraitsDatas(traitsHints);
            return self.preflight(entityReferences, traitsHints, publishingAccess, context);
          },
          py::arg("entityReferences"), py::arg("traitsHints"), py::arg("publishAccess"),
          py::arg("context").none(false), py::call_guard<py::gil_scoped_release>{})
      .def(
          "register",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& entityTraitsDatas,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::RegisterSuccessCallback& successCallback,
             const Manager::BatchElementErrorCallback& errorCallback) {
            validateTraitsDatas(entityTraitsDatas);
            self.register_(entityReferences, entityTraitsDatas, publishingAccess, context,
                           successCallback, errorCallback);
          },
          py::arg("entityReferences"), py::arg("entityTraitsDatas"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("successCallback"), py::arg("errorCallback"),
          py::call_guard<py::gil_scoped_release>{})

      .def("register",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&,
                             access::PublishingAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Exception&>(
               &Manager::register_),
           py::arg("entityReference"), py::arg("entityTraitsData").none(false),
           py::arg("publishAccess"), py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def("register",
           py::overload_cast<const EntityReference&, const trait::TraitsDataPtr&,
                             access::PublishingAccess, const ContextConstPtr&,
                             const Manager::BatchElementErrorPolicyTag::Variant&>(
               &Manager::register_),
           py::arg("entityReference"), py::arg("entityTraitsData").none(false),
           py::arg("publishAccess"), py::arg("context").none(false), py::arg("errorPolicyTag"),
           py::call_guard<py::gil_scoped_release>{})
      .def(
          "register",
          [](Manager& self, const EntityReference& entityReference,
             const trait::TraitsDataPtr& entityTraitsData,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context) {
            return self.register_(entityReference, entityTraitsData, publishingAccess, context);
          },
          py::arg("entityReference"), py::arg("entityTraitsData").none(false),
          py::arg("publishAccess"), py::arg("context").none(false),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "register",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& entityTraitsDatas,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::BatchElementErrorPolicyTag::Exception& errorPolicyTag) {
            validateTraitsDatas(entityTraitsDatas);
            return self.register_(entityReferences, entityTraitsDatas, publishingAccess, context,
                                  errorPolicyTag);
          },
          py::arg("entityReferences"), py::arg("entityTraitsDatas"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("errorPolicyTag"),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "register",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& entityTraitsDatas,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context,
             const Manager::BatchElementErrorPolicyTag::Variant& errorPolicyTag) {
            validateTraitsDatas(entityTraitsDatas);
            return self.register_(entityReferences, entityTraitsDatas, publishingAccess, context,
                                  errorPolicyTag);
          },
          py::arg("entityReferences"), py::arg("entityTraitsDatas"), py::arg("publishAccess"),
          py::arg("context").none(false), py::arg("errorPolicyTag"),
          py::call_guard<py::gil_scoped_release>{})
      .def(
          "register",
          [](Manager& self, const EntityReferences& entityReferences,
             const trait::TraitsDatas& entityTraitsDatas,
             const access::PublishingAccess publishingAccess, const ContextConstPtr& context) {
            validateTraitsDatas(entityTraitsDatas);
            return self.register_(entityReferences, entityTraitsDatas, publishingAccess, context);
          },
          py::arg("entityReference"), py::arg("entityTraitsData").none(false),
          py::arg("publishAccess"), py::arg("context").none(false),
          py::call_guard<py::gil_scoped_release>{});
}  // NOLINT(readability/fn_size)
