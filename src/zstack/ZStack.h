#ifndef ZSTACK_H
#define ZSTACK_H

#define ZSTACK_PORT                                 Serial2
#define ZSTACK_FRAME_FLAG                           0xFE
#define ZSTACK_BUFFER_SIZE                          256
#define ZSTACK_MINIMAL_LENGTH                       5
#define ZSTACK_REQUEST_TIMEOUT                      10000

#define SYS_OSAL_NV_READ                            0x2108
#define SYS_RESET_IND                               0x4180

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
    coordinatorReady
};

typedef void (*ZStackCallback) (ZStackEvent event, void *data, size_t length);

#pragma pack(push, 1)

struct nvReadRequestStruct
{
    uint16_t id;
    uint8_t  offset;
};

struct nvReadReplyStruct
{
    uint8_t  status;
    uint8_t  length;
};

#pragma pack(pop)

class ZStack
{
    public:

        ZStack(ZStackCallback callback, uint8_t channel, uint16_t panId, int8_t bsl, int8_t rst, int8_t tx, int8_t rx, int8_t core = 0);

        void reset(void);
        void parseInput(uint8_t *buffer, size_t length);

    private:

        ZStackCallback m_callback;

        uint8_t m_channel;
        uint16_t m_panId;

        int8_t m_bslPin;
        int8_t m_rstPin;

        uint16_t m_nvItems[8];
        uint8_t m_nvIndex;

        void parseFrame(uint16_t command, uint8_t *data, size_t length);
        void sendFrame(uint16_t command, uint8_t *data, size_t length);

        void checkNvData(uint8_t *data, uint8_t length);
        void readNvData(void);

        static void inputTask(void *data);

};

#endif
