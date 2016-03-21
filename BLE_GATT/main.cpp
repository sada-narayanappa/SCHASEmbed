#include "mbed.h"
#include "ble/BLE.h"

DigitalOut led(P0_19, 0);
DigitalOut led2(P0_4, 1);
InterruptIn button(P0_5);

bool visible = true;
bool connected = false;

Gap::GapState_t currentState;

BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);

//DigitalIn buttonInputPin(P0_10);

uint16_t customServiceUUID  = 0xA005;
uint16_t readCharUUID       = 0xA001;
uint16_t writeCharUUID      = 0xA002;

const static char     DEVICE_NAME[]        = "Inhaler Cap"; // change this
static const uint16_t uuid16_list[]        = {0xFFFF}; //Custom UUID, FFFF is reserved for development

/* Set Up custom Characteristics */
static uint8_t readValue[1] = {0};
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(readValue)> readChar(readCharUUID, readValue);

static uint8_t writeValue[10] = {0};
WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(writeValue)> writeChar(writeCharUUID, writeValue);

/* Set up custom service */
GattCharacteristic *characteristics[] = {&readChar, &writeChar};
GattService        customService(customServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));


void changeLED(){
        led2 = !led2;
        wait_ms(250);   
}

/*
 *  Restart advertising when phone app disconnects
*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
   visible = true;
   ble.gap().stopAdvertising();
   for(int i = 0; i<4; i++){
        led2 = !led2;
        wait_ms(250);
    }
    connected = false;
   // BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising();
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *){
    printf("Connected\n\r");    
    for(int i = 0; i<5; i++){
        led2 = !led2;
        wait_ms(250);
    }
    connected = true;
    visible = false;
}
/*
 *  Handle writes to writeCharacteristic
*/
void writeCharCallback(const GattWriteCallbackParams *params)
{
    /* Check to see what characteristic was written, by handle */   
    if(params->handle == writeChar.getValueHandle()) {
        /* toggle LED if only 1 byte is written */
        if(params->len == 1) {
            led2 = params->data[0];
            (params->data[0] == 0x00) ? printf("led on\n\r") : printf("led off\n\r"); // print led toggle
        }
        /* Print the data if more than 1 byte is written */
        else {
            printf("Data received: length = %d, data = 0x",params->len);
            for(int x=0; x < params->len; x++) {
                printf("%x", params->data[x]);
            }
            printf("\n\r");
        }
        /* Update the readChar with the value of writeChar */
        BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(readChar.getValueHandle(), params->data, params->len);
    }
}
/*
 * Initialization callback
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    if (error != BLE_ERROR_NONE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);
    ble.gap().onConnection(connectionCallback);
    ble.gattServer().onDataWritten(writeCharCallback);

    /* Setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE); // BLE only, no classic BT
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED); // advertising type
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME)); // add name
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list)); // UUID's broadcast in advertising packet
    ble.gap().setAdvertisingInterval(100); // 100ms.

    /* Add our custom service */
    ble.addService(customService);

    /* Start advertising */
//   ble.gap().startAdvertising();
}

    

void buttonPressed(){
    if(button){
        if(visible){
          /* Start advertising */
            ble.gap().startAdvertising();
            led2 = 0;
            printf("Advertising\n\r");
            visible = false;
        }
        else{
            if(connected){
                readValue[0] = (readValue[0] + 1u);
                BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(readChar.getValueHandle(), readValue, sizeof(readValue) / sizeof(readValue[0]));
                printf("value of readvalue: %i",readValue[0]);
            }
            else{
                ble.gap().stopAdvertising();
                led2 = 1;
                printf("Not advertising\n\r");
                visible = true;
            }
        }
     }
}



/*
 *  Main loop
*/
int main(void)
{
    
    //buttonInputPin.mode(PullUp);
    /* initialize stuff */
    printf("\n\r********* Starting Main Loop *********\n\r");
    
   // &ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);
    
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) {
        
    }

    /* Infinite loop waiting for BLE interrupt events */
    
    button.rise(&buttonPressed); 
    while(true){
        printf("test inside loop \n\r");
        ble.waitForEvent(); // Save power 
    }
}