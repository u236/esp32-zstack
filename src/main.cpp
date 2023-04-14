#include <zstack/ZStack.h>

#define ZSTACK_CHANNEL  11
#define ZSTACK_PANID    0x1234 // WARNING: use unique panId for each zigbee network in the same area!

#define ZSTACK_BSL_PIN  14
#define ZSTACK_RST_PIN  4
#define ZSTACK_RX_PIN   18
#define ZSTACK_TX_PIN   19

static ZStack *zstack;

void zstackCallback(ZStackEvent event, void *data, size_t length)
{
    (void) data;

    switch (event)
    {
        case ZStackEvent::resetDetected:
            Serial.printf("ZStack reset detected...\n");
            break;

        case ZStackEvent::configurationMismatch:
            Serial.printf("ZStack NV item 0x%04X value mismatch, updating configuration...\n", *(reinterpret_cast <uint16_t*> (data)));
            zstack->clear(); // or do something else?
            break;

        case ZStackEvent::configurationUpdated:
            Serial.printf("ZStack configuration updated...\n");
            break;

        case ZStackEvent::configurationFailed:
            Serial.printf("ZStack NV item 0x%04X configuration failed :(\n", *(reinterpret_cast <uint16_t*> (data)));
            zstack->reset(); // or do something else?
            break;

        case ZStackEvent::statusChanged:
            Serial.printf("ZStack state changed, new state is 0x%02X\n", *(reinterpret_cast <uint8_t*> (data)));
            break;

        case ZStackEvent::coordinatorStarting:
            Serial.printf("ZStack coordinator starting...\n");
            break;

        case ZStackEvent::coordinatorReady:
            Serial.printf("ZStack coordinator ready!\n");
            break;

        case ZStackEvent::coordinatorFailed:
            Serial.printf("ZStack coordinator startup failed :(\n");
            break;
    }
}

void setup(void)
{
    pinMode(2, OUTPUT);
    Serial.begin(9600);

    zstack = new ZStack(zstackCallback, ZSTACK_CHANNEL, ZSTACK_PANID, ZSTACK_BSL_PIN, ZSTACK_RST_PIN, ZSTACK_RX_PIN, ZSTACK_TX_PIN);
    zstack->reset();
}

void loop(void)
{
    digitalWrite (2, HIGH);
    delay (500);
    digitalWrite (2, LOW);
    delay (500);
}
