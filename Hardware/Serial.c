#include "Serial.h"
#include "MyDMA.h"
#include "MyProtocol.h"
#include "stm32f10x.h" // Device header
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Queue_t TxPacketQueue = {
    .ReadIndex = 0,
    .WriteIndex = 0,
    .QueueLength = 50,
    .DataQueue = {0}
};

Queue_t RxPacketQueue = {
    .ReadIndex = 0,
    .WriteIndex = 0,
    .QueueLength = 50,
    .DataQueue = {0}
};

Serial_t SerialA1;
void createSerialA1(void)
{
    uint8_t *RxBuff = getDM_RxBuff();
    strcpy((char*)SerialA1.SerialName, "A1");
    SerialA1.RxQueue = &RxPacketQueue;
    SerialA1.TxQueue = &TxPacketQueue;
    SerialA1.TxPin = GPIO_Pin_9;
    SerialA1.RxPin = GPIO_Pin_10;
    SerialA1.USARTx = USART1;
    SerialA1.USARTx_IRQn = USART1_IRQn;
    SerialA1.NVIC_IRQChannelPreemptionPriority = 1;
    SerialA1.NVIC_IRQChannelSubPriority = 1;
    SerialA1.DmaRxBuffer = RxBuff;
    SerialA1.DmaRxChannel = DMA1_Channel5;
    SerialA1.DmaRxSize = RxPacketQueue.QueueLength;
    SerialA1.DmaTxBusy = 0;
    SerialA1.DmaTxBuffer = TxPacketQueue.DataQueue;
    SerialA1.DmaTxChannel = DMA1_Channel4;
    SerialA1.DmaTxSize = TxPacketQueue.QueueLength;
    SerialA1.DmaRxBusy = 0;
}
Serial_t *getSerialA1(void)
{
    return &SerialA1;
}

uint8_t rxQueueIsEmpty(Serial_t *Serial)
{
    // 在环形缓冲区中，WriteIndex == ReadIndex 表示队列为空
    // 但需要和writeIndexAdd中的队列满检测逻辑配合
    if (Serial->RxQueue->WriteIndex == Serial->RxQueue->ReadIndex)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t writeIndexAdd(Queue_t *Queue)
{
    uint8_t nextWriteIndex = (Queue->WriteIndex + 1) % Queue->QueueLength;

    // 检查队列是否已满
    if (nextWriteIndex == Queue->ReadIndex)
    {
        // 队列已满，不移动写指针
        return 0;
    }

    Queue->WriteIndex = nextWriteIndex;
    return 1;
}

uint8_t readIndexAdd(Queue_t *Queue)
{
    // 检查队列是否为空（应使用）
    if (Queue->ReadIndex == Queue->WriteIndex) {
        return 0;  // 队列为空，不移动读指针
    }
    
    Queue->ReadIndex = (Queue->ReadIndex + 1) % Queue->QueueLength;
    return 1;
}

uint8_t giveQueueOneData(Queue_t *Queue, uint8_t Data)
{
    uint8_t nextWriteIndex = (Queue->WriteIndex + 1) % Queue->QueueLength;

    // 检查队列是否已满
    if (nextWriteIndex == Queue->ReadIndex)
    {
        // 队列已满，不写入数据
        return 0;
    }

    // 队列未满，写入数据到当前WriteIndex位置
    Queue->DataQueue[Queue->WriteIndex] = Data;
    // 更新写指针到下一个位置
    Queue->WriteIndex = nextWriteIndex;
    return 1;
}

uint8_t giveQueueBuff(Queue_t *Queue, uint8_t *buff, uint16_t Size)
{
    // 检查空指针
    if (buff == 0)
    {
        return 0;
    }
    uint8_t *Packet = (uint8_t *)buff;

    for (uint8_t i = 0; i < Size; i++)
    {
        if (!giveQueueOneData(Queue, Packet[i]))
        {
            // 队列已满，停止添加数据
            break;
        }
    }
    return 1;
}

uint8_t getQueueOneData(Queue_t *Queue)
{
    uint8_t Data;
    Data = Queue->DataQueue[Queue->ReadIndex];
    Queue->DataQueue[Queue->ReadIndex] = 0;
    readIndexAdd(Queue);
    return Data;
}

void sendTxQueueOneDataToSerial(Serial_t *Serial)
{
    if (Serial->TxQueue->WriteIndex != Serial->TxQueue->ReadIndex)
    {
        Serial_SendByte(getQueueOneData(Serial->TxQueue));
    }
}

void sendTxQueueAllDataToSerial(Serial_t *Serial)
{
    do
    {
        sendTxQueueOneDataToSerial(Serial);
    } while (TxPacketQueue.ReadIndex != TxPacketQueue.WriteIndex);
}

void Serial_Init(uint8_t Num, ...)
{
    va_list Serials;
    uint8_t i = 0;
    va_start(Serials, Num);

    for (i = 0; i < Num; i++)
    {
        OneSerial_Init((va_arg(Serials, Serial_t *)));
    }

    va_end(Serials);
}

/**
 * 函    数：串口初始化
 * 参    数：无
 * 返 回 值：无
 */
void OneSerial_Init(Serial_t *Serial)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); // 开启USART1的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 开启GPIOA的时钟

    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = Serial->TxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA9引脚初始化为复用推挽输出

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = Serial->RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA10引脚初始化为上拉输入

    /*USART初始化*/
    USART_InitTypeDef USART_InitStructure;                                          // 定义结构体变量
    USART_InitStructure.USART_BaudRate = 9600;                                      // 波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 硬件流控制，不需要
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 // 模式，发送模式和接收模式均选择
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 奇偶校验，不需要
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 停止位，选择1位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 字长，选择8位
    USART_Init(USART1, &USART_InitStructure);                                       // 将结构体变量交给USART_Init，配置USART1

    /*DMA时钟使能*/
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /*DMA初始化*/

    DMA_USART1_Init(Serial, DMA_DIR_TX);

    DMA_USART1_Init(Serial, DMA_DIR_RX);

    /*中断输出配置*/
    //USART_ITConfig(Serial->USARTx, USART_IT_RXNE, ENABLE); // 开启串口接收数据的中断

    /*NVIC中断分组*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC为分组2

    /*NVIC配置*/
    NVIC_InitTypeDef NVIC_InitStructure;                                                              // 定义结构体变量
    NVIC_InitStructure.NVIC_IRQChannel = Serial->USARTx_IRQn;                                         // 选择配置NVIC的USART1线
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                                   // 指定NVIC线路使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = Serial->NVIC_IRQChannelPreemptionPriority; // 指定NVIC线路的抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = Serial->NVIC_IRQChannelSubPriority;               // 指定NVIC线路的响应优先级为1
    NVIC_Init(&NVIC_InitStructure);                                                                   // 将结构体变量交给NVIC_Init，配置NVIC外设

    /*USART使能*/
    USART_Cmd(Serial->USARTx, ENABLE); // 使能USART1，串口开始运行
}

/**
 * 函    数：串口发送一个字节
 * 参    数：Byte 要发送的一个字节
 * 返 回 值：无
 */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte); // 将字节数据写入数据寄存器，写入后USART自动生成时序波形
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送完成
    /*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}

/**
 * 函    数：串口发送一个数组
 * 参    数：Array 要发送数组的首地址
 * 参    数：Length 要发送数组的长度
 * 返 回 值：无
 */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++) // 遍历数组
    {
        Serial_SendByte(Array[i]); // 依次调用Serial_SendByte发送每个字节数据
    }
}

/**
 * 函    数：串口发送一个字符串
 * 参    数：String 要发送字符串的首地址
 * 返 回 值：无
 */
void Serial_Send(void *Data)
{
    // 检查空指针
    if (Data == 0)
    {
        return;
    }
    uint8_t *Packet = (uint8_t *)Data;
    uint8_t DataLength = strlen((char *)Packet);

    Serial_SendByte(0xFF);

    for (uint8_t i = 0; i < DataLength; i++)
    {
        if (!giveQueueOneData(&TxPacketQueue, Packet[i]))
        {
            // 队列已满，停止添加数据
            break;
        }
    }
    sendTxQueueAllDataToSerial(&SerialA1);
    Serial_SendByte(0xFE);
}

void Serial_Send_byDMA(Serial_t *Serial, void *Data)
{
    // 检查空指针
    if (Data == 0)
    {
        return;
    }
    uint8_t *Packet = (uint8_t *)Data;
    uint8_t DataLength = strlen((char *)Packet);

    giveQueueOneData(Serial->TxQueue, 0xFF);

    for (uint8_t i = 0; i < DataLength; i++)
    {
        if (!giveQueueOneData(Serial->TxQueue, Packet[i]))
        {
            // 队列已满，停止添加数据
            break;
        }
    }
    giveQueueOneData(Serial->TxQueue, 0xFE);
    DMA_QueueSend(Serial);
}

/**
 * 函    数：次方函数（内部使用）
 * 返 回 值：返回值等于X的Y次方
 */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1; // 设置结果初值为1
    while (Y--)          // 执行Y次
    {
        Result *= X; // 将X累乘到结果
    }
    return Result;
}

uint8_t Serial_UnpackReceive(void *Buffer, uint8_t MaxSize, Serial_t *Serial)
{
    return protocolUnpack(Buffer, MaxSize, Serial);
}

/**
 * 函    数：使用printf需要重定向的底层函数
 * 参    数：保持原始格式即可，无需变动
 * 返 回 值：保持原始格式即可，无需变动
 */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch); // 将printf的底层重定向到自己的发送字节函数
    return ch;
}

/**
 * 函    数：USART1中断函数
 * 参    数：无
 * 返 回 值：无
 * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
 *           函数名为预留的指定名称，可以从启动文件复制
 *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
 */
// void USART1_IRQHandler(void)
// {
//     if (USART_GetITStatus(SerialA1.USARTx, USART_IT_RXNE) == SET) // 判断是否是USART1的接收事件触发的中断
//     {
//         uint8_t RxData = USART_ReceiveData(SerialA1.USARTx); // 读取数据寄存器，存放在接收的数据变量
//         giveQueueOneData(SerialA1.RxQueue, RxData);

//         USART_ClearITPendingBit(SerialA1.USARTx, USART_IT_RXNE); // 清除标志位
//     }
// }
