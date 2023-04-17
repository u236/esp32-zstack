#include <zstack/ZStack.h>

#define PRINT_DUMPS                         true
#define BLINK_PIN                           2

#define ZSTACK_CHANNEL                      11
#define ZSTACK_PANID                        0x1234 // WARNING: use unique panId for each zigbee network in the same area!

#define ZSTACK_BSL_PIN                      14
#define ZSTACK_RST_PIN                      4
#define ZSTACK_RX_PIN                       18
#define ZSTACK_TX_PIN                       19

// few ZCL definitions here
#define FC_MANUFACTURER_SPECIFIC            0x04
#define FC_CLUSTER_SPECIFIC                 0x01
#define CMD_REPORT_ATTRIBUTES               0x0A
#define CMD_CONFIGURE_REPORTING             0x06
#define CMD_CONFIGURE_REPORTING_RESPONSE    0x07
#define CMD_DEFAULT_RESPONSE                0x0B

#define DATA_TYPE_8BIT_UNSIGNED             0x20
#define DATA_TYPE_16BIT_UNSIGNED            0x21
#define DATA_TYPE_24BIT_UNSIGNED            0x22
#define DATA_TYPE_32BIT_UNSIGNED            0x23
#define DATA_TYPE_40BIT_UNSIGNED            0x24
#define DATA_TYPE_48BIT_UNSIGNED            0x25
#define DATA_TYPE_56BIT_UNSIGNED            0x26
#define DATA_TYPE_64BIT_UNSIGNED            0x27
#define DATA_TYPE_8BIT_SIGNED               0x28
#define DATA_TYPE_16BIT_SIGNED              0x29
#define DATA_TYPE_24BIT_SIGNED              0x2A
#define DATA_TYPE_32BIT_SIGNED              0x2B
#define DATA_TYPE_40BIT_SIGNED              0x2C
#define DATA_TYPE_48BIT_SIGNED              0x2D
#define DATA_TYPE_56BIT_SIGNED              0x2E
#define DATA_TYPE_64BIT_SIGNED              0x2F
#define DATA_TYPE_SINGLE_PRECISION          0x39
#define DATA_TYPE_DOUBLE_PRECISION          0x3A

#pragma pack(push, 1)

struct zclHeader // simplified header without "manufacturer core" field
{
    uint8_t  frameControl;
    uint8_t  transationId;
    uint8_t  commandId;
};

struct configureReportingStruct
{
    uint8_t  direction;
    uint16_t attributeId;
    uint8_t  dataType;
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t valueChange;
};

#pragma pack(pop)
// end of ZCL definitions

static ZStack *zstack;
static uint8_t transactionId = 0;

// look Zigbee Cluster Library Specification for all data types
uint8_t zclDataSize(uint8_t dataType)
{
    switch (dataType)
    {
        case DATA_TYPE_8BIT_UNSIGNED:
        case DATA_TYPE_8BIT_SIGNED:
            return 1;

        case DATA_TYPE_16BIT_UNSIGNED:
        case DATA_TYPE_16BIT_SIGNED:
            return 2;

        case DATA_TYPE_24BIT_UNSIGNED:
        case DATA_TYPE_24BIT_SIGNED:
            return 3;

        case DATA_TYPE_32BIT_UNSIGNED:
        case DATA_TYPE_32BIT_SIGNED:
        case DATA_TYPE_SINGLE_PRECISION:
            return 4;

        case DATA_TYPE_40BIT_UNSIGNED:
        case DATA_TYPE_40BIT_SIGNED:
            return 5;

        case DATA_TYPE_48BIT_UNSIGNED:
        case DATA_TYPE_48BIT_SIGNED:
            return 6;

        case DATA_TYPE_56BIT_UNSIGNED:
        case DATA_TYPE_56BIT_SIGNED:
            return 7;

        case DATA_TYPE_64BIT_UNSIGNED:
        case DATA_TYPE_64BIT_SIGNED:
        case DATA_TYPE_DOUBLE_PRECISION:
            return 8;
    }

    return 0;
}

static void parseAttribute(uint8_t endpointId, uint16_t clusterId, uint16_t attributeId, uint8_t *data, size_t length)
{
    (void) endpointId;
    (void) length;

    switch (clusterId)
    {
        case 0x0001: // power configuration cluster

            if (attributeId == 0x0021)
                Serial.printf("Battery: %.1f\n", *(reinterpret_cast <uint8_t*> (data)) / 2.0);

            break;

        case 0x0402: // temperature measurement cluster

            if (attributeId == 0x0000)
                Serial.printf("Temperature: %.1f\n", *(reinterpret_cast <int16_t*> (data)) / 100.0);

            break;

        case 0x0405: // relative humidity measurement cluster

            if (attributeId == 0x0000)
                Serial.printf("Relative humidity: %.1f\n", *(reinterpret_cast <uint16_t*> (data)) / 100.0);

            break;

        case 0x0408: // soil moisture measurement cluster

            if (attributeId == 0x0000)
                Serial.printf("Soil moisture: %.1f\n", *(reinterpret_cast <uint16_t*> (data)) / 100.0);

            break;
    }
}

 static void parseAttributesReport(uint8_t endpointId, uint16_t clusterId, uint8_t *data, size_t length)
 {
    size_t offset = 0;

    while (length >= offset + 3) // attribute id field + data type field = 3 bytes
    {
        uint16_t attributeId = *(reinterpret_cast <uint16_t*> (data + offset));
        uint8_t size = zclDataSize(data[offset + 2]), *payload = data + offset + 3;

        if (PRINT_DUMPS)
        {
            Serial.printf("Attribute 0x%04x (data type 0x%02x) data:", attributeId, data[offset + 2]);

            for (size_t i = 0; i < size; i++)
                Serial.printf(" %02x", payload[i]);

            Serial.printf("\n");
        }

        parseAttribute(endpointId, clusterId, attributeId, payload, size);
        offset += size + 3;
    }
 }

// there we receive ZCL message, look Zigbee Cluster Library Specification for more info
static void zclMessage(uint8_t endpointId, uint16_t clusterId, uint8_t *data, size_t length)
{
    uint8_t frameControl = data[0], commandId, *payload;
    size_t size;

    if (PRINT_DUMPS)
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
        size = length - 5;
    }
    else
    {
        commandId = data[2];
        payload = data + 3;
        size = length - 3;
    }

    Serial.printf("Endpoint ID: 0x%02x\n", endpointId);
    Serial.printf("Cluster ID:  0x%04x\n", clusterId);
    Serial.printf("Command ID:  0x%02x\n", commandId);

    if (frameControl & FC_CLUSTER_SPECIFIC)
    {
        Serial.printf("Cluster specific command 0x%02x received...\n", commandId);
        return;
    }

    switch(commandId)
    {
        case CMD_REPORT_ATTRIBUTES:
            parseAttributesReport(endpointId, clusterId, payload, size);
            break;

        case CMD_CONFIGURE_REPORTING_RESPONSE:
            Serial.printf("Configure reporting response received, status: 0x%02x\n", payload[0]);
            break;

        case CMD_DEFAULT_RESPONSE:
            Serial.printf("Default response received, commandId: 0x%02x, status: 0x%02x\n", payload[0], payload[1]);
            break;

        default:
            Serial.printf("Global coommand 0x%02x not supported here...\n", commandId);
            break;
    }
}

// configure battery percentage reporting request example, look Zigbee Cluster Library Specification for more info
static void configureBatteryPercentageReporting(uint16_t shortAddress, uint16_t endpointId)
{
    zclHeader header;
    configureReportingStruct request;
    uint8_t buffer[sizeof(header) + sizeof(request)];

    header.frameControl = 0x00;
    header.transationId = transactionId;
    header.commandId = CMD_CONFIGURE_REPORTING;

    request.direction = 0x00;       // server to client
    request.attributeId = 0x0021;   // battery percentage value attribure
    request.dataType = 0x20;        // 8 bit unsigned integer
    request.minInterval = 0;        // 0 seconds
    request.maxInterval = 3600;     // 1 hour
    request.valueChange = 0;        // any change

    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &request, sizeof(request));

    // request to 0x0001 (power configuration) cluster
    zstack->dataRequest(transactionId++, shortAddress, endpointId, 0x0001, buffer, sizeof(buffer));
}

// configure soil moisture reporting request example, look Zigbee Cluster Library Specification for more info
static void configureSoilMoistureReporting(uint16_t shortAddress, uint16_t endpointId)
{
    zclHeader header;
    configureReportingStruct request;
    uint8_t buffer[sizeof(header) + sizeof(request)];

    header.frameControl = 0x00;
    header.transationId = transactionId;
    header.commandId = CMD_CONFIGURE_REPORTING;

    request.direction = 0x00;       // server to client
    request.attributeId = 0x0000;   // soil mousture cluser measured value attribure
    request.dataType = 0x21;        // 16 bit unsigned integer
    request.minInterval = 0;        // 10 seconds
    request.maxInterval = 30;       // 30 seconds
    request.valueChange = 0;        // any change

    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), &request, sizeof(request));

    // request to 0x0408 (soil moisture) cluster
    zstack->dataRequest(transactionId++, shortAddress, endpointId, 0x0408, buffer, sizeof(buffer));
}

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
            Serial.printf("ZStack coordinator ready, address: 0x%016llx\n", *(reinterpret_cast <uint64_t*> (data)));
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
            Serial.printf("ZStack device 0x%016llx joined network with short address 0x%04x!\n", announce->ieeeAddress, announce->shortAddress);

            // bind and configure power configuration cluster reporting
            zstack->bindRequest(announce->shortAddress, announce->ieeeAddress, 0x01, 0x0001);
            configureBatteryPercentageReporting(announce->shortAddress, 0x01);
            //

            // bind and configure soil moisture cluster reporting
            zstack->bindRequest(announce->shortAddress, announce->ieeeAddress, 0x01, 0x0408);
            configureSoilMoistureReporting(announce->shortAddress, 0x01);
            //

            break;
        }

        case ZStackEvent::deviceLeftNetwork:
        {
            deviceLeaveStruct *leave = reinterpret_cast <deviceLeaveStruct*> (data);
            Serial.printf("ZStack device 0x%016llx left network...\n", leave->ieeeAddress);
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

        case ZStackEvent::bindEnqueued:
            Serial.printf("ZStack bind request was equeued...\n");
            break;

        case ZStackEvent::bindFailed:
            Serial.printf("ZStack bind request failed :(\n");
            break;

        case ZStackEvent::bindFinished:
        {
            bindResponseStruct *response = reinterpret_cast <bindResponseStruct*> (data);
            Serial.printf("ZStack bind request for 0x%04x finished %s!\n", response->shortAddress, response->status ? "with error" : "successfully");
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
