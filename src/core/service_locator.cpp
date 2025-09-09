#include "service_locator.h"
#include <unordered_set>

namespace RainmeterManager {
namespace Core {

ServiceLocator::~ServiceLocator() {
    Clear();
    Logger::GetInstance().LogInfo(L"ServiceLocator: Destroyed");
}

} // namespace Core
} // namespace RainmeterManager
