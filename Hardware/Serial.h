#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

#ifndef TX_PACKET_QUEUE_INDEX
    #define TX_PACKET_QUEUE_INDEX       1
#endif

#ifndef RX_PACKET_QUEUE_INDEX
    #define RX_PACKET_QUEUE_INDEX       2
#endif

#ifndef TX_PACKET_QUEUE_LENGTH
    #define TX_PACKET_QUEUE_LENGTH      50
#endif

#ifndef RX_PACKET_QUEUE_LENGTH
    #define RX_PACKET_QUEUE_LENGTH      50
#endif

#ifndef TX_PACKET_LENGTH
    #define TX_PACKET_LENGTH            4
#endif

#ifndef RX_PACKET_LENGTH
    #define RX_PACKET_LENGTH            4
#endif

typedef struct
{
    volatile uint8_t WriteIndex;
    volatile uint8_t ReadIndex;
    uint8_t DataQueue[50];
}Queue_t;


void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
uint8_t Serial_GetString(char *String, uint8_t MaxLength);
void Serial_SendNumber(uint32_t Number);
uint32_t Serial_GetNumber(void);
void Serial_Printf(char *format, ...);

uint8_t rxQueueIsEmpty(void);
uint8_t writeIndexAdd(volatile Queue_t *Queue, uint8_t QueueLength);
uint8_t getQueueOneData(volatile Queue_t *Queue, uint8_t QueueLength);
uint8_t giveQueueOneData(volatile Queue_t *Queue, uint8_t QueueLength, uint8_t Data);
void getQueueAllData(volatile Queue_t *Queue, uint8_t QueueLength, uint8_t *Data);
void readIndexAdd(volatile Queue_t *Queue, uint8_t QueueLength);



#endif
