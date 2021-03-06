#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
#include <SkXfermode.h>
#include <android/native_window.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <wchar.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <cutils/properties.h>
#include <cutils/memory.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <ui/DisplayInfo.h>
//#include <ui/Rect.h>
//#include <ui/Region.h>

#include <SkImageDecoder.h>
#include <SkGraphics.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkStream.h>
#include "SkBitmap.h"
#include "SkDevice.h"
// install signal function
#include "SkPaint.h"
#include "SkColorFilter.h"
#include "SkRect.h"
//#include "SkImageEncoder.h" // not found

#include "SkTypeface.h"
//#include "develop_watermark.h"
//#include <hardware/hwcomposer_defs.h>

//#include <core/SkImageDecoder.h>
#include <cutils/properties.h>

#define PROCESS_ID "process_property"
using namespace android;

struct ImageInfo {
    SkBitmap bmp;
    int32_t height;
    int32_t width;
};

const char* develop_info_zh= "开发机";
const char* develop_info_en= "Develop";
static pthread_t tids[4] = {0}; // each thread blend one text
static pthread_t imageTid = 0;
const char *prop1 = "myprop1"; // the property to detect
const char *prop2 = "myprop2"; // the property to detect
int textActivated = 0;
int textChangeTo = 0;
int imageActivated = 0;
int imageChangeTo = 0;

static SkBitmap::Config convertPixelFormat(PixelFormat format) {
    /* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
        we can map to SkBitmap::kARGB_8888_Config, and optionally call
        bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
    */
    switch (format) {
    case PIXEL_FORMAT_RGBX_8888:    return SkBitmap::kARGB_8888_Config;
    case PIXEL_FORMAT_RGBA_8888:    return SkBitmap::kARGB_8888_Config;
    case PIXEL_FORMAT_RGBA_4444:    return SkBitmap::kARGB_4444_Config;
    case PIXEL_FORMAT_RGB_565:      return SkBitmap::kRGB_565_Config;
    default:                        return SkBitmap::kNo_Config;
    }   
}

static void* DisplayText(void *para);
static void* DisplayImage(void *para);
int showDebugMachineInfo();
int showTempWarning(int ac, char *av[]);
int test();
void sendSignal();
void sigUsrFunc(int sig);

int main(int ac, char *av[]) {

    if (ac == 2 && !strcmp(av[1], "kill")) {  //send a signal  SIGUSR1
        sendSignal();
        return 0;
    } 

    test();
}

// this part executed when program is called with argument "kill"
void sendSignal(void) {
    char val[128];
    property_get(PROCESS_ID, val, "");
    pid_t processId = strtol(val, NULL, 10);
    if (processId <= 0) {
        perror("can not get process ");
        return;
    }

    printf("got process id %d\n", processId);
    if (kill(processId, SIGUSR1) < 0) {
        perror("can not kill ");
        return;
    }
}

int test()
{
    char val1[128], valInput[128];
     
    // install signal function
    signal(SIGUSR1, sigUsrFunc);
    sprintf(valInput, "%d", getpid());
    property_set(PROCESS_ID, valInput); // set process's pid
    property_get(prop1, val1, "off");
    if (!strcmp(val1, "on")) {
        textChangeTo = 1;
    } else  {
        textChangeTo = 0;
    }
    showDebugMachineInfo(); // means show text
    property_set(PROCESS_ID, ""); // clean process's pid
	return 0;
}

int showTempWarning(int ac, char *av[])
{
    printf("callbacked !!\n");
	return 0;
}

int showDebugMachineInfo()
{
    // 
	char para[2] = {0};
	para[0] = 0;

    while(1) {
        if (textActivated == 0 && textChangeTo == 1) { // set attr
            for (size_t i = 0; i < sizeof(tids); ++i) {
                para[0] = i;
                pthread_create(&tids[i], NULL, &DisplayText, para);
                usleep(200*1000);
            }
            textActivated = 1;
        } else if (textActivated == 1 && textChangeTo == 0) { // unset attr
            for (size_t i = 0; i < sizeof(tids); ++i) {
                pthread_kill(tids[i], SIGTERM);
                usleep(200*1000);
            }
            textActivated = 0;
        }
        if (imageActivated == 0 && imageChangeTo == 1) { // set attr
            pthread_create(&imageTid, NULL, &DisplayImage, para);
            imageActivated = 1;
        } else if (imageActivated == 1 && imageChangeTo == 0) { // unset attr
            pthread_kill(imageTid, SIGTERM);
            imageActivated = 0;
        }
        sleep(100); // this will wake each time signal is invoked
    }
	return 0;
}

void sigUsrFunc(int sig) {
    // getproperty
    // set flag which will be called after sleep
    // return;
    char val1[128];
    property_get(prop1, val1, "off");
    if (!strcmp(val1, "on")) {
        textChangeTo = 1;
    } else {
        textChangeTo = 0;
    }
}

static void getScreenSize(int32_t *width, int32_t *height) {
    struct fb_var_screeninfo fb_var;
    int fd = open("/dev/graphics/fb0", O_RDONLY);
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    close(fd);
    *height = fb_var.xres;
    *width = fb_var.yres;
}

static int getImageInfo(const char* path, ImageInfo *info) {
    // get image size
	SkFILEStream stream(path);
	SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
	if(!codec) return -1;

    stream.rewind();
    codec->decode(&stream, &info->bmp, SkBitmap::kRGB_565_Config, SkImageDecoder::kDecodePixels_Mode);
    SkIRect bounds;
    info->bmp.getBounds(&bounds);
    info->width = bounds.fRight;
    info->height = bounds.fBottom;
    return 0;
}

void putOnImage(float transparent, sp<SurfaceControl> &sControl, 
            ImageInfo &iInfo, ANativeWindow_Buffer &buffer, 
            int32_t picX, int32_t picY, ssize_t ibpr) {
    sControl->setAlpha(transparent);
    SkBitmap bitmap;
    bitmap.setConfig(convertPixelFormat(buffer.format), iInfo.width, iInfo.height, ibpr);
    bitmap.setPixels(buffer.bits);
    SkCanvas canvas(bitmap);
    // draw partition offset of picture
    canvas.drawBitmap(iInfo.bmp, picX, picY);
}

static void DisplayWarning(void *para){
}

static void* DisplayText(void *para){
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    int32_t sHeight, sWidth;


    int32_t textSize = 32;
    char text[32];
    int count =0;
    char lan[128];


    getScreenSize(&sWidth, &sHeight);

    printf("going to display text\n");
    property_get("persist.sys.language", lan, "zh");
    printf("lan=%s\n",lan);
    if(strncmp(lan, "zh",2)!=0){
	count = strlen(develop_info_zh);
    
    //this has problem with chinese ,should use wchar_t
	strncpy(text,develop_info_zh, count);
    printf("count is %d\n", count);
    }
    else{
	count = strlen(develop_info_en);
	memcpy(text,develop_info_en,count);
    }
    text[count] = '\0';
    printf("set done\n");

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(textSize);
    paint.setColor(SkColorSetRGB(255,0,0)); // red
    paint.setStyle(SkPaint::kStroke_Style);  // style of text
//    paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);

    // get text length on screen
    int glyphCount = paint.getTextWidths(text, count, NULL);
    SkScalar widths[textSize];
    paint.getTextWidths(text, glyphCount, widths);
    int textWidth = 0;
    for (int i = 0; i < glyphCount; ++i) {
        textWidth += widths[i];
    }
    printf("textwidth is %d\n", textWidth);
    
    // mury: surface size, size can be modified by setSize
    // this is hard restriction
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            textWidth, textSize, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
    if (((char*)para)[0] == 0){
        surfaceControl->setPosition(0,50);
    } else if (((char*)para)[0] == 1) {
        surfaceControl->setPosition(720-textWidth, 1280-textSize-96);
    } else if(((char*)para)[0] == 2) {
        surfaceControl->setPosition(0, 1280-textSize-96);
    } else if(((char*)para)[0] == 3){
        surfaceControl->setPosition(720-textWidth, 50);
    } else {
        surfaceControl->setPosition(0, 50);
    }

    // mury: set surface size , this function have no effect !!!
 


	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
	surfaceControl->setAlpha(0.5);
	surfaceControl->show();
	SkBitmap bitmap;
	bitmap.setConfig(convertPixelFormat(outBuffer.format), textWidth, textSize, bpr);
	bitmap.setPixels(outBuffer.bits);
	SkCanvas canvas(bitmap);
	canvas.clear(SK_ColorWHITE);

//        canvas.clear(SkColorSetARGB(0,255,255,255));
	canvas.drawText(text, textWidth, 0, textSize, paint );

	surface->unlockAndPost();
	SurfaceComposerClient::closeGlobalTransaction();	
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}

static void* DisplayImage(void *para) {
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	ImageInfo imageInfo;
	if (getImageInfo("/system/cpos.png", &imageInfo) == -1) return NULL;

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

	// write in one func
	int32_t sHeight, sWidth;
	getScreenSize(&sWidth, &sHeight);

	// mury: surface size, size can be modified by setSize
	// this is hard restriction
	sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            imageInfo.width, imageInfo.height, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
	// mury: set surface start pos of left upper
	surfaceControl->setPosition(0, (1280-imageInfo.height)/2);


	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
	putOnImage(0.5, surfaceControl, imageInfo, outBuffer, 0, 0, bpr);

	surface->unlockAndPost();
	SurfaceComposerClient::closeGlobalTransaction();	
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}
