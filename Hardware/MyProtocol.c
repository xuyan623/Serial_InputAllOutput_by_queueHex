#include "Serial.h"
#include "stm32f10x.h" // Device header
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t protocolUnpack(void *Buffer, uint8_t MaxSize, Serial_t *Serial)
{
    // 参数检查
    if (Buffer == NULL || MaxSize == 0 || Serial == NULL)
    {
        return 0;
    }

    uint8_t dataIndex = 0;
    uint8_t receivedStartFlag = 0;
    uint8_t receivedEndFlag = 0;
    uint8_t *byteBuffer = (uint8_t *)Buffer;

    memset(byteBuffer, 0, sizeof(uint8_t) * MaxSize);

    // 等待接收完整的数据包起始标志
    while (!rxQueueIsEmpty(Serial) && !receivedStartFlag)
    {
        uint8_t tempData = getQueueOneData(Serial->RxQueue);

        if (tempData == 0xFF) // 接收到起始标志
        {
            receivedStartFlag = 1;
            dataIndex = 0; // 重置索引
        }
    }

    // 如果没有接收到起始标志，返回0
    if (!receivedStartFlag)
    {
        return 0;
    }

    // 接收数据直到结束标志
    while (receivedStartFlag && !receivedEndFlag)
    {
        if(!rxQueueIsEmpty(Serial))
        {
            uint8_t tempData = getQueueOneData(Serial->RxQueue);

            if (tempData == 0xFE) // 接收到结束标志
            {
                receivedEndFlag = 1;
            }
            else
            {
                //接收字符直到缓冲区满
                if (dataIndex < MaxSize)
                {
                    byteBuffer[dataIndex++] = tempData;
                }
                else
                {
                    // 缓冲区已满，继续读取但不存储，直到遇到结束标志
                    continue;
                }
            }
        }
    }

    return dataIndex;
}
