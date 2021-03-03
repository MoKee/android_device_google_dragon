#define LOG_TAG "libgatekeeper_shim"

#include <dlfcn.h>
#include <log/log.h>
#include <gatekeeper/gatekeeper_messages.h>

/* Copied from gatekeeper_messages.cpp */
struct __attribute__((__packed__)) serial_header_t
{
    uint32_t error;
    uint32_t user_id;
};

namespace gatekeeper
{
    /*
     * Old blob required: _ZN10gatekeeper13EnrollRequestC1EjPNS_11SizedBufferES2_S2_
     * EnrollRequest::EnrollRequest(unsigned int, SizedBuffer*, SizedBuffer*, SizedBuffer*)
     *
     * New function: _ZN10gatekeeper13EnrollRequestC1EjNS_11SizedBufferES1_S1_
     * EnrollRequest::EnrollRequest(unsigned int, SizedBuffer, SizedBuffer, SizedBuffer)
     */

    extern "C" void _ZN10gatekeeper13EnrollRequestC1EjPNS_11SizedBufferES2_S2_(EnrollRequest *thiz,
                                                                               uint32_t user_id,
                                                                               SizedBuffer *password_handle,
                                                                               SizedBuffer *provided_password,
                                                                               SizedBuffer *enrolled_password)
    {
        new (thiz) EnrollRequest(user_id,
                                 password_handle == nullptr ? SizedBuffer(nullptr, 0) : SizedBuffer(const_cast<uint8_t *>(password_handle->Data<const uint8_t>()), password_handle->size()),
                                 provided_password == nullptr ? SizedBuffer(nullptr, 0) : SizedBuffer(const_cast<uint8_t *>(provided_password->Data<const uint8_t>()), provided_password->size()),
                                 enrolled_password == nullptr ? SizedBuffer(nullptr, 0) : SizedBuffer(const_cast<uint8_t *>(enrolled_password->Data<const uint8_t>()), enrolled_password->size()));
    }

#ifdef __LP64__
    /*
     * Old blob required: _ZN10gatekeeper13VerifyRequestC1EjmPNS_11SizedBufferES2_
     * VerifyRequest::VerifyRequest(unsigned int, unsigned long, SizedBuffer*, SizedBuffer*)
     *
     * New function: _ZN10gatekeeper13VerifyRequestC1EjmNS_11SizedBufferES1_
     * VerifyRequest::VerifyRequest(unsigned int, unsigned long, SizedBuffer, SizedBuffer)
     */
    extern "C" void _ZN10gatekeeper13VerifyRequestC1EjmPNS_11SizedBufferES2_(VerifyRequest *thiz,
                                                                             uint32_t user_id,
                                                                             uint64_t challenge,
                                                                             SizedBuffer *enrolled_password_handle,
                                                                             SizedBuffer *provided_password_payload)
    {
        new (thiz) VerifyRequest(user_id,
                                 challenge,
                                 enrolled_password_handle == nullptr ? SizedBuffer(nullptr, 0) : SizedBuffer(const_cast<uint8_t *>(enrolled_password_handle->Data<const uint8_t>()), enrolled_password_handle->size()),
                                 provided_password_payload == nullptr ? SizedBuffer(nullptr, 0) : SizedBuffer(const_cast<uint8_t *>(provided_password_payload->Data<const uint8_t>()), provided_password_payload->size()));
    }
#endif

    /*
     * _ZN10gatekeeper17GateKeeperMessage11DeserializeEPKhS2_
     * gatekeeper::GateKeeperMessage::Deserialize(unsigned char const*, unsigned char const*)
     *
     * This function actually exists in Android 11, but it was changed to return ERROR_NONE
     * as long as deserialization succeeded. This could confuse the TlkGateKeeperDevice::Verify in gatekeeper HAL
     * and finally lead to crashes like "Failed to decrypt blob".
     *
     * It's interpositioned by this one below so that the old behavior, where the error code from serialization
     * side is returned, can be restored.
     *
     * Yeah the old behavior seemes like a bug, but the blobs are already hardcoded like that.
     */
    extern "C" gatekeeper_error_t _ZN10gatekeeper17GateKeeperMessage11DeserializeEPKhS2_(GateKeeperMessage *thiz,
                                                                                         const uint8_t *payload,
                                                                                         const uint8_t *end)
    {
        void *libgk_handle = NULL;
        gatekeeper_error_t (*orig_func_handle)(GateKeeperMessage * thiz, const uint8_t *payload, const uint8_t *end) = NULL;
        const serial_header_t *header = reinterpret_cast<const serial_header_t *>(payload);
        gatekeeper_error_t ret = ERROR_INVALID;

        libgk_handle = dlopen("libgatekeeper.so", RTLD_LAZY | RTLD_LOCAL);
        if (libgk_handle == NULL)
        {
            ALOGE("libgatekeeper.so not found?\n");
            return ERROR_INVALID;
        }

        orig_func_handle = reinterpret_cast<gatekeeper_error_t (*)(GateKeeperMessage *, const uint8_t *, const uint8_t *)>(dlsym(libgk_handle, "_ZN10gatekeeper17GateKeeperMessage11DeserializeEPKhS2_"));
        if (orig_func_handle == NULL)
        {
            ALOGE("Deserialize() not found?\n");
            dlclose(libgk_handle);
            return ERROR_INVALID;
        }

        if (end - payload < sizeof(serial_header_t))
        {
            ALOGE("Truncated payload. payload = %p, end = %p\n", payload, end);
            dlclose(libgk_handle);
            return ERROR_INVALID;
        }

        if (header->error == ERROR_NONE)
        {
            ALOGI("Passthrough payload.\n");
            ret = orig_func_handle(thiz, payload, end);
        }
        else
        {
            ALOGI("Simulate old return value behavior.\n");
            ret = orig_func_handle(thiz, payload, end);

            if (ret == ERROR_NONE)
            {
                /* Old blob relies on this behavior for processing failures */
                ret = static_cast<gatekeeper_error_t>(header->error);
            }
        }

        dlclose(libgk_handle);

        return ret;
    }

    /*
     * Old blob required: _ZN10gatekeeper14EnrollResponseC1Ev
     * EnrollResponse::EnrollResponse()
     *
     * No corresponding new function - use the other constructor.
     */
    extern "C" void _ZN10gatekeeper14EnrollResponseC1Ev(EnrollResponse *thiz)
    {
        uint8_t *payload_space = new uint8_t[128]; /* Will be automatically freed in SizedBuffer destructor? */
        new (thiz) EnrollResponse(0, SizedBuffer(payload_space, sizeof(payload_space)));
    }

    extern "C" void _ZN10gatekeeper14EnrollResponseD1Ev(EnrollResponse *thiz)
    {
        (void)thiz;
    }

    extern "C" void _ZN10gatekeeper13EnrollRequestD1Ev(EnrollRequest *thiz)
    {
        (void)thiz;
    }

    extern "C" void _ZN10gatekeeper14VerifyResponseD1Ev(VerifyResponse *thiz)
    {
        (void)thiz;
    }

    extern "C" void _ZN10gatekeeper13VerifyRequestD1Ev(VerifyRequest *thiz)
    {
        (void)thiz;
    }

}
