#include "stm32f10x.h"
#include "stm32f10x_dma.h"

#ifndef __SERIAL_H
#define __SERIAL_H

/*******************************************配置区头******************************************/

#define RX_QUEUE_LENGTH 50

typedef struct Queue {
    uint8_t WriteIndex;
    uint8_t ReadIndex;
    uint8_t QueueLength;
    uint8_t DataQueue[50];
} Queue_t;

typedef struct Serial {
    uint8_t SerialName[10];
    Queue_t *RxQueue;
    Queue_t *TxQueue;
    uint16_t TxPin;
    uint16_t RxPin;
    USART_TypeDef *USARTx;
    IRQn_Type USARTx_IRQn;
    uint8_t NVIC_IRQChannelPreemptionPriority;
    uint8_t NVIC_IRQChannelSubPriority;
    uint8_t DmaTxEn;
    uint8_t DmaRxEn;
    DMA_Channel_TypeDef *DmaTxChannel;
    DMA_Channel_TypeDef *DmaRxChannel;
    uint8_t *DmaTxBuffer;
    uint8_t *DmaRxBuffer;
    uint8_t DmaTxSize;
    uint8_t DmaRxSize;
    uint8_t DmaTxBusy;
    uint8_t DmaRxBusy;
} Serial_t;

void Serial_Init(uint8_t Num, ...);
void OneSerial_Init(Serial_t *Serial);

void createSerialA1(void);

/*******************************************配置区尾******************************************/

/*******************************************内部函数区头******************************************/

void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);

uint8_t rxQueueIsEmpty(Serial_t *Serial);
uint8_t writeIndexAdd(Queue_t *Queue);
uint8_t readIndexAdd(Queue_t *Queue);
uint8_t getQueueOneData(Queue_t *Queue);
uint8_t giveQueueBuff(Queue_t *Queue, uint8_t *buff, uint16_t Size);
uint8_t giveQueueOneData(Queue_t *Queue, uint8_t Data);


/*******************************************内部函数区尾******************************************/

/*******************************************应用函数区头******************************************/

Serial_t *getSerialA1(void);
void Serial_Send(void *Data);
void Serial_Send_byDMA(Serial_t *Serial, void *Data);
uint8_t Serial_UnpackReceive(void *Buffer, uint8_t MaxSize, Serial_t *Serial);

/*******************************************应用函数区尾******************************************/

#endif
