/**
 * main.cpp
 */

#include "drivers/DigitalOut.h"
#include "drivers/SPI.h"
#include "rtos/ThisThread.h"
#include "EasyScale.h"
#include "CAN.h"

#include "events/EventQueue.h"

#include "LittlevGL.h"
#include "MarqDisplayDriver.hpp"

#include "NCV7608/NCV7608.h"

#include "PinNames.h"

#include "lv_label.h"
#include "lv_btn.h"
#include "lv_disp.h"

#define EXTERNAL_MEMORY_SIZE_BYTES 8388608
#define EXTERNAL_RAM_BASE 0xC0000000
#define DISPLAY_BUFFER_BASE 0xC0177000

#define DISPLAY_BUFFER_SIZE_PIXELS 384000 // (800*480)

#define SPI_MOSI PB_5
#define SPI_MISO PG_9
#define SPI_SCLK PB_3

#define CSB1 PG_14
#define CSB2 PG_13
#define CSB3 PD_4
#define GPO_EN PD_3

mbed::DigitalOut led1(LED1, 1);
EasyScale bl_ctrl(PB_4);
mbed::CAN can(PB_12, PB_13);
mbed::DigitalOut disp_en(PG_1, 1);
mbed::DigitalOut csb1(CSB1, 1);
mbed::DigitalOut csb2(CSB2, 1);

events::EventQueue main_queue;

mbed::SPI ncv_spi(SPI_MOSI, SPI_MISO, SPI_SCLK);

ep::NCV7608 ncv7608(ncv_spi, CSB3, GPO_EN);

MarqDisplayDriver display(mbed::Span<lv_color_t>((lv_color_t*)EXTERNAL_RAM_BASE,
        DISPLAY_BUFFER_SIZE_PIXELS));

lv_style_t background_style;

void create_gui(void) {
    /** Set up styles */
    // Setup the background style
    lv_style_copy(&background_style, &lv_style_plain);
    //memset(&background_style, 0, sizeof(lv_style_t));
    background_style.body.main_color = LV_COLOR_BLACK;
    background_style.body.grad_color = LV_COLOR_BLACK;
    background_style.body.border.color = LV_COLOR_BLACK; // No border
    background_style.body.border.width = 0;
    background_style.text.color = LV_COLOR_WHITE;
    background_style.text.font = &lv_font_roboto_28;
    background_style.image.color = LV_COLOR_WHITE;

    lv_obj_set_style(lv_scr_act(), &background_style);

    lv_obj_t* hello_btn = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_t* hello_lbl = lv_label_create(hello_btn, NULL);
//    lv_obj_set_style(hello_lbl, &background_style);
    lv_label_set_text(hello_lbl, "Hello World!");
    lv_obj_align(hello_lbl, hello_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(hello_btn, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);

}



static int channel_num = 1;
void cycle_hsls_channel(void) {

    ep::NCV7608::ChannelOut channel = ncv7608.channel(channel_num);

    // See if there's a fault
    ep::NCV7608::fault_condition_t fault = channel.get_fault();
    if(fault == ep::NCV7608::OVER_CURRENT) {
        printf("ncv7608 [0x%X]: over current on channel %i!\r\n", (int)(&ncv7608), channel_num);
    } else if(fault == ep::NCV7608::THERMAL_FAULT) {
        printf("ncv7608 [0x%X]: thermal fault on channel %i\r\n", (int)(&ncv7608), channel_num);
    }

    // Turn current channel off
    channel.off();

    // Enable open load diagnostics on channel
    channel.enable_open_load_diag();

    rtos::ThisThread::sleep_for(1);

    fault = channel.get_fault();
    if(fault == ep::NCV7608::OPEN_LOAD) {
        printf("ncv7608 [0x%X]: open load on channel %i!\r\n", (int)(&ncv7608), channel_num);
    }

    // Disable open load diagnostics on channel
    channel.disable_open_load_diag();

    channel_num++;
    if(channel_num > 8) {
        channel_num = 1;
    }

    // Turn next channel on
    ncv7608.channel(channel_num).on();

}

int main(void) {

    volatile uint32_t* ext_mem_ptr = (uint32_t*) EXTERNAL_RAM_BASE;
    uint32_t count = 1;

//    while((count-1) < (EXTERNAL_MEMORY_SIZE_BYTES >> 2)) {
//        *ext_mem_ptr = count;
//        MBED_ASSERT(*ext_mem_ptr == count);
//        ext_mem_ptr++;
//        count++;
//    }

//    while((count-1) < (EXTERNAL_MEMORY_SIZE_BYTES >> 2)) {
//        *ext_mem_ptr = 0xFF000000;
//        ext_mem_ptr++;
//        count++;
//    }

    printf("cm7: app start\r\n");

    ncv_spi.format(16, 1);
    ncv_spi.frequency(100E3);

    ncv7608.enable();

    LittlevGL&  lvgl = LittlevGL::get_instance();

    lvgl.init();
    lvgl.add_display_driver(display);

    create_gui();

    bl_ctrl.power_on();
    bl_ctrl.set_brightness(25, EasyScale::DEVICE_ADDRESS_TPS61165);

    main_queue.call_every(10, mbed::callback(&lvgl, &LittlevGL::update));

    main_queue.call_every(500, cycle_hsls_channel);

    main_queue.dispatch_forever();

    while(true) { /* Should never be reached */ }

}
