/**
 * main.cpp
 */

#include "drivers/DigitalOut.h"
#include "rtos/ThisThread.h"
#include "EasyScale.h"
#include "CAN.h"

#include "events/EventQueue.h"

#include "LittlevGL.h"
#include "MarqDisplayDriver.hpp"

#include "lv_label.h"
#include "lv_btn.h"
#include "lv_disp.h"

#define EXTERNAL_MEMORY_SIZE_BYTES 8388608
#define EXTERNAL_RAM_BASE 0xC0000000
#define DISPLAY_BUFFER_BASE 0xC0177000

#define DISPLAY_BUFFER_SIZE_PIXELS 384000 // (800*480)

mbed::DigitalOut led1(LED1, 1);
EasyScale bl_ctrl(PB_4);
mbed::CAN can(PB_12, PB_13);
mbed::DigitalOut disp_en(PG_1, 1);

events::EventQueue main_queue;

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

int main(void) {

    volatile uint32_t* ext_mem_ptr = (uint32_t*) EXTERNAL_RAM_BASE;
    uint32_t count = 1;

//    while((count-1) < (EXTERNAL_MEMORY_SIZE_BYTES >> 2)) {
//        *ext_mem_ptr = count;
//        MBED_ASSERT(*ext_mem_ptr == count);
//        ext_mem_ptr++;
//        count++;
//    }

    while((count-1) < (EXTERNAL_MEMORY_SIZE_BYTES >> 2)) {
        *ext_mem_ptr = 0xFF000000;
        ext_mem_ptr++;
        count++;
    }

    LittlevGL&  lvgl = LittlevGL::get_instance();

    lvgl.init();
    lvgl.add_display_driver(display);

    create_gui();

    bl_ctrl.power_on();
    bl_ctrl.set_brightness(25, EasyScale::DEVICE_ADDRESS_TPS61165);

    main_queue.call_every(10, mbed::callback(&lvgl, &LittlevGL::update));

    main_queue.dispatch_forever();

    while(true) { /* Should never be reached */ }

}
