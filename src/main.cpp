#include <zstack/ZStack.h>

#define PRINT_ZCL_DUMP              true
#define BLINK_PIN                   2

#define ZSTACK_CHANNEL              11
#define ZSTACK_PANID                0x1234 // WARNING: use unique panId for each zigbee network in the same area!

#define ZSTACK_BSL_PIN              14
#define ZSTACK_RST_PIN              4
#define ZSTACK_RX_PIN               18
#define ZSTACK_TX_PIN               19

// few ZCL definitions here
#define FC_MANUFACTURER_SPECIFIC    0x04
#define FC_CLUSTER_SPECIFIC         0x01
#define CMD_REPORT_ATTRIBUTES       0x0A
//

static ZStack *zstack;

// there we receive ZCL message, look Zigbee Cluster Library Specification for more info
static void zclMessage(uint8_t endpointId, uint16_t clusterId, uint8_t *data, size_t length)
{
    uint8_t frameControl = data[0], commandId, *payload;

    if (PRINT_ZCL_DUMP)
    {
        Serial.printf("Мessage raw data:");

        for (size_t i = 0; i < length; i++)
            Serial.printf(" %02x", data[i]);

        Serial.printf("\n");
    }

    if (frameControl & FC_MANUFACTURER_SPECIFIC)
    {
        commandId = data[4];
        payload = data + 5;
    }
    else
    {
        commandId = data[2];
        payload = data + 3;
    }

    Serial.printf("Endpoint ID: 0x%02x\n", endpointId);
    Serial.printf("Cluster ID:  0x%04x\n", clusterId);
    Serial.printf("Command ID:  0x%02x\n", commandId);

    if (frameControl & FC_CLUSTER_SPECIFIC)
    {
        Serial.printf("Cluster specific command 0x%02x received...\n", commandId);
        return;
    }

    if (commandId == CMD_REPORT_ATTRIBUTES)
    {
        uint16_t attributeId = payload[0] | payload[1] << 8;

        switch (clusterId)
        {
            case 0x0402: // temperature measurement cluster

                if (attributeId == 0x0000)
                    Serial.printf("Temperature: %.1f\n", *(reinterpret_cast <int16_t*> (payload + 3)) / 100.0);

                break;

            case 0x0405: // relative humidity measurement cluster

                if (attributeId == 0x0000)
                    Serial.printf("Relative humidity: %.1f\n", *(reinterpret_cast <uint16_t*> (payload + 3)) / 100.0);

                break;

            case 0x0408: // soil moisture measurement cluster

                if (attributeId == 0x0000)
                    Serial.printf("Soil moisture: %.1f\n", *(reinterpret_cast <uint16_t*> (payload + 3)) / 100.0);

                break;
        }

        return;
    }

    Serial.printf("Global coommand 0x%02x not supported here...\n", commandId);
}
//

static void zstackCallback(ZStackEvent event, void *data, size_t length)
{
    (void) data;

    switch (event)
    {
        case ZStackEvent::resetDetected:
            Serial.printf("ZStack reset detected...\n");
            break;

        case ZStackEvent::configurationMismatch:
            Serial.printf("ZStack NV item 0x%04x value mismatch, updating configuration...\n", *(reinterpret_cast <uint16_t*> (data)));
            zstack->clear(); // or do something else?
            break;

        case ZStackEvent::configurationUpdated:
            Serial.printf("ZStack configuration updated...\n");
            break;

        case ZStackEvent::configurationFailed:
            Serial.printf("ZStack NV item 0x%04x configuration failed :(\n", *(reinterpret_cast <uint16_t*> (data)));
            zstack->reset(); // or do something else?
            break;

        case ZStackEvent::statusChanged:
            Serial.printf("ZStack state changed, new state is 0x%02x\n", *(reinterpret_cast <uint8_t*> (data)));
            break;

        case ZStackEvent::coordinatorStarting:
            Serial.printf("ZStack coordinator starting...\n");
            break;

        case ZStackEvent::coordinatorReady:
            Serial.printf("ZStack coordinator ready!\n");
            zstack->permitJoin(true); // move it somewhere
            break;

        case ZStackEvent::coordinatorFailed:
            Serial.printf("ZStack coordinator startup failed :(\n");
            break;

        case ZStackEvent::permitJoinChanged:
            Serial.printf("ZStack permit join is now %s...\n", *(reinterpret_cast <bool*> (data)) ? "enabled" : "disabled");
            break;

        case ZStackEvent::permitJoinFailed:
            Serial.printf("ZStack permit join request failed :(\n");
            break;

        case ZStackEvent::deviceJoinedNetwork:
        {
            deviceAnnounceStruct *announce = reinterpret_cast <deviceAnnounceStruct*> (data);
            Serial.printf("ZStack device 0x%16llx joined network with short address 0x%04x!\n", announce->ieeeAddress, announce->shortAddress);
            break;
        }

        case ZStackEvent::deviceLeftNetwork:
        {
            deviceLeaveStruct *leave = reinterpret_cast <deviceLeaveStruct*> (data);
            Serial.printf("ZStack device 0x%16llx left network...\n", leave->ieeeAddress);
            break;
        }

        case ZStackEvent::requestEnqueued:
            Serial.printf("ZStack data request was equeued...\n");
            break;

        case ZStackEvent::requestFailed:
            Serial.printf("ZStack data request failed :(\n");
            break;

        case ZStackEvent::requestFinished:
        {
            dataConfirmStruct *confirm = reinterpret_cast <dataConfirmStruct*> (data);
            Serial.printf("ZStack data request %d finished %s!\n", confirm->transactionId, confirm->status ? "with error" : "successfully");
            break;
        }

        case ZStackEvent::messageReceived:
        {
            incomingMessageStruct *message = reinterpret_cast <incomingMessageStruct*> (data);
            Serial.printf("ZStack message received from 0x%04x with link quality = %d\n", message->srcAddress, message->linkQuality);
            zclMessage(message->srcEndpointId, message->clusterId, reinterpret_cast <uint8_t*> (data) + sizeof(incomingMessageStruct), message->length);
            break;
        }
    }
}

void setup(void)
{
    pinMode(BLINK_PIN, OUTPUT);
    Serial.begin(9600);

    zstack = new ZStack(zstackCallback, ZSTACK_CHANNEL, ZSTACK_PANID, ZSTACK_BSL_PIN, ZSTACK_RST_PIN, ZSTACK_RX_PIN, ZSTACK_TX_PIN);
    zstack->reset();
}

void loop(void)
{
    digitalWrite (BLINK_PIN, HIGH);
    delay (500);
    digitalWrite (BLINK_PIN, LOW);
    delay (500);
}
