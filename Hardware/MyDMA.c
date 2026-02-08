#include "MyDMA.h"
#include <stdio.h>
#include <stdlib.h>
#include "Serial.h"
#include "stm32f10x.h" // Device header
#include "stm32f10x_dma.h"

uint8_t DMA_RxBuff[RX_BUFF_LENGTH];
uint16_t LastTotalDataLength;
uint8_t* getDM_RxBuff(void)
{
    return DMA_RxBuff;
}

/**
 * @brief 初始化USART1的DMA传输
 * @param Serial 串口实例指针
 * @param Direction 传输方向（DMA_DIR_TX或DMA_DIR_RX）
 */
void DMA_USART1_Init(Serial_t *Serial, uint8_t Direction)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 参数检查 */
    if (Serial == NULL)
        return;

    /* 使能DMA时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    if (Direction == DMA_DIR_TX)
    {
        /* 初始化DMA发送通道 */
        /* 配置DMA通道指针（如果未设置） */
        if (Serial->DmaTxChannel == NULL)
        {
            Serial->DmaTxChannel = DMA_USART1_TX_CHANNEL;
        }

        /* 配置DMA通道参数 */
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&Serial->USARTx->DR;
        DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial->DmaTxBuffer;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; /* 内存到外设 */
        DMA_InitStructure.DMA_BufferSize = Serial->DmaTxSize;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; /* 正常模式 */
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

        DMA_Init(Serial->DmaTxChannel, &DMA_InitStructure);

        /* 配置USART DMA请求 */
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Tx, ENABLE);

        // 使能DMA发送完成和错误中断
        DMA_ITConfig(Serial->DmaTxChannel, DMA_IT_TC | DMA_IT_TE, ENABLE);

        /*NVIC中断分组*/
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC为分组2
        /*NVIC配置*/
        NVIC_InitTypeDef NVIC_InitStructure;  
        NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;  // DMA1 Channel4中断
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 较低优先级
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_Init(&NVIC_InitStructure);

        /* 使能DMA通道 */
        DMA_Cmd(Serial->DmaTxChannel, DISABLE);

        /* 标记DMA TX已使能 */
        Serial->DmaTxEn = 1;
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 初始化DMA接收通道 */
        /* 配置DMA通道指针（如果未设置） */
        if (Serial->DmaRxChannel == NULL)
        {
            Serial->DmaRxChannel = DMA_USART1_RX_CHANNEL;
        }

        /* 配置DMA通道参数 */
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&Serial->USARTx->DR;
        DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial->DmaRxBuffer;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; /* 外设到内存 */
        DMA_InitStructure.DMA_BufferSize = Serial->DmaRxSize;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; /* 循环缓冲区模式 */
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
        DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

        DMA_Init(Serial->DmaRxChannel, &DMA_InitStructure);

        /* 配置USART DMA请求 */
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Rx, ENABLE);

        // 使能DMA接收半传输、传输完成和错误中断
        DMA_ITConfig(Serial->DmaRxChannel, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, ENABLE);
        
        /*NVIC中断分组*/
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC为分组2
        /*NVIC配置*/
        NVIC_InitTypeDef NVIC_InitStructure;  
        NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;  // DMA1 Channel4中断
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 较低优先级
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_Init(&NVIC_InitStructure);

        /* 使能DMA通道 */
        DMA_Cmd(Serial->DmaRxChannel, ENABLE);

        /* 标记DMA RX已使能 */
        Serial->DmaRxEn = 1;
    }
}

void DMA_QueueSend(Serial_t *Serial)
{
    if (Serial->DmaTxEn && !Serial->DmaTxBusy)
    {
        uint8_t *buffer_start;
        uint16_t linear_length;
        
        // 处理回绕情况
        if (Serial->TxQueue->ReadIndex <= Serial->TxQueue->WriteIndex || Serial->TxQueue->WriteIndex == 0)
        {
            // 数据连续存储
            buffer_start = &Serial->TxQueue->DataQueue[Serial->TxQueue->ReadIndex];
            linear_length = abs(Serial->TxQueue->WriteIndex - Serial->TxQueue->ReadIndex);
            if (linear_length > 0)
            {
                DMA_Cmd(Serial->DmaTxChannel, DISABLE);
                Serial->DmaTxChannel->CMAR = (uint32_t)buffer_start;
                Serial->DmaTxChannel->CNDTR = linear_length;
                DMA_Cmd(Serial->DmaTxChannel, ENABLE);
                Serial->DmaTxBusy = 1;
            }
        }
        else
        {
            // 数据回绕，先发送到缓冲区末尾的数据
            buffer_start = &Serial->TxQueue->DataQueue[Serial->TxQueue->ReadIndex];
            linear_length = Serial->TxQueue->QueueLength - Serial->TxQueue->ReadIndex;
            DMA_Cmd(Serial->DmaTxChannel, DISABLE);
            Serial->DmaTxChannel->CMAR = (uint32_t)buffer_start;
            Serial->DmaTxChannel->CNDTR = linear_length;
            DMA_Cmd(Serial->DmaTxChannel, ENABLE);
            Serial->DmaTxBusy = 1;
        }
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
    DMA_Cmd(Serial->DmaTxChannel, DISABLE);                    // 先禁用DMA通道
    Serial->DmaTxChannel->CMAR = (uint32_t)data;              // 设置DMA内存地址
    Serial->DmaTxChannel->CNDTR = size;                       // 设置DMA传输数量
    DMA_Cmd(Serial->DmaTxChannel, ENABLE);                     // 启动DMA传输

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
    {
        return;
    }

    /* 配置DMA传输 */
    DMA_Cmd(Serial->DmaRxChannel, DISABLE);                   // 先禁用DMA通道
    Serial->DmaRxChannel->CMAR = (uint32_t)buffer;           // 设置DMA内存地址
    Serial->DmaRxChannel->CNDTR = size;                      // 设置DMA传输数量
    DMA_Cmd(Serial->DmaRxChannel, ENABLE);                    // 启动DMA传输

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
        DMA_Cmd(Serial->DmaTxChannel, ENABLE);                // 使能DMA通道
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Tx, ENABLE); // 使能USART DMA发送请求
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 启动DMA接收 */
        DMA_Cmd(Serial->DmaRxChannel, ENABLE);                // 使能DMA通道
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Rx, ENABLE); // 使能USART DMA接收请求
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
        DMA_Cmd(Serial->DmaTxChannel, DISABLE);               // 禁用DMA通道
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Tx, DISABLE); // 禁用USART DMA发送请求
        Serial->DmaTxBusy = 0;                                // 清除繁忙标志
    }
    else if (Direction == DMA_DIR_RX)
    {
        /* 停止DMA接收 */
        DMA_Cmd(Serial->DmaRxChannel, DISABLE);               // 禁用DMA通道
        USART_DMACmd(Serial->USARTx, USART_DMAReq_Rx, DISABLE); // 禁用USART DMA接收请求
        Serial->DmaRxBusy = 0;                                // 清除繁忙标志
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

/**
 * 函    数：DMA1 Channel4中断处理函数（USART1 TX DMA）
 * 参    数：无
 * 返 回 值：无
 */
void DMA1_Channel4_IRQHandler(void)
{
    // 处理传输完成中断
    if (DMA_GetITStatus(DMA1_IT_TC4) == SET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        Serial_t *SerialA1 = getSerialA1();
        SerialA1->DmaTxBusy = 0;  // 清除发送繁忙标志
        // 重置队列指针，准备下一次发送
        SerialA1->TxQueue->ReadIndex = 0;
        SerialA1->TxQueue->WriteIndex = 0;
        DMA_Cmd(SerialA1->DmaTxChannel, DISABLE);
    }
    
    // 处理传输错误中断
    if (DMA_GetITStatus(DMA1_IT_TE4) == SET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE4);
        Serial_t *SerialA1 = getSerialA1();
        SerialA1->DmaTxBusy = 0;  // 清除发送繁忙标志
        // 重置队列指针，准备下一次发送
        SerialA1->TxQueue->ReadIndex = 0;
        SerialA1->TxQueue->WriteIndex = 0;
        DMA_Cmd(SerialA1->DmaTxChannel, DISABLE);
    }
}

/**
 * 函    数：DMA1 Channel5中断处理函数（USART1 RX DMA）
 * 参    数：无
 * 返 回 值：无
 */
void DMA1_Channel5_IRQHandler(void)
{
    // 处理半传输完成中断
    if (DMA_GetITStatus(DMA1_IT_HT5) == SET)
    {
        Serial_t *SerialA1 = getSerialA1();
        uint16_t TotalDataLength;
        uint16_t NeedHandleLength;
        
        TotalDataLength = SerialA1->DmaRxSize - DMA_GetCurrDataCounter(SerialA1->DmaRxChannel);
        NeedHandleLength = TotalDataLength - LastTotalDataLength;
        giveQueueBuff(SerialA1->RxQueue, &DMA_RxBuff[LastTotalDataLength], NeedHandleLength);
        LastTotalDataLength = TotalDataLength;

        DMA_ClearITPendingBit(DMA1_IT_HT5);
        // 半传输完成处理（可在此处处理已接收的数据）
    }
    
    // 处理传输完成中断
    if (DMA_GetITStatus(DMA1_IT_TC5) == SET)
    {
        Serial_t *SerialA1 = getSerialA1();
        uint16_t NeedHandleLength;
        NeedHandleLength = SerialA1->DmaRxSize - LastTotalDataLength;
        giveQueueBuff(SerialA1->RxQueue, &DMA_RxBuff[LastTotalDataLength], NeedHandleLength);
        LastTotalDataLength = 0;

        DMA_ClearITPendingBit(DMA1_IT_TC5);
        // 传输完成处理（对于循环模式，这表示缓冲区已满一轮）
    }
    
    // 处理传输错误中断
    if (DMA_GetITStatus(DMA1_IT_TE5) == SET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE5);
        // 传输错误处理
    }
}

void USART1_IRQHandler(void)
{
    Serial_t *SerialA1 = getSerialA1();
    if (USART_GetITStatus(SerialA1->USARTx, USART_IT_IDLE) == SET) // 判断是否是USART1的接收事件触发的中断
    {
        if (SerialA1->DmaRxEn)
        {
            // DMA模式处理
            uint16_t TotalDataLength;
            uint16_t NeedHandleLength;
            
            TotalDataLength = SerialA1->DmaRxSize - DMA_GetCurrDataCounter(SerialA1->DmaRxChannel);
            NeedHandleLength = TotalDataLength - LastTotalDataLength;
            giveQueueBuff(SerialA1->RxQueue, &DMA_RxBuff[LastTotalDataLength], NeedHandleLength);
            LastTotalDataLength = TotalDataLength;
        }
        USART_ClearITPendingBit(SerialA1->USARTx, USART_IT_IDLE); // 清除标志位
    }
}
