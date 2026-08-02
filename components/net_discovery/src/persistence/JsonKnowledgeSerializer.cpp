/**
 * @file JsonKnowledgeSerializer.cpp
 * @brief Versioned JSON implementation of IKnowledgeSerializer using cJSON (v5.0.0 Architecture Phase 7.5).
 */

#include "persistence/JsonKnowledgeSerializer.h"

#include <cstdlib>
#include <cstring>
#include "cJSON.h"

namespace NetDiscovery {
namespace Persistence {

std::string JsonKnowledgeSerializer::Serialize(const KnowledgeEntity& entity) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "schema_version", entity.schemaVersion);
    cJSON_AddStringToObject(root, "entity_id", entity.persistentId.c_str());

    // Identity Layer
    cJSON* identity = cJSON_CreateObject();
    cJSON_AddStringToObject(identity, "mac_address", entity.identity.macAddress.c_str());
    cJSON_AddStringToObject(identity, "serial_number", entity.identity.serialNumber.c_str());
    cJSON_AddStringToObject(identity, "vendor", entity.identity.vendor.c_str());
    cJSON_AddStringToObject(identity, "model", entity.identity.model.c_str());
    cJSON_AddStringToObject(identity, "primary_class", NetDiscovery::ToString(entity.primaryClass).c_str());
    
    cJSON* caps = cJSON_CreateArray();
    for (const auto& cap : entity.capabilities.GetCapabilities()) {
        cJSON* capObj = cJSON_CreateObject();
        cJSON_AddStringToObject(capObj, "id", cap.id.c_str());
        cJSON_AddStringToObject(capObj, "display_name", cap.displayName.c_str());
        cJSON_AddStringToObject(capObj, "description", cap.description.c_str());
        cJSON_AddStringToObject(capObj, "category", NetDiscovery::ToString(cap.category).c_str());
        cJSON_AddStringToObject(capObj, "version", cap.version.c_str());

        cJSON* ops = cJSON_CreateArray();
        for (const auto& [opKey, opDef] : cap.operations) {
            cJSON* opObj = cJSON_CreateObject();
            std::string opId = opDef.id.empty() ? opDef.name : opDef.id;
            cJSON_AddStringToObject(opObj, "id", opId.c_str());
            cJSON_AddStringToObject(opObj, "name", opDef.name.c_str());
            cJSON_AddStringToObject(opObj, "display_name", opDef.displayName.c_str());
            cJSON_AddStringToObject(opObj, "description", opDef.description.c_str());
            cJSON_AddStringToObject(opObj, "return_type", NetDiscovery::ToString(opDef.returnType).c_str());
            cJSON_AddBoolToObject(opObj, "read_only", opDef.readOnly);
            cJSON_AddNumberToObject(opObj, "estimated_duration_ms", opDef.estimatedDurationMs);
            cJSON_AddNumberToObject(opObj, "timeout_ms", opDef.timeoutMs);
            cJSON_AddBoolToObject(opObj, "idempotent", opDef.idempotent);
            cJSON_AddBoolToObject(opObj, "safe", opDef.safe);
            cJSON_AddBoolToObject(opObj, "requires_confirmation", opDef.requiresConfirmation);

            cJSON* params = cJSON_CreateArray();
            for (const auto& pDef : opDef.parameters) {
                cJSON* pObj = cJSON_CreateObject();
                cJSON_AddStringToObject(pObj, "name", pDef.name.c_str());
                cJSON_AddStringToObject(pObj, "display_name", pDef.displayName.c_str());
                cJSON_AddStringToObject(pObj, "description", pDef.description.c_str());
                cJSON_AddStringToObject(pObj, "type", NetDiscovery::ToString(pDef.type).c_str());
                cJSON_AddBoolToObject(pObj, "required", pDef.required);
                cJSON_AddStringToObject(pObj, "default_value", pDef.defaultValue.c_str());
                cJSON_AddStringToObject(pObj, "unit", pDef.unit.c_str());

                // ParameterConstraint Object
                cJSON* constr = cJSON_CreateObject();
                if (pDef.constraint.minimum.has_value()) {
                    cJSON_AddNumberToObject(constr, "minimum", pDef.constraint.minimum.value());
                }
                if (pDef.constraint.maximum.has_value()) {
                    cJSON_AddNumberToObject(constr, "maximum", pDef.constraint.maximum.value());
                }
                if (pDef.constraint.step.has_value()) {
                    cJSON_AddNumberToObject(constr, "step", pDef.constraint.step.value());
                }
                if (pDef.constraint.minimumLength.has_value()) {
                    cJSON_AddNumberToObject(constr, "minimum_length", (double)pDef.constraint.minimumLength.value());
                }
                if (pDef.constraint.maximumLength.has_value()) {
                    cJSON_AddNumberToObject(constr, "maximum_length", (double)pDef.constraint.maximumLength.value());
                }
                if (pDef.constraint.regex.has_value()) {
                    cJSON_AddStringToObject(constr, "regex", pDef.constraint.regex.value().c_str());
                }
                if (!pDef.constraint.unit.empty()) {
                    cJSON_AddStringToObject(constr, "unit", pDef.constraint.unit.c_str());
                }
                if (pDef.constraint.precision.has_value()) {
                    cJSON_AddNumberToObject(constr, "precision", pDef.constraint.precision.value());
                }
                if (!pDef.constraint.allowedValues.empty()) {
                    cJSON* allowed = cJSON_CreateArray();
                    for (const auto& val : pDef.constraint.allowedValues) {
                        cJSON_AddItemToArray(allowed, cJSON_CreateString(val.c_str()));
                    }
                    cJSON_AddItemToObject(constr, "allowed_values", allowed);
                }
                cJSON_AddItemToObject(pObj, "constraint", constr);

                cJSON_AddItemToArray(params, pObj);
            }
            cJSON_AddItemToObject(opObj, "parameters", params);
            cJSON_AddItemToArray(ops, opObj);
        }
        cJSON_AddItemToObject(capObj, "operations", ops);
        cJSON_AddItemToArray(caps, capObj);
    }
    cJSON_AddItemToObject(identity, "capabilities", caps);

    cJSON* ctrls = cJSON_CreateArray();
    for (const auto& ctrl : entity.compatibleControllers) {
        cJSON_AddItemToArray(ctrls, cJSON_CreateString(ctrl.name.c_str()));
    }
    cJSON_AddItemToObject(identity, "supported_controllers", ctrls);

    cJSON_AddItemToObject(root, "identity", identity);

    // Runtime State Layer
    cJSON* runtime = cJSON_CreateObject();
    cJSON_AddBoolToObject(runtime, "is_online", entity.runtimeState.isOnline);
    cJSON_AddNumberToObject(runtime, "active_endpoint_index", entity.runtimeState.activeEndpointIndex);
    cJSON_AddNumberToObject(runtime, "rssi", entity.runtimeState.rssi);
    cJSON_AddNumberToObject(runtime, "latency_ms", entity.runtimeState.latencyMs);
    cJSON_AddNumberToObject(runtime, "battery_pct", entity.runtimeState.batteryPct);

    cJSON* eps = cJSON_CreateArray();
    for (const auto& ep : entity.endpoints) {
        cJSON* epObj = cJSON_CreateObject();
        cJSON_AddStringToObject(epObj, "uuid", ep.uuid.c_str());
        cJSON_AddStringToObject(epObj, "ip", ep.ip.c_str());
        cJSON_AddItemToArray(eps, epObj);
    }
    cJSON_AddItemToObject(runtime, "endpoints", eps);

    cJSON_AddItemToObject(root, "runtime_state", runtime);

    // AI Annotation Layer
    cJSON* ai = cJSON_CreateObject();
    cJSON_AddStringToObject(ai, "summary", entity.aiAnnotations.summary.c_str());
    cJSON_AddStringToObject(ai, "user_notes", entity.aiAnnotations.userNotes.c_str());
    cJSON_AddStringToObject(ai, "confidence_reason", entity.aiAnnotations.confidenceReason.c_str());
    cJSON_AddStringToObject(ai, "embedding_reference", entity.aiAnnotations.embeddingReference.c_str());

    cJSON* tags = cJSON_CreateArray();
    for (const auto& tag : entity.aiAnnotations.semanticTags) {
        cJSON_AddItemToArray(tags, cJSON_CreateString(tag.c_str()));
    }
    cJSON_AddItemToObject(ai, "semantic_tags", tags);

    cJSON_AddItemToObject(root, "ai_annotations", ai);

    // Lifecycle Layer
    cJSON* lc = cJSON_CreateObject();
    cJSON_AddStringToObject(lc, "state", entity.lifecycle.state.c_str());
    cJSON_AddStringToObject(lc, "retention_policy", entity.lifecycle.retentionPolicy.c_str());
    cJSON_AddBoolToObject(lc, "user_deleted", entity.lifecycle.userDeleted);
    cJSON_AddNumberToObject(lc, "confidence_score", entity.lifecycle.confidenceScore);
    cJSON_AddNumberToObject(lc, "revision", entity.lifecycle.revision);

    cJSON_AddItemToObject(root, "lifecycle", lc);

    // Aliases & Name
    cJSON_AddStringToObject(root, "display_name", entity.displayName.c_str());
    cJSON* aliases = cJSON_CreateArray();
    for (const auto& alias : entity.aliases.systemAliases) {
        cJSON_AddItemToArray(aliases, cJSON_CreateString(alias.c_str()));
    }
    for (const auto& alias : entity.aliases.userAliases) {
        cJSON_AddItemToArray(aliases, cJSON_CreateString(alias.c_str()));
    }
    cJSON_AddItemToObject(root, "aliases", aliases);

    // Relationships Layer
    cJSON* rels = cJSON_CreateArray();
    for (const auto& rel : entity.relationships) {
        cJSON* rObj = cJSON_CreateObject();
        cJSON_AddStringToObject(rObj, "type", rel.type.c_str());
        cJSON_AddStringToObject(rObj, "target_id", rel.targetId.c_str());
        cJSON_AddItemToArray(rels, rObj);
    }
    cJSON_AddItemToObject(root, "relationships", rels);

    char* formatted = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!formatted) return "";

    std::string result(formatted);
    free(formatted);
    return result;
}

int JsonKnowledgeSerializer::ExtractSchemaVersion(const std::string& rawBuffer) {
    if (rawBuffer.empty()) return 1;
    cJSON* root = cJSON_Parse(rawBuffer.c_str());
    if (!root) return 1;

    int ver = 1;
    cJSON* verItem = cJSON_GetObjectItem(root, "schema_version");
    if (verItem && cJSON_IsNumber(verItem)) {
        ver = verItem->valueint;
    }
    cJSON_Delete(root);
    return ver;
}

std::optional<KnowledgeEntity> JsonKnowledgeSerializer::Deserialize(const std::string& rawBuffer) {
    if (rawBuffer.empty()) return std::nullopt;

    cJSON* root = cJSON_Parse(rawBuffer.c_str());
    if (!root || !cJSON_IsObject(root)) {
        if (root) cJSON_Delete(root);
        return std::nullopt;
    }

    KnowledgeEntity entity;

    cJSON* verItem = cJSON_GetObjectItem(root, "schema_version");
    if (verItem && cJSON_IsNumber(verItem)) {
        entity.schemaVersion = verItem->valueint;
    }

    cJSON* idItem = cJSON_GetObjectItem(root, "entity_id");
    if (idItem && cJSON_IsString(idItem)) {
        entity.persistentId = idItem->valuestring;
    } else {
        cJSON_Delete(root);
        return std::nullopt;
    }

    cJSON* nameItem = cJSON_GetObjectItem(root, "display_name");
    if (nameItem && cJSON_IsString(nameItem)) {
        entity.displayName = nameItem->valuestring;
    }

    // Parse Identity Layer
    cJSON* identity = cJSON_GetObjectItem(root, "identity");
    if (identity && cJSON_IsObject(identity)) {
        cJSON* mac = cJSON_GetObjectItem(identity, "mac_address");
        if (mac && cJSON_IsString(mac)) entity.identity.macAddress = mac->valuestring;

        cJSON* serial = cJSON_GetObjectItem(identity, "serial_number");
        if (serial && cJSON_IsString(serial)) entity.identity.serialNumber = serial->valuestring;

        cJSON* vendor = cJSON_GetObjectItem(identity, "vendor");
        if (vendor && cJSON_IsString(vendor)) entity.identity.vendor = vendor->valuestring;

        cJSON* model = cJSON_GetObjectItem(identity, "model");
        if (model && cJSON_IsString(model)) entity.identity.model = model->valuestring;

        cJSON* caps = cJSON_GetObjectItem(identity, "capabilities");
        if (caps && cJSON_IsArray(caps)) {
            int capCount = cJSON_GetArraySize(caps);
            for (int i = 0; i < capCount; ++i) {
                cJSON* item = cJSON_GetArrayItem(caps, i);
                if (cJSON_IsString(item)) {
                    CapabilityModel cap;
                    cap.id = item->valuestring;
                    cap.displayName = item->valuestring;
                    entity.capabilities.AddCapability(cap);
                    entity.identity.capabilities.AddCapability(cap);
                } else if (cJSON_IsObject(item)) {
                    CapabilityModel cap;
                    cJSON* cId = cJSON_GetObjectItem(item, "id");
                    if (cId && cJSON_IsString(cId)) cap.id = cId->valuestring;

                    cJSON* cName = cJSON_GetObjectItem(item, "display_name");
                    if (cName && cJSON_IsString(cName)) cap.displayName = cName->valuestring;

                    cJSON* cDesc = cJSON_GetObjectItem(item, "description");
                    if (cDesc && cJSON_IsString(cDesc)) cap.description = cDesc->valuestring;

                    cJSON* cVer = cJSON_GetObjectItem(item, "version");
                    if (cVer && cJSON_IsString(cVer)) cap.version = cVer->valuestring;

                    cJSON* ops = cJSON_GetObjectItem(item, "operations");
                    if (ops && cJSON_IsArray(ops)) {
                        int opCount = cJSON_GetArraySize(ops);
                        for (int j = 0; j < opCount; ++j) {
                            cJSON* opObj = cJSON_GetArrayItem(ops, j);
                            if (opObj && cJSON_IsObject(opObj)) {
                                OperationDefinition opDef;
                                cJSON* oId = cJSON_GetObjectItem(opObj, "id");
                                if (oId && cJSON_IsString(oId)) opDef.id = oId->valuestring;

                                cJSON* oName = cJSON_GetObjectItem(opObj, "name");
                                if (oName && cJSON_IsString(oName)) opDef.name = oName->valuestring;

                                if (opDef.id.empty()) opDef.id = opDef.name;
                                if (opDef.name.empty()) opDef.name = opDef.id;

                                cJSON* oDName = cJSON_GetObjectItem(opObj, "display_name");
                                if (oDName && cJSON_IsString(oDName)) opDef.displayName = oDName->valuestring;

                                cJSON* oDesc = cJSON_GetObjectItem(opObj, "description");
                                if (oDesc && cJSON_IsString(oDesc)) opDef.description = oDesc->valuestring;

                                cJSON* oRO = cJSON_GetObjectItem(opObj, "read_only");
                                if (oRO && cJSON_IsBool(oRO)) opDef.readOnly = cJSON_IsTrue(oRO);

                                cJSON* oDur = cJSON_GetObjectItem(opObj, "estimated_duration_ms");
                                if (oDur && cJSON_IsNumber(oDur)) opDef.estimatedDurationMs = (uint32_t)oDur->valueint;

                                cJSON* oTO = cJSON_GetObjectItem(opObj, "timeout_ms");
                                if (oTO && cJSON_IsNumber(oTO)) opDef.timeoutMs = (uint32_t)oTO->valueint;

                                cJSON* oIdem = cJSON_GetObjectItem(opObj, "idempotent");
                                if (oIdem && cJSON_IsBool(oIdem)) opDef.idempotent = cJSON_IsTrue(oIdem);

                                cJSON* oSafe = cJSON_GetObjectItem(opObj, "safe");
                                if (oSafe && cJSON_IsBool(oSafe)) opDef.safe = cJSON_IsTrue(oSafe);

                                cJSON* oConf = cJSON_GetObjectItem(opObj, "requires_confirmation");
                                if (oConf && cJSON_IsBool(oConf)) opDef.requiresConfirmation = cJSON_IsTrue(oConf);

                                cJSON* params = cJSON_GetObjectItem(opObj, "parameters");
                                if (params && cJSON_IsArray(params)) {
                                    int paramCount = cJSON_GetArraySize(params);
                                    for (int k = 0; k < paramCount; ++k) {
                                        cJSON* pObj = cJSON_GetArrayItem(params, k);
                                        if (pObj && cJSON_IsObject(pObj)) {
                                            ParameterDefinition pDef;
                                            cJSON* pName = cJSON_GetObjectItem(pObj, "name");
                                            if (pName && cJSON_IsString(pName)) pDef.name = pName->valuestring;

                                            cJSON* pDName = cJSON_GetObjectItem(pObj, "display_name");
                                            if (pDName && cJSON_IsString(pDName)) pDef.displayName = pDName->valuestring;

                                            cJSON* pDesc = cJSON_GetObjectItem(pObj, "description");
                                            if (pDesc && cJSON_IsString(pDesc)) pDef.description = pDesc->valuestring;

                                            cJSON* pReq = cJSON_GetObjectItem(pObj, "required");
                                            if (pReq && cJSON_IsBool(pReq)) pDef.required = cJSON_IsTrue(pReq);

                                            cJSON* pDefVal = cJSON_GetObjectItem(pObj, "default_value");
                                            if (pDefVal && cJSON_IsString(pDefVal)) pDef.defaultValue = pDefVal->valuestring;

                                            cJSON* pUnit = cJSON_GetObjectItem(pObj, "unit");
                                            if (pUnit && cJSON_IsString(pUnit)) pDef.unit = pUnit->valuestring;

                                            opDef.AddParameter(pDef);
                                        }
                                    }
                                }

                                if (!opDef.id.empty()) {
                                    cap.AddOperation(opDef);
                                }
                            }
                        }
                    }

                    if (!cap.id.empty()) {
                        entity.capabilities.AddCapability(cap);
                        entity.identity.capabilities.AddCapability(cap);
                    }
                }
            }
        }
    }

    // Parse Runtime State
    cJSON* runtime = cJSON_GetObjectItem(root, "runtime_state");
    if (runtime && cJSON_IsObject(runtime)) {
        cJSON* online = cJSON_GetObjectItem(runtime, "is_online");
        if (online && cJSON_IsBool(online)) entity.runtimeState.isOnline = cJSON_IsTrue(online);

        cJSON* eps = cJSON_GetObjectItem(runtime, "endpoints");
        if (eps && cJSON_IsArray(eps)) {
            int count = cJSON_GetArraySize(eps);
            for (int i = 0; i < count; ++i) {
                cJSON* epObj = cJSON_GetArrayItem(eps, i);
                if (epObj && cJSON_IsObject(epObj)) {
                    ProtocolEndpoint ep;
                    cJSON* uuid = cJSON_GetObjectItem(epObj, "uuid");
                    if (uuid && cJSON_IsString(uuid)) ep.uuid = uuid->valuestring;

                    cJSON* ip = cJSON_GetObjectItem(epObj, "ip");
                    if (ip && cJSON_IsString(ip)) ep.ip = ip->valuestring;

                    entity.endpoints.push_back(ep);
                }
            }
        }
    }

    // Parse Lifecycle Layer
    cJSON* lc = cJSON_GetObjectItem(root, "lifecycle");
    if (lc && cJSON_IsObject(lc)) {
        cJSON* state = cJSON_GetObjectItem(lc, "state");
        if (state && cJSON_IsString(state)) entity.lifecycle.state = state->valuestring;

        cJSON* policy = cJSON_GetObjectItem(lc, "retention_policy");
        if (policy && cJSON_IsString(policy)) entity.lifecycle.retentionPolicy = policy->valuestring;

        cJSON* userDel = cJSON_GetObjectItem(lc, "user_deleted");
        if (userDel && cJSON_IsBool(userDel)) entity.lifecycle.userDeleted = cJSON_IsTrue(userDel);
    }

    cJSON_Delete(root);
    return entity;
}

} // namespace Persistence
} // namespace NetDiscovery
