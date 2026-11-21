/*
 * app_metadata.c — APP 元数据块（bootloader 校验依据）
 *
 * Flash 布局（与 boot_jig 配套，详见 design/IAP架构与移植方案.md）：
 *   0x8000000  bootloader（10KB）
 *   0x8002800  APP 代码区起点（ApplicationAddress）
 *   0x8004800  APP 元数据块（本文件，占 2KB 页首页，共 28 字节有效）
 *   APP 末尾   0x145089bd 末尾标记
 *
 * boot 的 DFU_CheckUserApp() 校验三项：
 *   1) [+0] 末尾指针 ≤ 0x803F800，且该指针处内容为 0x145089bd
 *   2) [+4] Program_Mark == 0xFFFFFFFF（APP 写 0x55AA5A55 请求升级，见 iapgoboot）
 *   3) [+20] APP_SIGN == 0x14500301（KS-JIG 机型签名，防与主板固件串刷）
 *
 * 说明：
 *   - 用 J-Link 直接烧录（不经打包工具）时，[+0] 指向本块 [+24] 的 0x145089bd，
 *     自指即可通过校验；
 *   - 经 bin2bt 打包升级时，打包工具会把 [+0] 改写为真正的 APP 末尾标记地址；
 *   - APP 请求升级：擦除 0x8004800 所在页 -> 写入 Program_Mark=0x55AA5A55 -> 复位，
 *     boot 校验失败即停留升级模式（升级串口 UART4, PC10/PC11, 115200）。
 */
#include "stm32f10x.h"

#define APP_METADATA_ADDR   0x8004800
#define APP_SIGN_KS_JIG     0x14500301
#define APP_END_MARK        0x145089bd

__attribute__((at(0x8004800))) const u32 app_metadata[7] = {
    APP_METADATA_ADDR + 24,     /* +0  末尾标记指针（自指，打包时可被改写） */
    0xFFFFFFFF,                 /* +4  Program_Mark：0xFFFFFFFF=APP有效 */
    0,                          /* +8  Checksum 占位    */
    0,                          /* +12 Checksumlen 占位 */
    0,                          /* +16 Checksumdat 占位 */
    APP_SIGN_KS_JIG,            /* +20 APP_SIGN 机型签名 */
    APP_END_MARK                /* +24 末尾标记（供自指校验） */
};
