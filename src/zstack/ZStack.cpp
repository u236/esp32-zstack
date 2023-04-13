#include "ZStack.h"

#define PRINT_FRAMES 0

ZStack::ZStack(ZStackCallback callback, uint8_t channel, uint16_t panId, int8_t bslPin, int8_t rstPin, int8_t rxPin, int8_t txPin, int8_t core) : m_callback(callback), m_channel(channel), m_panId(panId), m_bslPin(bslPin), m_rstPin(rstPin)
{
    pinMode(m_bslPin, OUTPUT);
    pinMode(m_rstPin, OUTPUT);

    digitalWrite(m_bslPin, HIGH);

    ZSTACK_PORT.begin(115200, SERIAL_8N1, rxPin, txPin);
    ZSTACK_PORT.setTimeout(10);

    m_nvItems[0] = ZCD_NV_MARKER;
    m_nvItems[1] = ZCD_NV_PRECFGKEY;
    m_nvItems[2] = ZCD_NV_PRECFGKEYS_ENABLE;
    m_nvItems[3] = ZCD_NV_PANID;
    m_nvItems[4] = ZCD_NV_CHANLIST;
    m_nvItems[5] = ZCD_NV_LOGICAL_TYPE;
    m_nvItems[6] = ZCD_NV_ZDO_DIRECT_CB;
    m_nvItems[7] = 0;

    xTaskCreatePinnedToCore(inputTask, "ZStack Input", 4096, this, 0, NULL, core);
}

void ZStack::reset(void)
{
    // TODO: add reset task with timeout

    digitalWrite(m_rstPin, LOW);
    delay(10);
    digitalWrite(m_rstPin, HIGH);
}

void ZStack::parseInput(uint8_t *buffer, size_t length)
{
    size_t offset = 0;

    if (PRINT_FRAMES) { //////////////////////
        Serial.printf("received serial data:");

        for (size_t i = 0; i < length; i++)
            Serial.printf(" %02x", buffer[i]);

        Serial.printf(" (%d)\n", length);
    } ////////////////////////////////////////

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
    if (PRINT_FRAMES) { //////////////////////
        Serial.printf("received frame 0x%04X with data:", command);

        for (size_t i = 0; i < length; i++)
            Serial.printf(" %02x", data[i]);

        Serial.printf("\n");
    } ////////////////////////////////////////

    if (command & 0x2000)
        command ^= 0x4000;

    switch (command)
    {
        case SYS_OSAL_NV_READ:
        {
            checkNvData(data, length);
            break;
        }

        case SYS_RESET_IND:
        {
            m_callback(ZStackEvent::resetDetected, NULL, 0);
            m_nvIndex = 0;
            readNvData();
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

    if (PRINT_FRAMES) { //////////////////////
        Serial.printf("send frame:");

        for (size_t i = 0; i < length + 5; i++)
            Serial.printf(" %02x", buffer[i]);

        Serial.printf("\n");
    } ////////////////////////////////////////

    ZSTACK_PORT.write(buffer, length + 5);
}

void ZStack::checkNvData(uint8_t *data, uint8_t length)
{
    { ////////////////////////////////////////
        Serial.printf("nv item 0x%04X:", m_nvItems[m_nvIndex]);

        for (size_t i = 0; i < length; i++)
            Serial.printf(" %02x", data[i]);

        Serial.printf("\n");
    } ////////////////////////////////////////

    m_nvIndex++;

    if (!m_nvItems[m_nvIndex])
    {
        Serial.println("start coordinator here");
        return;
    }

    readNvData();
}

void ZStack::readNvData(void)
{
    nvReadRequestStruct request;

    request.id = m_nvItems[m_nvIndex];
    request.offset = 0x00;

    sendFrame(SYS_OSAL_NV_READ, reinterpret_cast <uint8_t*> (&request), sizeof(request));
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
