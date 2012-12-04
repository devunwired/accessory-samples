/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define ADK_INTERNAL
extern "C"{
    #include "fwk.h"
    #include "LEDs.h"
    #include "Audio.h"
    #include "dbg.h"
    #include "coop.h"
    #include "BT.h"
    #include "HCI.h"
    #include "btL2CAP.h"
    #include "btRFCOMM.h"
    #include "btSDP.h"
    #include "btA2DP.h"
    #include "I2C.h"
    #include "hygro.h"
    #include "baro.h"
    #include "capsense.h"
    #include "als.h"
    #include "accel.h"
    #include "hygro.h"
    #include "capsense.h"
    #include "als.h"
    #include "accel.h"
    #include "mag.h"
    #include "f_ff.h"
    #include "simpleOgg.h"
    #include "RTC.h"
    #include "sgBuf.h"
    void eliza(void);
}
#include "usbh.h"
#include "ADK.h"

static adkBtConnectionRequestF bt_crF = 0;
static adkBtLinkKeyRequestF bt_krF = 0;
static adkBtLinkKeyCreatedF bt_kcF = 0;
static adkBtPinRequestF bt_prF = 0;
static adkBtDiscoveryResultF bt_drF = 0;
static adkBtSspDisplayF bt_pdF = 0;
static FATFS fs;

static char btLinkKeyRequest(void* userData, const uint8_t* mac, uint8_t* buf){

    if(bt_krF) return bt_krF(mac, buf);

    dbgPrintf("BT Link key request from %02x:%02x:%02x:%02x:%02x:%02x -> denied due to lack of handler", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    return 0;
}
static uint8_t btPinRequestF(void* userData, const uint8_t* mac, uint8_t* buf){	//fill buff with PIN code, return num bytes used (16 max) return 0 to decline

    if(bt_prF) return bt_prF(mac, buf);

    dbgPrintf("BT PIN request from %02x:%02x:%02x:%02x:%02x:%02x -> '0000' due to lack of handler", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

    buf[0] = buf[1] = buf[2] = buf[3] = '0';
    return 4;
}

static void btLinkKeyCreated(void* userData, const uint8_t* mac, const uint8_t* buf){

    if(bt_kcF) bt_kcF(mac, buf);
}

static char btConnReqF(void* userData, const uint8_t* mac, uint32_t devClass, uint8_t linkType){	//return 1 to accept

    if(bt_crF) return bt_crF(mac, devClass, linkType);

    dbgPrintf("BT connection request: %s connection from %02x:%02x:%02x:%02x:%02x:%02x (class %06X) -> accepted due to lack of handler\n", linkType ? "ACL" : "SCO", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], devClass);
    return 1;
}

static void btConnStartF(void* userData, uint16_t conn, const uint8_t* mac, uint8_t linkType, uint8_t encrMode){

    dbgPrintf("BT %s connection up with handle %d to %02x:%02x:%02x:%02x:%02x:%02x encryption type %d\n",
    linkType ? "ACL" : "SCO", conn, mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], encrMode);
    l2capAclLinkUp(conn);
}

static void btConnEndF(void* userData, uint16_t conn, uint8_t reason){

    dbgPrintf("BT connection with handle %d down for reason %d\n", conn, reason);
    l2capAclLinkDown(conn);
}

static void btAclDataRxF(void* userData, uint16_t conn, char first, uint8_t bcastType, const uint8_t* data, uint16_t sz){

    l2capAclLinkDataRx(conn, first, data, sz);
}

static void btSspShowF(void* userData, const uint8_t* mac, uint32_t val){

    if(val == BT_SSP_DONE_VAL) val = ADK_BT_SSP_DONE_VAL;

    if(bt_pdF) bt_pdF(mac, val);
}

static char btVerboseScanCbkF(void* userData, BtDiscoveryResult* dr){

    if(bt_drF) return bt_drF(dr->mac, dr->PSRM, dr->PSPM, dr->PSM, dr->co, dr->dc);

    dbgPrintf("BT: no callback for scan makes the scan useless, no?");
    return 0;
}

void ADK::adkInit(void){

    //board init
    ::fwkInit();
    ::coopInit();
    ::ledsInit();
    ::i2cInit(1, 400000);
    ::rtcInit();
    ::rtcSet(2012, 3, 28, 19, 8, 50);
    ::audioInit();
    ::usbh_init();

    //bt init
    static const BtFuncs myBtFuncs = {this, btVerboseScanCbkF, btConnReqF, btConnStartF, btConnEndF, btPinRequestF, btLinkKeyRequest, btLinkKeyCreated, btAclDataRxF, btSspShowF};
    btInit(&myBtFuncs);              //BT UART & HCI driver
    btSdpRegisterL2capService();     //SDP daemon
    btRfcommRegisterL2capService();  //RFCOMM framework
    eliza();                         //easter egg
    btA2dpRegister();                //A2DP profile


    uint8_t mac[BT_MAC_SIZE];

    if(::btLocalMac(mac)) dbgPrintf("BT MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

    //i2c devices init
//    if(!hygroInit()) dbgPrintf("ADK i2c init failure: hygrometer\n");
//    if(!baroInit()) dbgPrintf("ADK i2c init failure: barometer\n");
//    if(!capSenseInit()) dbgPrintf("ADK i2c init failure: capsense\n");
//    if(!alsInit()) dbgPrintf("ADK i2c init failure: ALS\n");
//    if(!accelInit()) dbgPrintf("ADK i2c init failure: accelerometer\n");
//    if(!magInit())  dbgPrintf("ADK i2c init failure: magnetometer\n");
}

void ADK::adkEventProcess(void){
    ::coopYield();
}

void ADK::getUniqueId(uint32_t* ID){

    ::cpuGetUniqId(ID);
}

uint64_t ADK::getUptime(void){	//in ms

    return ::fwkGetUptime();
}

void ADK::adkSetPutchar(adkPutcharF f){

    ::dbgSetPutchar(f);
}

void ADK::ledWrite(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b){

    ::ledWrite(led_id, r, g, b);
}

void ADK::ledDrawIcon(uint8_t offset, uint8_t r, uint8_t g, uint8_t b){

    ::ledDrawIcon(offset, r, g, b);
}

void ADK::ledDrawLetter(char letter, uint8_t val, uint8_t r, uint8_t g, uint8_t b){

    ::ledDrawLetter(letter, val, r, g, b);
}

void ADK::ledUpdate(void){

    ::ledUpdate();
}

void ADK::ledDbgState(char on){

    ::ledDbgState(on);
}

void ADK::audioOn(int source, uint32_t samplerate){

    ::audioOn(source, samplerate);
}

void ADK::audioOff(int source){

    ::audioOff(source);
}

void ADK::audioAddBuffer(int source, const uint16_t* samples, uint32_t numSamples){

    ::audioAddBuffer(source, samples, numSamples);
}

int ADK::audioTryAddBuffer(int source, const uint16_t* samples, uint32_t numSamples){

    return ::audioTryAddBuffer(source, samples, numSamples);
}

void ADK::btEnable(adkBtConnectionRequestF crF, adkBtLinkKeyRequestF krF, adkBtLinkKeyCreatedF kcF, adkBtPinRequestF prF, adkBtDiscoveryResultF drF){

    bt_crF = crF;
    bt_krF = krF;
    bt_kcF = kcF;
    bt_prF = prF;
    bt_drF = drF;
}

void ADK::btSetSspCallback(adkBtSspDisplayF pdF){

    bt_pdF = pdF;
}

char ADK::btSetLocalName(const char* name){

    return ::btSetLocalName(name);
}

char ADK::btGetRemoteName(const uint8_t* mac, uint8_t PSRM, uint8_t PSM, uint16_t co, char* nameBuf){

    return ::btGetRemoteName(mac, PSRM, PSM, co, nameBuf);
}

void ADK::btScan(void){

    ::btScan();
}

char ADK::btDiscoverable(char on){

    return ::btDiscoverable(on);
}

char ADK::btConnectable(char on){

    return ::btConnectable(on);
}

char ADK::btSetDeviceClass(uint32_t cls){

    return ::btSetDeviceClass(cls);
}

char ADK::hygroRead(int32_t *temp, int32_t *humidity){

    return ::hygroRead(temp, humidity);
}

void ADK::baroRead(uint8_t oss, long *kPa, long* decicelcius){

    return ::baroRead(oss, kPa, decicelcius);
}

uint8_t ADK::capSenseSlider(void){

    return ::capSenseSlider();
}

uint16_t ADK::capSenseButtons(void){

    return ::capSenseButtons();
}

uint16_t ADK::capSenseIcons(void){

    return ::capSenseIcons();
}

void ADK::capSenseDump(void){

    return ::capSenseDump();
}

void ADK::alsRead(uint16_t* prox, uint16_t* clear, uint16_t* R, uint16_t* G, uint16_t* B, uint16_t* IR, uint16_t* temp){

    return ::alsRead(prox, clear, R, G, B, IR, temp);
}

void ADK::accelRead(int16_t* x, int16_t* y, int16_t* z){

    ::accelRead(x, y, z);
}

void ADK::magRead(int16_t* x, int16_t* y, int16_t* z){

    ::magRead(x, y, z);
}

void ADK::playOgg(const char* path){

    ::playOgg(path);
}

void ADK::playOggBackground(const char* path, char *complete, char *abort) {

    ::playOggBackground(path, complete, abort);
}

void ADK::setVolume(uint8_t vol){

    ::setVolume(vol);
}

uint8_t ADK::getVolume(void){

    return ::getVolume();
}

void ADK::rtcGet(uint16_t* yearP, uint8_t* monthP, uint8_t* dayP, uint8_t* hourP, uint8_t* minuteP, uint8_t* secondP){

    ::rtcGet(yearP, monthP, dayP, hourP, minuteP, secondP);
}

void ADK::rtcSet(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second){

    ::rtcSet(year, month, day, hour, minute, second);
}


char ADK::fatfsMount(void){								// Mount/Unmount a logical drive

    return f_mount (0, &fs);
}

char ADK::fatfsOpen(struct FatFile** fPP, const char* path, uint8_t mode){		// Open or create a file

    FIL f;
    FIL* fP;
    char ret;
    uint8_t flg = 0;

    if(mode & FATFS_READ) flg |= FA_READ;
    if(mode & FATFS_WRITE) flg |= FA_WRITE;
    if(mode & FATFS_CREATE) flg |= FA_CREATE_NEW | FA_OPEN_ALWAYS;
    if(mode & FATFS_TRUNCATE) flg |= FA_CREATE_ALWAYS | FA_OPEN_ALWAYS;

    ret = f_open(&f, path, flg);
    if(ret != FR_OK) return ret;

    fP = (FIL*)malloc(sizeof(FIL));
    if(!fP){
        f_close(&f);
        return -1;
    }
    *fP = f;
    *fPP = (struct FatFile*)fP;
    return FR_OK;
}

char ADK::fatfsRead(struct FatFile* f, void* buf, uint32_t num, uint32_t* numDone){	// Read data from a file

    return f_read((FIL*)f, buf, num, numDone);
}

char ADK::fatfsWrite(struct FatFile* f, void* buf, uint32_t num, uint32_t* numDone){		// Write data to a file

    return f_write((FIL*)f, buf, num, numDone);
}

char ADK::fatfsSeek(struct FatFile* f_, uint8_t whence, int32_t ofst){			// Move file pointer of a file object

    FIL* f = (FIL*)f_;
    uint32_t pos;

    pos = f->fptr;
    switch(pos){
        case FATFS_START: pos = ofst; break;
        case FATFS_CUR: pos += ofst; break;
        case FATFS_END: pos = f->fsize + ofst; break;
        default: return -1;
    }

    return f_lseek(f, pos);
}

char ADK::fatfsClose(struct FatFile* f){						// Close an open file object

    char ret;

    ret = f_close((FIL*)f);
    free(f);
    return ret;
}

char ADK::fatfsTruncate(struct FatFile* f){						// Truncate file

    return f_truncate((FIL*)f);
}

char ADK::fatfsSync(struct FatFile* f){							// Flush cached data of a writing file

    return f_sync((FIL*)f);
}

char ADK::fatfsOpenDir(struct FatDir** dPP, const char* path){				// Open an existing directory

    DIR d;
    DIR* dP;
    char ret;

    ret = f_opendir(&d, path);
    if(ret != FR_OK) return ret;

    dP = (DIR*)malloc(sizeof(DIR));
    if(!dP) return -1;

    *dP = d;
    *dPP = (struct FatDir*)dP;
    return FR_OK;
}

static void fatfsfileInfoToFileInfo(FILINFO* fat, FatFileInfo* fatfs, char before){

    if(before){
        fat->lfname = fatfs->longName;
        fat->lfsize = fatfs->nameSz;
    }
    else{
        int i;

        fatfs->longName = fat->lfname;
        fatfs->nameSz = fat->lfsize;
        fatfs->fsize = fat->fsize;
        fatfs->attrib = fat->fattrib;
        for(i = 0 ;i < 13; i++) fatfs->name[i] = fat->fname[i];
    }
}

char ADK::fatfsReadDir(struct FatDir* d, FatFileInfo* i){				// Read a directory item

    FILINFO fi;
    char ret;

    fatfsfileInfoToFileInfo(&fi, i, 1);
    ret = f_readdir((DIR*)d, &fi);
    if(ret == FR_OK) fatfsfileInfoToFileInfo(&fi, i, 0);
    return ret;
}

char ADK::fatfsCloseDir(struct FatDir* d){						// Close a directory

    free(d);
}

char ADK::fatfsStat(const char* path,  FatFileInfo* i){					// Get file status

    FILINFO fi;
    char ret;

    fatfsfileInfoToFileInfo(&fi, i, 1);
    ret = f_stat(path, &fi);
    fatfsfileInfoToFileInfo(&fi, i, 0);
    return ret;
}

char ADK::fatfsGetFree(const char* path, uint64_t* freeSize){				// Get number of free space on the drive

    char ret;
    DWORD clus;
    FATFS* fsP = &fs;

    ret = f_getfree("/", &clus, &fsP);
    if(freeSize) *freeSize = ((uint64_t)clus * (uint64_t)fs.csize) << 9;
    return ret;
}

char ADK::fatfsUnlink(const char* path){						// Delete an existing file or directory

    return f_unlink(path);
}

char ADK::fatfsMkdir(const char* path){							// Create a new directory

    return f_mkdir(path);
}

char ADK::fatfsChmod(const char* path, uint8_t val, uint8_t mask){			// Change attributes of the file/dir

    return f_chmod(path, val, mask);
}

char ADK::fatfsRename(const char* path, const char* newPath){				// Rename/Move a file or directory

    return f_rename(path, newPath);
}

char ADK::fatfsMkfs(void){								// Create a file system on the drive

    return f_mkfs(0, 0, 0);
}















void ADK::l2capServiceTx(uint16_t conn, uint16_t remChan, const uint8_t* data, uint32_t size){ //send data over L2CAP
    
    sg_buf* buf = ::sg_alloc();

    if(!buf) return;
    if(::sg_add_back(buf, data, size, SG_FLAG_MAKE_A_COPY)) ::l2capServiceTx(conn, remChan, buf);
    else ::sg_free(buf);
}

void ADK::l2capServiceCloseConn(uint16_t conn, uint16_t chan){

    ::l2capServiceCloseConn(conn, chan);
}

char ADK::l2capServiceRegister(uint16_t PSM, const L2capService* svcData){

    return::l2capServiceRegister(PSM, svcData);
}

char ADK::l2capServiceUnregister(uint16_t PSM){

    return ::l2capServiceUnregister(PSM);
}

void ADK::btSdpServiceDescriptorAdd(const uint8_t* descriptor, uint16_t descrLen){

    ::btSdpServiceDescriptorAdd(descriptor, descrLen);
}

void ADK::btSdpServiceDescriptorDel(const uint8_t* descriptor){

    ::btSdpServiceDescriptorDel(descriptor);
}

void ADK::btRfcommRegisterPort(uint8_t dlci, BtRfcommPortOpenF oF, BtRfcommPortCloseF cF, BtRfcommPortRxF rF){

    ::btRfcommRegisterPort(dlci, oF, cF, rF);
}

void ADK::btRfcommPortTx(void* port, uint8_t dlci, const uint8_t* data, uint16_t size){

    ::btRfcommPortTx(port, dlci, data, size);
}

uint8_t ADK::btRfcommReserveDlci(uint8_t preference){

    return ::btRfcommReserveDlci(preference);
}

void ADK::btRfcommReleaseDlci(uint8_t dlci){

    ::btRfcommReleaseDlci(dlci);
}

// USB
void ADK::usbStart()
{
    ::usbh_start();
}

void ADK::usbSetAccessoryStringVendor(const char *str)
{
    ::usbh_set_accessory_string_vendor(str);
}

void ADK::usbSetAccessoryStringName(const char *str)
{
    ::usbh_set_accessory_string_name(str);
}

void ADK::usbSetAccessoryStringLongname(const char *str)
{
    ::usbh_set_accessory_string_longname(str);
}

void ADK::usbSetAccessoryStringVersion(const char *str)
{
    ::usbh_set_accessory_string_version(str);
}

void ADK::usbSetAccessoryStringUrl(const char *str)
{
    ::usbh_set_accessory_string_url(str);
}

void ADK::usbSetAccessoryStringSerial(const char *str)
{
    ::usbh_set_accessory_string_serial(str);
}

int ADK::accessoryConnected()
{
    return ::usbh_accessory_connected();
}

int ADK::accessorySend(const void *buf, unsigned int len)
{
    return ::accessory_send(buf, len);
}

int ADK::accessoryReceive(void *buf, unsigned int len)
{
    return ::accessory_receive(buf, len);
}

int ADK::accessorySendHidEvent(void *buf, unsigned int len)
{
	return ::accessory_send_hid(buf, len);
}

extern "C"{
void logHardFault(unsigned long* auto_pushed, unsigned long* self_pushed)
{
    unsigned long r[16], sr;
    int i;

    for(i = 0; i < 4; i++) r[i] = auto_pushed[i];
    r[12] = auto_pushed[4];
    r[13] = ((unsigned long)auto_pushed) - 32;
    r[14] = auto_pushed[5];
    r[15] = auto_pushed[6];
    sr = auto_pushed[7];

    for(i = 4; i < 11; i++) r[i] = self_pushed[i - 4];

    dbgPrintf ("\n\n[Cortex-M3 HARD FAULT]\n");
    for(i = 0 ;i < 16; i++) dbgPrintf ("R%2d = 0x%08X\n", i, r[i]);
    dbgPrintf ("SR  = 0x%08X\n", sr);
    while (1);
}

void __attribute__((naked)) HardFault_Handler(){
    asm(
        "tst lr, #4	\n\t"
        "ite eq		\n\t"
        "mrseq r0, msp	\n\t"
        "mrsne r0, psp	\n\t"
        "push {r4-r11}	\n\t"
        "mov r1, sp	\n\t"
        "B logHardFault	\n\t"
    );
}
}


