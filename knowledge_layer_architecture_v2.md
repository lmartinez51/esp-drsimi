# Knowledge Layer Architecture v2.1 Specification
**ESP32-S3 Dr. Simi Voice Assistant & ESP-Claw Platform**  
**Document Status:** Refined Architectural Contract & Blueprint (Approved for Implementation)  
**Target Subsystems:** KnowledgeStore, DeviceSessionManager, Semantic Layer, Persistence, ESP-Claw IPC  
**Revision:** 2.1 (Incorporates 5 Mandatory Architectural Design Review Refinements)  

---

## Architectural Review Refinements Summary (v2.1)

The following five mandatory refinements have been incorporated into this specification following the architectural design review:

1. **Asynchronous Write-Back Cache (Flash Wear Protection):** `KnowledgeStore::UpdateFromDiscovery()` updates in-memory entities and flags them as dirty (`isDirty = true`). Disk writes to LittleFS are debounced and executed asynchronously by a background task/idle flush handler to prevent flash wear and task starvation on Core 0.
2. **Zero-Copy / Shared Pointer Query APIs:** All query APIs (`FindById`, `FindByUUID`, `FindByMAC`, `FindByAlias`, `FindByCapability`, `FindByRoom`, `GetLoadedEntities`) return `std::shared_ptr<const KnowledgeEntity>` or `std::vector<std::shared_ptr<const KnowledgeEntity>>` to eliminate dynamic heap allocation overhead.
3. **Memory-Disk Lock Separation (Thread Safety & Priority Protection):** Mutex protection inside `KnowledgeStore` is strictly scoped to in-memory `m_entities` map operations and is explicitly released before initiating LittleFS disk I/O.
4. **Explicit Entity Type Initialization & Serialization:** `KnowledgeEntity::type` is explicitly assigned `EntityType::Device` during discovery conversion and serialized/deserialized in `JsonKnowledgeSerializer` to guarantee restored devices are never dropped at boot.
5. **Refactored Migration Sequence:** Dead code removal (`FileKnowledgeRepository`, `StorageManager`, etc.) is prioritized as Phase 1 to clean the codebase prior to serializer and driver refactoring.

---

## 1. High-Level Architecture

The Knowledge Layer v2.1 establishes a centralized cognitive database and volatile session manager for all network and automation entities. It operates between live network discovery/IPC inputs and the frozen Phase C/D/E Execution Runtime.

### System Architecture Layout

```
                               ┌───────────────────────────┐
                               │  OpenAI Realtime / WebRTC │
                               │   (Core 1, Priority 7-10) │
                               └─────────────┬─────────────┘
                                             │ (Async Tool Call)
                                             ▼
                               ┌───────────────────────────┐
                               │     ESP-Claw (Lua VM)     │
                               │   (Core 0, Priority 3)    │
                               └─────────────┬─────────────┘
                                             │
                       ┌─────────────────────┴─────────────────────┐
                       │ FreeRTOS Queue (netdiscovery_intent_queue)│
                       └─────────────────────┬─────────────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │    NetDiscovery IPC       │
                               └─────────────┬─────────────┘
                                             │
                     ┌───────────────────────┼───────────────────────┐
                     │                       │                       │
                     ▼                       ▼                       ▼
       ┌───────────────────────────┐ ┌───────────────┐ ┌───────────────────────────┐
       │   DeviceSessionManager    │ │ KnowledgeStore│ │     SSDP / Discovery      │
       │    (RAM Only, Max 4)      │ │ (v2.1 SSOT +  │ │   (DeviceRegistry Buffer) │
       │                           │ │ Dirty Cache)  │ │                           │
       └─────────────┬─────────────┘ └───────┬───────┘ └─────────────┬─────────────┘
                     │                       │                       │
                     │ (Session Hit)         │ (Store Query - Shared)│ (Live Observations)
                     └───────────────────────┼───────────────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │    SemanticOrchestrator   │
                               │   (Coordinator Pipeline)  │
                               └─────────────┬─────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │     Intent Compiler       │
                               │   (IntentAST -> Plan)     │
                               └─────────────┬─────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │   ExecutionPlanExecutor   │
                               │  (Frozen Runtime Phase D) │
                               └─────────────┬─────────────┘
                                             │
                                             ▼ (Async Background Flush)
                               ┌───────────────────────────┐
                               │  JsonKnowledgeSerializer  │
                               │    (cJSON Engine v2.1)    │
                               └─────────────┬─────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │    FileKnowledgeStore     │
                               │ (Atomic POSIX File Write) │
                               └─────────────┬─────────────┘
                                             │
                                             ▼
                               ┌───────────────────────────┐
                               │   LittleFS Flash Memory   │
                               │ (/littlefs/knowledge/...) │
                               └───────────────────────────┘
```

### Module Ownership & Boundary Contract

* **`KnowledgeStore` (Single Source of Truth)**: Absolute owner of persistent entity metadata, network fingerprint mappings, user attributes, credentials, write-back dirty cache, and journal history.
* **`DeviceSessionManager`**: Sole owner of dynamic, volatile conversation device sessions in RAM (max 4 concurrent, 300s TTL).
* **`DeviceRegistry`**: Owned by `DiscoveryManager`. Used strictly as a transient staging buffer for raw discovery evidence during active scans.
* **`SemanticOrchestrator`**: Pure coordinator. Owns no persistent state or session tables.

---

## 2. Single Source of Truth (SSOT)

`KnowledgeStore` is designated as the absolute single source of truth for all device metadata.

### Component Survival & Role Matrix

| Component Name | Role in Architecture v2.1 | Status | Rationale |
| :--- | :--- | :--- | :--- |
| **`KnowledgeStore`** | Central Cognitive Engine & Query Facade | **Survives (Unified Engine)** | Retains single-instance ownership of entity map, thread-safe mutex, and zero-copy query facade. |
| **`KnowledgeEntity`** | Core Data Model v2.1 | **Survives (Promoted v2.1)** | Expanded to carry complete user, discovery, runtime, credential, and network metadata. |
| **`IKnowledgeStore`** | Mechanical Storage Interface | **Survives (Contract Interface)** | Defines low-level byte persistence contract (`SaveEntityData`, `LoadAllEntities`). |
| **`FileKnowledgeStore`** | LittleFS Storage Driver | **Survives (Rebound to cJSON)** | Implements `IKnowledgeStore` using POSIX filesystem calls and atomic file replacement. |
| **`JsonKnowledgeSerializer`** | Serializer Engine | **Survives (Promoted to Primary)** | Replaces custom line-formatted text parsing with versioned cJSON format. |
| **`DeviceSessionManager`** | Volatile Conversation Cache | **New Component (RAM Only)** | Manages 300s rolling session bindings (max 4 concurrent entries). |
| **`IKnowledgeRepository`** | Legacy Repository Interface | **Deleted / Merged** | Redundant abstraction layer. Methods merged directly into `KnowledgeStore`. |
| **`FileKnowledgeRepository`** | Legacy Repository Driver | **Deleted** | Orphaned duplicate architecture. Features merged into `KnowledgeStore` + `FileKnowledgeStore`. |
| **`InMemoryKnowledgeRepo`** | Legacy Mock Repository | **Deleted** | Obsolete test mock. |
| **`StorageManager`** | Legacy Container | **Deleted** | Redundant container layer. |

---

## 3. KnowledgeEntity v2.1 Field Specification

```cpp
struct KnowledgeEntity {
    // 1. Schema Versioning
    uint32_t schemaVersion{2}; // [MANDATORY, PERSISTENT] Schema version tag

    // 2. Identity
    std::string persistentId;  // [MANDATORY, PERSISTENT] Unique persistent entity key
    std::string primaryMacAddress; // [OPTIONAL, PERSISTENT] Hardware MAC address
    std::string upnpUuid;      // [OPTIONAL, PERSISTENT] UPnP device UUID string

    // 3. User Metadata
    std::string displayName;   // [MANDATORY, PERSISTENT] User-visible friendly name
    std::string assignedRoom;  // [OPTIONAL, PERSISTENT] Assigned room/location
    std::string customCategory;// [OPTIONAL, PERSISTENT] User-overridden category
    EntityAliases aliases;     // [MANDATORY, PERSISTENT] System and user aliases array

    // 4. Discovery & Classification Metadata
    EntityType type{EntityType::Device}; // [MANDATORY, PERSISTENT] Explicit entity type tag (Fixed for boot loading)
    DeviceClass primaryClass;  // [MANDATORY, PERSISTENT] Primary class taxonomy
    std::vector<DeviceRole> roles; // [OPTIONAL, PERSISTENT] Secondary functional roles
    EntityIdentity identity;   // [MANDATORY, PERSISTENT] Vendor, model, serial number

    // 5. Capabilities & Actions
    CapabilitySet capabilities; // [MANDATORY, PERSISTENT] Supported capability set
    std::vector<CapabilityProfile> capabilityProfiles; // [OPTIONAL, PERSISTENT] Action profiles
    std::vector<NormalizedService> normalizedServices; // [OPTIONAL, PERSISTENT] Services
    std::vector<ServiceDescriptor> services;           // [OPTIONAL, PERSISTENT] Raw services
    std::vector<ControllerCandidate> compatibleControllers; // [MANDATORY, PERSISTENT] Controllers

    // 6. Network Endpoints
    std::vector<ProtocolEndpoint> endpoints; // [MANDATORY, PERSISTENT] IP, headers, URLs

    // 7. Credentials & Auth
    std::map<std::string, std::string> credentials; // [OPTIONAL, PERSISTENT] Auth tokens

    // 8. Timestamps & State
    int64_t firstDiscovered{0}; // [MANDATORY, PERSISTENT] Epoch timestamp of discovery
    int64_t lastSeen{0};        // [MANDATORY, PERSISTENT] Epoch timestamp of last observation
    bool isArchived{false};     // [MANDATORY, PERSISTENT] Soft-archive flag

    // 9. Operational History
    std::vector<JournalEntry> journal; // [OPTIONAL, PERSISTENT] Administrative event log
    std::vector<CommunicationRecord> commHistory; // [OPTIONAL, PERSISTENT] Execution log

    // 10. Runtime-Only & Dirty Tracking (NEVER PERSISTED)
    int runtimeConfidenceScore{0}; // [DERIVED, RUNTIME-ONLY] Dynamic confidence score
    bool isReachable{false};       // [DERIVED, RUNTIME-ONLY] Reachability status flag
    bool isDirty{false};           // [RUNTIME-ONLY] Write-back dirty cache flag for LittleFS
};
```

---

## 4. KnowledgeStore Public API v2.1 Specification (Zero-Copy)

```cpp
namespace NetDiscovery {

class KnowledgeStore {
public:
    explicit KnowledgeStore(std::unique_ptr<IKnowledgeStore> backend);
    ~KnowledgeStore() = default;

    // --- Lifecycle & Boot ---
    void Initialize();
    void ResolveKnownNetwork(const NetworkFingerprint& network);
    void FlushDirtyEntities(); // Asynchronous Write-Back Cache Flush to LittleFS

    // --- Zero-Copy Semantic Queries (Read-Only) ---
    std::shared_ptr<const KnowledgeEntity> FindById(const std::string& entityId) const;
    std::shared_ptr<const KnowledgeEntity> FindByUUID(const std::string& uuid) const;
    std::shared_ptr<const KnowledgeEntity> FindByMAC(const std::string& mac) const;
    std::shared_ptr<const KnowledgeEntity> FindByAlias(const std::string& alias) const;
    std::vector<std::shared_ptr<const KnowledgeEntity>> FindByCapability(Capability cap) const;
    std::vector<std::shared_ptr<const KnowledgeEntity>> FindByManufacturer(const std::string& vendor) const;
    std::vector<std::shared_ptr<const KnowledgeEntity>> FindByRoom(const std::string& room) const;
    std::shared_ptr<const KnowledgeEntity> ResolveDevice(const std::string& queryIdentifier) const;
    std::string                             ResolveFriendlyName(const std::string& entityId) const;
    std::vector<std::shared_ptr<const KnowledgeEntity>> GetLoadedEntities() const;

    // --- Ingestion & Merging ---
    void UpdateFromDiscovery(const LogicalDevice& liveDevice);

    // --- User CRUD Operations ---
    bool RenameDevice(const std::string& entityId, const std::string& newDisplayName);
    bool SetAliases(const std::string& entityId, const std::vector<std::string>& userAliases);
    bool SetRoom(const std::string& entityId, const std::string& roomName);
    bool SetCategory(const std::string& entityId, DeviceClass primaryClass);
    bool DeleteDevice(const std::string& entityId);
    bool ArchiveDevice(const std::string& entityId);
    bool RestoreDevice(const std::string& entityId);
    bool ForgetDevice(const std::string& entityId);
    size_t PurgeArchivedEntities();

    // --- State & Credentials ---
    void UpdateCredentials(const std::string& entityId, const std::string& key, const std::string& value);
    void AppendJournal(const std::string& entityId, JournalEventType type, const std::string& description);
    void AppendCommunicationRecord(const std::string& entityId, const CommunicationRecord& record);
    KnowledgeConfidence ComputeConfidence(const KnowledgeEntity& entity) const;
};

} // namespace NetDiscovery
```

---

## 5. DeviceSessionManager Specification

```cpp
namespace NetDiscovery {

struct DeviceSession {
    std::string sessionId;             // Session identifier (e.g. "S-101")
    std::string boundEntityId;         // Bound KnowledgeEntity persistentId
    std::string boundUuid;             // Bound device UUID
    ProtocolEndpoint selectedEndpoint; // Confirmed IP & port endpoint
    std::string selectedController;    // Selected IDeviceController name
    DeviceClass deviceClass;           // Primary class (e.g. MediaRenderer)
    int64_t lastInteractionTimestamp;  // Epoch ms of last interaction
    int64_t expirationTimestamp;       // Epoch ms when session expires (T_last + 300s)
};

class DeviceSessionManager {
public:
    DeviceSessionManager() = default;

    std::optional<DeviceSession> GetSession(const std::string& sessionId);
    std::optional<DeviceSession> FindSessionByEntity(const std::string& entityId);
    
    DeviceSession CreateOrUpdateSession(
        const std::string& sessionId,
        const KnowledgeEntity& entity,
        const ProtocolEndpoint& endpoint,
        const std::string& controllerName
    );

    void TouchSession(const std::string& sessionId);
    void CloseSession(const std::string& sessionId);
    void InvalidateEntitySessions(const std::string& entityId);
    void PurgeExpiredSessions();

private:
    static constexpr size_t MAX_CONCURRENT_SESSIONS = 4;
    static constexpr int64_t SESSION_TTL_MS = 300000; // 5 minutes (300 seconds)

    mutable std::mutex m_sessionMutex;
    std::map<std::string, DeviceSession> m_sessions; // In-memory RAM cache
};

} // namespace NetDiscovery
```

---

## 6. Refactored Migration Strategy (Phases 1–7)

```
Phase 1: Dead Code Purge ──► Phase 2: cJSON & Boot Fix ──► Phase 3: Zero-Copy Queries
                                                                    │
Phase 6: Session Manager ◄── Phase 5: Complete CRUD   ◄── Phase 4: Write-Back Cache
       │
       ▼
Phase 7: End-to-End Qualification & Freeze
```

1. **Phase 1: Dead Code Purge & Workspace Cleanup**  
   Remove `FileKnowledgeRepository`, `InMemoryKnowledgeRepository`, `StorageManager`, and `IKnowledgeRepository` from disk and `CMakeLists.txt`.
2. **Phase 2: cJSON Integration & Boot Persistence Fix**  
   Bind `JsonKnowledgeSerializer` inside `KnowledgeStore`, explicitly set `entity.type = EntityType::Device`, and resolve the boot-loading drop bug.
3. **Phase 3: Zero-Copy Query API Implementation**  
   Implement `FindById`, `FindByUUID`, `FindByMAC`, `FindByAlias`, `FindByCapability`, `FindByRoom`, `ResolveDevice`, returning `std::shared_ptr<const KnowledgeEntity>`.
4. **Phase 4: Asynchronous Write-Back Dirty Cache Implementation**  
   Implement dirty-flag tracking in `KnowledgeStore` and an asynchronous `FlushDirtyEntities()` mechanism to protect LittleFS flash from wear.
5. **Phase 5: User Device Management (CRUD) Implementation**  
   Implement `RenameDevice`, `SetAliases`, `SetRoom`, `SetCategory`, `DeleteDevice`, `ArchiveDevice`, `RestoreDevice`, `ForgetDevice`, `PurgeArchivedEntities`.
6. **Phase 6: Volatile DeviceSessionManager Integration**  
   Implement `DeviceSessionManager` in RAM with 300s TTL and 4-session capacity. Wire IPC listener to check active sessions before invoking `DeviceMatcher`.
7. **Phase 7: End-to-End System Qualification**  
   Execute full multi-device boot persistence, zero heap leak, session retention, and flash protection qualification tests.

---

## 7. Authoritative Final Architecture Diagram (v2.1 Reconciled)

```
===================================================================================
                      FINAL RECONCILED ARCHITECTURE v2.1
===================================================================================

  [ WebRTC Task ] (Core 1)
        │
        ▼ (IPC Intent / Query)
  [ ESP-Claw Lua Engine ] (Core 0)
        │
        ▼ (Queue: netdiscovery_intent_queue)
  [ NetDiscovery IPC ]
        │
        ├──────────────────────────────────────┐
        ▼ (1. Check Session)                   ▼ (2. Query Metadata)
  ┌───────────────────────────┐          ┌───────────────────────────┐
  │   DeviceSessionManager    │          │      KnowledgeStore       │
  │   (RAM Cache, 300s TTL)   │          │  (v2.1 Single Source Truth)│
  └─────────────┬─────────────┘          └─────────────┬─────────────┘
                │ (Session Match)                      │ (Zero-Copy Query)
                └──────────────────┬───────────────────┘
                                   │
                                   ▼
                      ┌───────────────────────────┐
                      │    SemanticOrchestrator   │
                      └────────────┬──────────────┘
                                   │
                                   ▼
                      ┌───────────────────────────┐
                      │      Intent Compiler      │
                      │  (IntentDocument->AST)    │
                      └────────────┬──────────────┘
                                   │
                                   ▼
                      ┌───────────────────────────┐
                      │   ExecutionPlanExecutor   │
                      │ (Frozen Runtime Phase D)  │
                      └────────────┬──────────────┘
                                   │
                                   ▼ (Async Write-Back Flush)
                      ┌───────────────────────────┐
                      │  JsonKnowledgeSerializer  │
                      └────────────┬──────────────┘
                                   │
                                   ▼
                      ┌───────────────────────────┐
                      │    FileKnowledgeStore     │
                      └────────────┬──────────────┘
                                   │
                                   ▼
                      ┌───────────────────────────┐
                      │   LittleFS Flash Memory   │
                      └───────────────────────────┘

===================================================================================
```

This specification is frozen and serves as the authoritative blueprint for implementation.
