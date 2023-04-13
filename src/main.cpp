#include <zstack/ZStack.h>

#define ZSTACK_CHANNEL  11
#define ZSTACK_PANID    0x1234  // WARNING: use unique panId for each zigbee network in the same area!

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
            Serial.println("ZStack reset detected...");
            break;

        case ZStackEvent::coordinatorReady:
            Serial.println("ZStack coordinator ready!");
            break;
    }
}

void setup(void)
{
    Serial.begin(9600);
    pinMode(2, OUTPUT);

    delay(2000);
    Serial.println("Hello there!");

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
