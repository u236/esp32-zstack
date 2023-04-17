#include "ZStack.h"

ZStack::ZStack(ZStackCallback callback, uint8_t channel, uint16_t panId, int8_t bslPin, int8_t rstPin, int8_t rxPin, int8_t txPin, int8_t core) : m_callback(callback), m_bslPin(bslPin), m_rstPin(rstPin), m_clear(false), m_permitJoin(false), m_status(0x00)
{
    uint32_t channelList = static_cast <uint32_t> (1 << channel);

    m_nvData[0] = {ZCD_NV_MARKER,            0x01, {ZSTACK_CONFIGURATION_MARKER}};
    m_nvData[1] = {ZCD_NV_PRECFGKEY,         0x10, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}};
    m_nvData[2] = {ZCD_NV_PRECFGKEYS_ENABLE, 0x01, {0x01}};
    m_nvData[3] = {ZCD_NV_PANID,             0x02, {static_cast <uint8_t> (panId), static_cast <uint8_t> (panId >> 8)}};
    m_nvData[4] = {ZCD_NV_CHANLIST,          0x04, {static_cast <uint8_t> (channelList), static_cast <uint8_t> (channelList >> 8), static_cast <uint8_t> (channelList >> 16), static_cast <uint8_t> (channelList >> 24)}};
    m_nvData[5] = {ZCD_NV_LOGICAL_TYPE,      0x01, {0x00}};
    m_nvData[6] = {ZCD_NV_ZDO_DIRECT_CB,     0x01, {0x01}};
    m_nvData[7] = {0x0000};

    pinMode(m_bslPin, OUTPUT);
    pinMode(m_rstPin, OUTPUT);

    digitalWrite(m_bslPin, HIGH);

    ZSTACK_PORT.begin(115200, SERIAL_8N1, rxPin, txPin);
    ZSTACK_PORT.setTimeout(10);

    xTaskCreatePinnedToCore(inputTask, "ZStack Input", 4096, this, 0, NULL, core);
}

void ZStack::reset(void)
{
    digitalWrite(m_rstPin, LOW);
    delay(10);
    digitalWrite(m_rstPin, HIGH);
}

void ZStack::clear(void)
{
    nvWriteRequestStruct request;
    uint8_t buffer[sizeof(request) + 1];

    request.id = ZCD_NV_STARTUP_OPTION;
    request.offset = 0x00;
    request.length = 0x01;

    memcpy(buffer, &request, sizeof(request));
    buffer[sizeof(request)] = 0x03;

    m_clear = true;
    sendFrame(SYS_OSAL_NV_WRITE, buffer, sizeof(buffer));
}

void ZStack::permitJoin(bool enabled)
{
    permitJoinRequestStruct request;

    request.mode = 0x0F;
    request.dstAddress = 0xFFFC;
    request.duration = enabled ? 0xFF : 0x00;
    request.significance = 0x00;

    m_permitJoin = enabled;
    sendFrame(ZDO_MGMT_PERMIT_JOIN_REQ, reinterpret_cast <uint8_t*> (&request), sizeof(request));
}

void ZStack::dataRequest(uint8_t id, uint16_t shortAddress, uint8_t endpointId, uint16_t clusterId, uint8_t *data, size_t length)
{
    dataRequestStruct request;
    uint8_t buffer[sizeof(request) + length];

    request.shortAddress = shortAddress;
    request.dstEndpointId = endpointId;
    request.srcEndpointId = 0x01;
    request.clusterId = clusterId;
    request.transactionId = id;
    request.options = AF_DISCV_ROUTE;
    request.radius = AF_DEFAULT_RADIUS;
    request.length = static_cast <uint8_t> (length);

    memcpy(buffer, &request, sizeof(request));
    memcpy(buffer + sizeof(request), data, length);

    sendFrame(AF_DATA_REQUEST, reinterpret_cast <uint8_t*> (&buffer), sizeof(buffer));
}

void ZStack::parseInput(uint8_t *buffer, size_t length)
{
    size_t offset = 0;

    while (length >= offset + ZSTACK_MINIMAL_LENGTH)
    {
        uint8_t *data = buffer + offset, size = data[1], fcs = 0;
        uint16_t command = data[2] << 8 | data[3];

        if (data[0] != ZSTACK_FRAME_FLAG)
        {
            offset++;
            continue;
        }

        if (length + offset < size + 5)
            return;

        for (size_t i = 1; i < size + 4; i++)
            fcs ^= data[i];

        if (fcs != data[size + 4])
            return;

        parseFrame(command, data + 4, size);
        offset += size + 5;
    }
}

void ZStack::parseFrame(uint16_t command, uint8_t *data, size_t length)
{
    switch (command & 0x2000 ? command ^= 0x4000 : command)
    {
        case SYS_OSAL_NV_ITEM_INIT:
        {
            uint16_t id = ZCD_NV_MARKER;

            if (data[0] && data[0] != 0x09)
            {
                m_callback(ZStackEvent::configurationFailed, &id, sizeof(id));
                break;
            }

            writeNvItem();
            break;
        }

        case SYS_OSAL_NV_READ:
        {
            nvReadReplyStruct *reply = reinterpret_cast <nvReadReplyStruct*> (data);
            nvDataStruct *item = &m_nvData[m_nvIndex];

            if (reply->status || reply->length != item->length || memcmp(data + sizeof(nvReadReplyStruct), item->value, item->length))
            {
                m_callback(ZStackEvent::configurationMismatch, &item->id, sizeof(item->id));
                break;
            }

            m_nvIndex++;

            if (!m_nvData[m_nvIndex].id)
            {
                afRegisterRequestStruct request;
                uint8_t buffer[sizeof(request) + 2];

                m_callback(ZStackEvent::coordinatorStarting, &item->id, sizeof(item->id));

                request.endpointId = ZSTACK_ENDPOINT_ID;
                request.profileId = ZSTACK_ENDPOINT_PROFILE_ID;
                request.deviceId = ZSTACK_ENDPOINT_DEVICE_ID;
                request.version = 0x00;
                request.latency = 0x00;

                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, &request, sizeof(request));

                sendFrame(AF_REGISTER, buffer, sizeof(buffer));
                break;
            }

            readNvItem();
            break;
        }

        case SYS_OSAL_NV_WRITE:
        {
            nvDataStruct *item = &m_nvData[m_nvIndex];

            if (data[0])
            {
                m_callback(ZStackEvent::configurationFailed, &item->id, sizeof(item->id));;
                break;
            }

            m_nvIndex++;

            if (m_clear || !m_nvData[m_nvIndex].id)
            {
                if (!m_clear)
                    m_callback(ZStackEvent::configurationUpdated, NULL, 0);

                reset();
                break;
            }

            writeNvItem();
            break;
        }

        case AF_REGISTER:
        {
            uint16_t delay = 0;

            if (data[0])
            {
                m_callback(ZStackEvent::coordinatorFailed, NULL, 0);
                break;
            }

            sendFrame(ZDO_STARTUP_FROM_APP, reinterpret_cast <uint8_t*> (&delay), sizeof(delay));
            break;
        }

        case AF_DATA_REQUEST:
        {
            m_callback(data[0] ? ZStackEvent::requestFailed : ZStackEvent::requestEnqueued, NULL, 0);
            break;
        }

        case ZDO_MGMT_PERMIT_JOIN_REQ:
        {
            m_callback(data[0] ? ZStackEvent::permitJoinFailed : ZStackEvent::permitJoinChanged, &m_permitJoin, sizeof(m_permitJoin));
            break;
        }

        case ZDO_STARTUP_FROM_APP:
        {
            if (data[0] == 0x02)
                m_callback(ZStackEvent::coordinatorFailed, NULL, 0);

            break;
        }

        case UTIL_GET_DEVICE_INFO:
        {
            if (data[0])
            {
                m_callback(ZStackEvent::coordinatorFailed, NULL, 0);
                break;
            }

            memcpy(&m_ieeeAddress, data + 1, sizeof(m_ieeeAddress));
            readNvItem();
            break;
        }

        case SYS_RESET_IND:
        {
            m_callback(ZStackEvent::resetDetected, NULL, 0);
            m_nvIndex = 0;

            if (m_clear)
            {
                nvInitRequestStruct request;
                uint8_t buffer[sizeof(request) + 1];

                request.id = ZCD_NV_MARKER;
                request.itemLength = 0x01;
                request.dataLength = 0x01;

                memcpy(buffer, &request, sizeof(request));
                buffer[sizeof(request)] = ZSTACK_CONFIGURATION_MARKER;

                m_clear = false;
                sendFrame(SYS_OSAL_NV_ITEM_INIT, buffer, sizeof(buffer));
                break;
            }

            sendFrame(UTIL_GET_DEVICE_INFO, NULL, 0);
            break;
        }

        case AF_DATA_CONFIRM:
        {
            m_callback(ZStackEvent::requestFinished, data, length);
            break;
        }

        case AF_INCOMING_MSG:
        {
            m_callback(ZStackEvent::messageReceived, data, length);
            break;
        }

        case ZDO_STATE_CHANGE_IND:
        {
            m_status = data[0];
            m_callback(ZStackEvent::statusChanged, &m_status, sizeof(m_status));
            break;
        }

        case ZDO_END_DEVICE_ANNCE_IND:
        {
            m_callback(ZStackEvent::deviceJoinedNetwork, data + 2, length);
            break;
        }

        case ZDO_LEAVE_IND:
        {
            m_callback(ZStackEvent::deviceLeftNetwork, data, length);
            break;
        }

        case APP_CNF_BDB_COMMISSIONING_NOTIFICATION:
        {
            if (data[1] == 0x02 && m_status == 0x09)
                m_callback(data[2] ? ZStackEvent::coordinatorFailed : ZStackEvent::coordinatorReady, reinterpret_cast <uint8_t*> (&m_ieeeAddress), sizeof(m_ieeeAddress));

            break;
        }
    }
}

void ZStack::sendFrame(uint16_t command, uint8_t *data, size_t length)
{
    uint8_t buffer[ZSTACK_BUFFER_SIZE] = {ZSTACK_FRAME_FLAG, static_cast <uint8_t> (length), static_cast <uint8_t> (command >> 8), static_cast <uint8_t> (command)}, fcs = 0;

    memcpy(buffer + 4, data, length);

    for (size_t i = 1; i < length + 4; i++)
        fcs ^= buffer[i];

    buffer[length + 4] = fcs;
    ZSTACK_PORT.write(buffer, length + 5);
}

void ZStack::readNvItem(void)
{
    nvReadRequestStruct request;

    request.id = m_nvData[m_nvIndex].id;
    request.offset = 0x00;

    sendFrame(SYS_OSAL_NV_READ, reinterpret_cast <uint8_t*> (&request), sizeof(request));
}

void ZStack::writeNvItem(void)
{
    nvWriteRequestStruct request;
    nvDataStruct *item = &m_nvData[m_nvIndex];
    uint8_t buffer[sizeof(request) + item->length];

    request.id = item->id;
    request.offset = 0x00;
    request.length = item->length;

    memcpy(buffer, &request, sizeof(request));
    memcpy(buffer + sizeof(request), item->value, item->length);

    sendFrame(SYS_OSAL_NV_WRITE, reinterpret_cast <uint8_t*> (&buffer), sizeof(buffer));
}

void ZStack::inputTask(void *data)
{
    ZStack *zstack = reinterpret_cast <ZStack*> (data);
    uint8_t buffer[ZSTACK_BUFFER_SIZE];

    while (1)
    {
        if (ZSTACK_PORT.available())
        {
            size_t length = ZSTACK_PORT.read(buffer, sizeof(buffer));
            zstack->parseInput(buffer, length);
        }
    }
}
