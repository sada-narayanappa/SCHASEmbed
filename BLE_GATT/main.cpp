#include "mbed.h"
#include "ble/BLE.h"
#include "nrf51_rtc.h"

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
static uint8_t readValue[4] = {0};
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(readValue)> readChar(readCharUUID, readValue);

static uint8_t writeValue[10] = {0};
WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(writeValue)> writeChar(writeCharUUID, writeValue);

/* Set up custom service */
GattCharacteristic *characteristics[] = {&readChar, &writeChar};
GattService        customService(customServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));


void changeLED()
{
        led2 = !led2;
        wait_ms(250);   
}

/*
 *  Restart advertising when phone app disconnects
*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
   printf("Disconnect callback called\n\r");
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

void beginAdvertise()
{
    ble.gap().startAdvertising();
    led2 = 0;
    printf("Advertising\n\r");
    visible = false;
}

void stopAdvertise()
{
    ble.gap().stopAdvertising();
    led2 = 1;
    printf("Not advertising\n\r");
    visible = true;  
}

void incrememntInhalerCount()
{
    readValue[0] = (readValue[0] + 1u);
    BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(readChar.getValueHandle(), readValue, sizeof(readValue) / sizeof(readValue[0]));
    printf("value of readvalue: %i \n\r",readValue[0]);    
}  

void displayRtcTime()
{
    unsigned int curr_time = rtc.time();
    
    int s = curr_time%60;
    int m = (curr_time/60)%60;
    int h = (curr_time/3600)%24;
    int d = (curr_time/86400);
    
    readValue[3] = s + 6*((s/10)%10);
    readValue[2] = m + 6*((m/10)%10);
    readValue[1] = h + 6*((h/10)%10);
    readValue[0] = d + 6*((d/10)%10);
    
    // for(int i = 0; i < 4; i++) readValue[3-i] = (curr_time>>8*i)&255; // Use this if time isn't being displayed correctly otherwise.
        
    BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(readChar.getValueHandle(), readValue, sizeof(readValue) / sizeof(readValue[0]));
    printf("value of readvalue: %i \n\r",readValue[0]);    
}  

int buttonDuration()
{
    int counter = 0;
    while(button && counter<6000)
    {
        if((counter > 100) && (counter%1000 == 0)) led2 = !led2;
        else if((counter > 100) && ((counter-100)%1000 == 0)) led2 = !led2;
        counter++;
        wait_ms(1);
    }    
    
    if((counter%1000 <= 100) && (counter%1000 > 0)) led2 = !led2; // if button released while led is on, turn led off
    
    return counter;
}

void buttonPressed()
{
    int counterValue = buttonDuration();
    printf("Counter Value: %i \r\n", counterValue);
    
    if(counterValue >= 5000) rtc.set_time(0);
    else if(counterValue >= 3000)
    {
        if(visible && !connected)
            beginAdvertise();
            
        else if(!visible && !connected)
            stopAdvertise(); 
    }
    else displayRtcTime();
}

void update_time() { rtc.update_rtc(); }

/*
 *  Main loop
*/
int main(void)
{
    
    //buttonInputPin.mode(PullUp);
    /* initialize stuff */
    printf("\n\r********* Starting Main Loop *********\n\r");
    
    #define PERIODIC_UPDATE 480 // Update every 8 minutes.
    Ticker rtc_ticker;
    rtc_ticker.attach(&update_time, PERIODIC_UPDATE); // update the time regularly
    
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