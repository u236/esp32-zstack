#ifndef ZSTACK_H
#define ZSTACK_H

#define ZSTACK_CONFIGURATION_MARKER                 0x42   // Some value to mark that the configuration was written by this controller
#define ZSTACK_ENDPOINT_ID                          0x01   // Default coordinator endpoint id
#define ZSTACK_ENDPOINT_PROFILE_ID                  0x0104 // ZigBee Home Automation Profile
#define ZSTACK_ENDPOINT_DEVICE_ID                   0x0005 // Default for ZigBee Home Automation devices

#define ZSTACK_PORT                                 Serial2
#define ZSTACK_FRAME_FLAG                           0xFE
#define ZSTACK_BUFFER_SIZE                          256
#define ZSTACK_MINIMAL_LENGTH                       5
#define ZSTACK_REQUEST_TIMEOUT                      10000

#define SYS_OSAL_NV_ITEM_INIT                       0x2107
#define SYS_OSAL_NV_READ                            0x2108
#define SYS_OSAL_NV_WRITE                           0x2109
#define AF_REGISTER                                 0x2400
#define ZDO_STARTUP_FROM_APP                        0x2540

#define SYS_RESET_IND                               0x4180
#define ZDO_STATE_CHANGE_IND                        0x45C0
#define APP_CNF_BDB_COMMISSIONING_NOTIFICATION      0x4F80

#define ZCD_NV_STARTUP_OPTION                       0x0003
#define ZCD_NV_MARKER                               0x0060
#define ZCD_NV_PRECFGKEY                            0x0062
#define ZCD_NV_PRECFGKEYS_ENABLE                    0x0063
#define ZCD_NV_PANID                                0x0083
#define ZCD_NV_CHANLIST                             0x0084
#define ZCD_NV_LOGICAL_TYPE                         0x0087
#define ZCD_NV_ZDO_DIRECT_CB                        0x008F
#define ZCD_NV_TCLK_TABLE                           0x0101

#include "Arduino.h"

enum ZStackEvent
{
    resetDetected,
    configurationMismatch,
    configurationUpdated,
    configurationFailed,
    statusChanged,
    coordinatorStarting,
    coordinatorReady,
    coordinatorFailed
};

typedef void (*ZStackCallback) (ZStackEvent event, void *data, size_t length);

#pragma pack(push, 1)

struct nvInitRequestStruct
{
    uint16_t id;
    uint16_t itemLength;
    uint8_t  dataLength;
};

struct nvReadRequestStruct
{
    uint16_t id;
    uint8_t  offset;
};

struct nvWriteRequestStruct
{
    uint16_t id;
    uint8_t  offset;
    uint8_t  length;
};

struct nvReadReplyStruct
{
    uint8_t  status;
    uint8_t  length;
};

struct nvDataStruct
{
    uint16_t id;
    uint8_t  length;
    uint8_t  value[16];
};

struct afRegisterRequestStruct
{
    uint8_t  endpointId;
    uint16_t profileId;
    uint16_t deviceId;
    uint8_t  version;
    uint8_t  latency;
};

#pragma pack(pop)

class ZStack
{
    public:

        ZStack(ZStackCallback callback, uint8_t channel, uint16_t panId, int8_t bsl, int8_t rst, int8_t tx, int8_t rx, int8_t core = 0);

        void reset(void);
        void clear(void);
        void parseInput(uint8_t *buffer, size_t length);

    private:

        ZStackCallback m_callback;
        uint8_t m_channel;
        int8_t m_bslPin, m_rstPin;

        bool m_clear;
        uint8_t m_status;

        nvDataStruct m_nvData[8];
        uint8_t m_nvIndex;

        void parseFrame(uint16_t command, uint8_t *data, size_t length);
        void sendFrame(uint16_t command, uint8_t *data, size_t length);

        void readNvItem(void);
        void writeNvItem(void);

        static void inputTask(void *data);

};

#endif
