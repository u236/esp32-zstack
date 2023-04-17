#ifndef ZSTACK_H
#define ZSTACK_H

#define ZSTACK_CONFIGURATION_MARKER                 0x42   // some value to mark that the configuration was written by this controller
#define ZSTACK_ENDPOINT_ID                          0x01   // default coordinator endpoint id
#define ZSTACK_ENDPOINT_PROFILE_ID                  0x0104 // ZigBee Home Automation Profile
#define ZSTACK_ENDPOINT_DEVICE_ID                   0x0005 // default for ZigBee Home Automation devices

#define ZSTACK_PORT                                 Serial2
#define ZSTACK_FRAME_FLAG                           0xFE
#define ZSTACK_BUFFER_SIZE                          256
#define ZSTACK_MINIMAL_LENGTH                       5
#define ZSTACK_REQUEST_TIMEOUT                      10000

#define SYS_OSAL_NV_ITEM_INIT                       0x2107
#define SYS_OSAL_NV_READ                            0x2108
#define SYS_OSAL_NV_WRITE                           0x2109
#define AF_REGISTER                                 0x2400
#define AF_DATA_REQUEST                             0x2401
#define ZDO_BIND_REQ                                0x2521
#define ZDO_MGMT_PERMIT_JOIN_REQ                    0x2536
#define ZDO_STARTUP_FROM_APP                        0x2540
#define UTIL_GET_DEVICE_INFO                        0x2700

#define SYS_RESET_IND                               0x4180
#define AF_DATA_CONFIRM                             0x4480
#define AF_INCOMING_MSG                             0x4481
#define ZDO_BIND_RSP                                0x45A1
#define ZDO_MGMT_PERMIT_JOIN_RSP                    0x45B6
#define ZDO_STATE_CHANGE_IND                        0x45C0
#define ZDO_END_DEVICE_ANNCE_IND                    0x45C1
#define ZDO_LEAVE_IND                               0x45C9
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

#define AF_DISCV_ROUTE                              0x20
#define AF_DEFAULT_RADIUS                           0x0F

#define ADDRESS_MODE_NOT_PRESENT                    0x00
#define ADDRESS_MODE_GROUP                          0x01
#define ADDRESS_MODE_16_BIT                         0x02
#define ADDRESS_MODE_64_BIT                         0x03
#define ADDRESS_MODE_BROADCAST                      0xFF

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
    coordinatorFailed,
    permitJoinChanged,
    permitJoinFailed,
    deviceJoinedNetwork,
    deviceLeftNetwork,
    requestEnqueued,
    requestFailed,
    requestFinished,
    bindEnqueued,
    bindFailed,
    bindFinished,
    messageReceived
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

struct dataRequestStruct
{
    uint16_t shortAddress;
    uint8_t  dstEndpointId;
    uint8_t  srcEndpointId;
    uint16_t clusterId;
    uint8_t  transactionId;
    uint8_t  options;
    uint8_t  radius;
    uint8_t  length;
};

struct bindRequestStruct
{
    uint16_t shortAddress;
    uint64_t srcAddress;
    uint8_t  srcEndpointId;
    uint16_t clusterId;
    uint8_t  dstAddressMode;
    uint64_t dstAddress;
    uint8_t  dstEndpointId;
};

struct permitJoinRequestStruct
{
    uint8_t  mode;
    uint16_t dstAddress;
    uint8_t  duration;
    uint8_t  significance;
};

struct deviceAnnounceStruct
{
    uint16_t shortAddress;
    uint64_t ieeeAddress;
    uint8_t  capabilities;
};

struct deviceLeaveStruct
{
    uint16_t shortAddress;
    uint64_t ieeeAddress;
    uint8_t  request;
    uint8_t  remove;
    uint8_t  rejoin;
};

struct dataConfirmStruct
{
    uint8_t  status;
    uint8_t  endpointId;
    uint8_t  transactionId;
};

struct incomingMessageStruct
{
    uint16_t groupId;
    uint16_t clusterId;
    uint16_t srcAddress;
    uint8_t  srcEndpointId;
    uint8_t  dstEndpointId;
    uint8_t  broadcast;
    uint8_t  linkQuality;
    uint8_t  security;
    uint32_t timestamp;
    uint8_t  transactionId;
    uint8_t  length;
};

struct bindResponseStruct
{
    uint16_t shortAddress;
    uint8_t  status;
};

#pragma pack(pop)

class ZStack
{
    public:

        ZStack(ZStackCallback callback, uint8_t channel, uint16_t panId, int8_t bsl, int8_t rst, int8_t tx, int8_t rx, int8_t core = 0);

        void reset(void);
        void clear(void);
        void permitJoin(bool permit);
        void dataRequest(uint8_t id, uint16_t shortAddress, uint8_t endpointId, uint16_t clusterId, uint8_t *data, size_t length);
        void bindRequest(uint16_t shortAddress, uint64_t ieeeAddress, uint8_t endpointId, uint16_t clusterId);
        void parseInput(uint8_t *buffer, size_t length);

    private:

        ZStackCallback m_callback;
        int8_t m_bslPin, m_rstPin;

        bool m_clear, m_permitJoin;
        uint64_t m_ieeeAddress;
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
