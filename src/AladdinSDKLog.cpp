#include "AladdinSDKLog.h"
#include "AladdinSDKLogPack.h"

bool aladdinSDK_log_init(aladdinSDK_log_handle* plog_handle, const char* module_name, const char* module_version, const char* sdk_name, const char* sdk_version)
{
    if (plog_handle == NULL || ((module_name == NULL || module_version == NULL) && (sdk_name == NULL || sdk_version == NULL)))
        return false;

    AladdinSDKLogPack* handle = new AladdinSDKLogPack(module_name, module_version, sdk_name, sdk_version);
    if (handle == NULL)
        return false;

    *plog_handle = (aladdinSDK_log_handle)handle;
    return true;
}

bool aladdinSDK_log_auth(aladdinSDK_log_handle log_handle, int* plog_auth)
{
    if (log_handle == NULL)
        return false;

    AladdinSDKLogPack* handle = (AladdinSDKLogPack*)log_handle;
    *plog_auth = handle->logAuth();

    return true;
}

bool aladdinSDK_log_push(aladdinSDK_log_handle log_handle, aladdinSDK_log_type log_type, const char* log_buf, int log_len, long log_time, const char* log_str, aladdinSDK_log_cb log_cb)
{
    if (log_handle == NULL)
        return false;

    AladdinSDKLogPack* handle = (AladdinSDKLogPack*)log_handle;
    bool ret = handle->logSent((int)log_type, log_buf, log_len, log_time, log_str, log_cb);

    return ret;
}

bool aladdinSDK_log_vers(aladdinSDK_log_handle log_handle, char* plog_vers)
{
    if (log_handle == NULL)
        return false;

    AladdinSDKLogPack* handle = (AladdinSDKLogPack*)log_handle;
    handle->version(plog_vers);

    return true;
}

bool aladdinSDK_log_free(aladdinSDK_log_handle* plog_handle)
{
    if (plog_handle == NULL)
        return false;

    AladdinSDKLogPack* handle = (AladdinSDKLogPack*)(*plog_handle);
    if (handle != NULL)
        delete handle;

    *plog_handle = NULL;
	return true;
}

