#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "Serial.h"

#ifndef __MYDMA_H
#define __MYDMA_H

/*******************************************配置区头******************************************/

#define RX_BUFF_LENGTH  RX_QUEUE_LENGTH

/* DMA方向定义 */
#define DMA_DIR_TX 0
#define DMA_DIR_RX 1

/* DMA通道定义（USART1） */
#define DMA_USART1_TX_CHANNEL DMA1_Channel4
#define DMA_USART1_RX_CHANNEL DMA1_Channel5

uint8_t* getDM_RxBuff(void);

/*******************************************配置区尾******************************************/

/*******************************************DMA管理器API函数区头******************************************/

void DMA_QueueSend(Serial_t *Serial);

/**
 * @brief 初始化USART1的DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Init(Serial_t *Serial, uint8_t Direction);

/**
 * @brief 启动USART1 DMA发送
 * @param Serial 串口实例指针
 * @param data 发送数据缓冲区指针
 * @param size 发送数据大小
 */
void DMA_USART1_TxSend(Serial_t *Serial, uint8_t *data, uint16_t size);

/**
 * @brief 启动USART1 DMA接收
 * @param Serial 串口实例指针
 * @param buffer 接收数据缓冲区指针
 * @param size 接收数据大小
 */
void DMA_USART1_RxReceive(Serial_t *Serial, uint8_t *buffer, uint16_t size);

/**
 * @brief 启动USART1 DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Start(Serial_t *Serial, uint8_t Direction);

/**
 * @brief 停止USART1 DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Stop(Serial_t *Serial, uint8_t Direction);

/**
 * @brief 检查USART1 DMA发送是否繁忙
 * @param Serial 串口实例指针
 * @return 1:繁忙 0:空闲
 */
uint8_t DMA_USART1_TxBusy(Serial_t *Serial);

/**
 * @brief 检查USART1 DMA接收是否繁忙
 * @param Serial 串口实例指针
 * @return 1:繁忙 0:空闲
 */
uint8_t DMA_USART1_RxBusy(Serial_t *Serial);

/*******************************************DMA管理器API函数区尾******************************************/

#endif /* __MYDMA_H */
