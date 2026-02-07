#include "MyDMA.h"
#include "Serial.h"
#include "stm32f10x.h" // Device header
#include "stm32f10x_dma.h"

#ifdef USE_DMA

/**
 * @brief 初始化USART1的DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Init(Serial_t *Serial, uint8_t Direction)
{
    /* 参数检查 */
    if (Serial == NULL)
        return;

    if (Direction == DMA_DIR_TX)
    {
        /* 初始化DMA发送通道 */
        // TODO: 配置DMA通道参数
        // TODO: 配置USART DMA请求
        // TODO: 使能DMA通道
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 初始化DMA接收通道 */
        // TODO: 配置DMA通道参数
        // TODO: 配置USART DMA请求
        // TODO: 使能DMA通道
    }
}

/**
 * @brief 启动USART1 DMA发送
 * @param Serial 串口实例指针
 * @param data 发送数据缓冲区指针
 * @param size 发送数据大小
 */
void DMA_USART1_TxSend(Serial_t *Serial, uint8_t *data, uint16_t size)
{
    /* 参数检查 */
    if (Serial == NULL || data == NULL || size == 0)
        return;

    /* 检查DMA发送是否已使能 */
    if (Serial->DmaTxEn == 0)
        return;

    /* 配置DMA传输 */
    // TODO: 设置DMA内存地址
    // TODO: 设置DMA传输数量
    // TODO: 启动DMA传输

    /* 标记发送繁忙 */
    Serial->DmaTxBusy = 1;
}

/**
 * @brief 启动USART1 DMA接收
 * @param Serial 串口实例指针
 * @param buffer 接收数据缓冲区指针
 * @param size 接收数据大小
 */
void DMA_USART1_RxReceive(Serial_t *Serial, uint8_t *buffer, uint16_t size)
{
    /* 参数检查 */
    if (Serial == NULL || buffer == NULL || size == 0)
        return;

    /* 检查DMA接收是否已使能 */
    if (Serial->DmaRxEn == 0)
        return;

    /* 配置DMA传输 */
    // TODO: 设置DMA内存地址
    // TODO: 设置DMA传输数量
    // TODO: 启动DMA传输

    /* 标记接收繁忙 */
    Serial->DmaRxBusy = 1;
}

/**
 * @brief 启动USART1 DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Start(Serial_t *Serial, uint8_t Direction)
{
    /* 参数检查 */
    if (Serial == NULL)
        return;

    if (Direction == DMA_DIR_TX)
    {
        /* 启动DMA发送 */
        // TODO: 使能DMA通道
        // TODO: 使能USART DMA发送请求
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 启动DMA接收 */
        // TODO: 使能DMA通道
        // TODO: 使能USART DMA接收请求
    }
}

/**
 * @brief 停止USART1 DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Stop(Serial_t *Serial, uint8_t Direction)
{
    /* 参数检查 */
    if (Serial == NULL)
        return;

    if (Direction == DMA_DIR_TX)
    {
        /* 停止DMA发送 */
        // TODO: 禁用DMA通道
        // TODO: 禁用USART DMA发送请求
        // TODO: 清除繁忙标志
        Serial->DmaTxBusy = 0;
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 停止DMA接收 */
        // TODO: 禁用DMA通道
        // TODO: 禁用USART DMA接收请求
        // TODO: 清除繁忙标志
        Serial->DmaRxBusy = 0;
    }
}

/**
 * @brief 检查USART1 DMA发送是否繁忙
 * @param Serial 串口实例指针
 * @return 1:繁忙 0:空闲
 */
uint8_t DMA_USART1_TxBusy(Serial_t *Serial)
{
    /* 参数检查 */
    if (Serial == NULL)
        return 0;

    /* 检查繁忙标志 */
    return Serial->DmaTxBusy;
}

/**
 * @brief 检查USART1 DMA接收是否繁忙
 * @param Serial 串口实例指针
 * @return 1:繁忙 0:空闲
 */
uint8_t DMA_USART1_RxBusy(Serial_t *Serial)
{
    /* 参数检查 */
    if (Serial == NULL)
        return 0;

    /* 检查繁忙标志 */
    return Serial->DmaRxBusy;
}

#endif /* USE_DMA */