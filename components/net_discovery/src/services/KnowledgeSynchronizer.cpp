#include "services/KnowledgeSynchronizer.h"
#include <chrono>

namespace NetDiscovery {

KnowledgeSynchronizer::KnowledgeSynchronizer(KnowledgeStore& store)
    : m_store(store)
{
}

void KnowledgeSynchronizer::OnExecutionCompleted(const std::string& entityId, 
                                                 const std::string& transportName, 
                                                 const ExecutionResult& result) {
    CommunicationRecord record;
    record.transportName = transportName;
    record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    record.durationMs = result.elapsedTimeMs;
    record.status = result.status;
    
    if (result.status != ExecutionStatus::Success) {
        record.failureClassification = result.errorMessage.empty() ? result.transportDiagnostics.rawPayload : result.errorMessage;
    }

    m_store.AppendCommunicationRecord(entityId, record);
}

} // namespace NetDiscovery
