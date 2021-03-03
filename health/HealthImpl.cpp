#include <memory>
#include <string_view>

#include <health/utils.h>
#include <health2impl/Health.h>

using ::android::hardware::health::InitHealthdConfig;
using ::android::hardware::health::V2_1::IHealth;

using namespace std::literals;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

int dragon_update_health_info(HealthInfo* health_info);

class DragonHealthImpl : public Health {
  public:
    DragonHealthImpl(std::unique_ptr<healthd_config>&& config)
        : Health(std::move(config)) {}

  protected:
    void UpdateHealthInfo(HealthInfo* health_info) override {
        dragon_update_health_info(health_info);
    }
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    using ::android::hardware::health::V2_1::implementation::DragonHealthImpl;
    if (instance != "default"sv) {
        return nullptr;
    }
    auto config = std::make_unique<healthd_config>();
    InitHealthdConfig(config.get());

    return new DragonHealthImpl(std::move(config));
}
