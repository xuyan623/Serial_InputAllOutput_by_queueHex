#include "Serial.h"
#include "stm32f10x.h" // Device header
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Queue_t TxPacketQueue = {
    .ReadIndex = 0,
    .WriteIndex = 0,
    .QueueLength = 50,
    .DataQueue = {0}};

Queue_t RxPacketQueue = {
    .ReadIndex = 0,
    .WriteIndex = 0,
    .QueueLength = 50,
    .DataQueue = {0}};

uint8_t rxQueueIsEmpty(void)
{
    // 在环形缓冲区中，WriteIndex == ReadIndex 表示队列为空
    // 但需要和writeIndexAdd中的队列满检测逻辑配合
    if (RxPacketQueue.WriteIndex == RxPacketQueue.ReadIndex)
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

void readIndexAdd(Queue_t *Queue)
{
    Queue->ReadIndex = (Queue->ReadIndex + 1) % Queue->QueueLength;
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

uint8_t getQueueOneData(Queue_t *Queue)
{
    uint8_t Data;
    Data = Queue->DataQueue[Queue->ReadIndex];
    Queue->DataQueue[Queue->ReadIndex] = 0;
    readIndexAdd(Queue);
    return Data;
}

void getQueueAllData(Queue_t *Queue, uint8_t *Data)
{
    for (uint8_t i = 0; Queue->WriteIndex != Queue->ReadIndex; i++)
    {
        Data[i] = getQueueOneData(Queue);
    }
}

void sendTxQueueOneDataToSerial(void)
{
    if (TxPacketQueue.WriteIndex != TxPacketQueue.ReadIndex)
    {
        Serial_SendByte(getQueueOneData(&TxPacketQueue));
    }
}

void sendTxQueueAllDataToSerial(void)
{
    do
    {
        sendTxQueueOneDataToSerial();
    } while (TxPacketQueue.ReadIndex != TxPacketQueue.WriteIndex);
}

/**
 * 函    数：串口初始化
 * 参    数：无
 * 返 回 值：无
 */
void Serial_Init(void)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); // 开启USART1的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 开启GPIOA的时钟

    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA9引脚初始化为复用推挽输出

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
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

    /*中断输出配置*/
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启串口接收数据的中断

    /*NVIC中断分组*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC为分组2

    /*NVIC配置*/
    NVIC_InitTypeDef NVIC_InitStructure;                      // 定义结构体变量
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         // 选择配置NVIC的USART1线
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // 指定NVIC线路使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 指定NVIC线路的抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        // 指定NVIC线路的响应优先级为1
    NVIC_Init(&NVIC_InitStructure);                           // 将结构体变量交给NVIC_Init，配置NVIC外设

    /*USART使能*/
    USART_Cmd(USART1, ENABLE); // 使能USART1，串口开始运行
}

/**
 * 函    数：串口发送一个字节
 * 参    数：Byte 要发送的一个字节
 * 返 回 值：无
 */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte); // 将字节数据写入数据寄存器，写入后USART自动生成时序波形
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ; // 等待发送完成
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
    sendTxQueueAllDataToSerial();
    Serial_SendByte(0xFE);
}

uint8_t Serial_GetString(char *String, uint8_t MaxLength)
{
    // 参数检查
    if (String == NULL || MaxLength == 0)
    {
        return 0;
    }

    // 清空目标字符串
    memset(String, 0, MaxLength);

    // 使用通用接收函数获取字符串
    uint8_t length = Serial_Receive(String, MaxLength, SERIAL_TYPE_STRING);

    return length;
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

/**
 * 函    数：串口发送数字
 * 参    数：Number 要发送的数字，范围：0~4294967295
 * 返 回 值：无
 */

uint32_t Serial_GetNumber(void)
{
    // 使用较大的缓冲区接收数字数据，最多支持32字节
    uint8_t dataBuffer[32] = {0};
    uint32_t result = 0;

    // 使用通用接收函数获取数字数据，最多接收32字节
    uint8_t dataIndex = Serial_Receive(dataBuffer, 32, SERIAL_TYPE_NUMBER);

    // 根据实际接收的字节数组合成最终的数字
    // 如果接收超过4字节，只使用前4字节（保持32位兼容性）
    uint8_t bytesToProcess = dataIndex < 4 ? dataIndex : 4;

    for (uint8_t i = 0; i < bytesToProcess; i++)
    {
        result += ((uint32_t)dataBuffer[i]) << (i * 8);
    }

    return result;
}

/**
  * 函    数：串口通用接收函数
  * 参    数：buffer 接收数据缓冲区指针
  * 参    数：maxSize 缓冲区最大大小
  * 参    数：type 接收数据类型（字符串或数字）
  * 返 回 值：实际接收到的数据长度或状态码
  */
uint8_t Serial_Receive(void *buffer, uint8_t maxSize, SerialDataType type)
{
    uint8_t dataIndex = 0;
    uint8_t receivedStartFlag = 0;
    uint8_t receivedEndFlag = 0;
    uint8_t *byteBuffer = (uint8_t *)buffer;

    // 参数检查
    if (buffer == NULL || maxSize == 0)
    {
        return 0;
    }

    // 等待接收完整的数据包起始标志
    while (RxPacketQueue.WriteIndex != RxPacketQueue.ReadIndex && !receivedStartFlag)
    {
        if (!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue);

            if (tempData == 0xFF) // 接收到起始标志
            {
                receivedStartFlag = 1;
                dataIndex = 0; // 重置索引
            }
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
        if (!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue);

            if (tempData == 0xFE) // 接收到结束标志
            {
                receivedEndFlag = 1;
            }
            else
            {
                // 根据数据类型处理接收的数据
                if (type == SERIAL_TYPE_STRING)
                {
                    // 字符串类型：接收字符直到缓冲区满
                    if (dataIndex < maxSize - 1) // 留出一个字节给null终止符
                    {
                        byteBuffer[dataIndex++] = tempData;
                    }
                    else
                    {
                        // 缓冲区已满，继续读取但不存储，直到遇到结束标志
                        continue;
                    }
                }
                else if (type == SERIAL_TYPE_NUMBER)
                {
                    // 数字类型：接收字节直到缓冲区满
                    if (dataIndex < maxSize)
                    {
                        byteBuffer[dataIndex++] = tempData;
                    }
                    else
                    {
                        // 缓冲区已满，继续读取直到遇到结束标志
                        continue;
                    }
                }
            }
        }
    }

    // 后处理根据数据类型
    if (type == SERIAL_TYPE_STRING)
    {
        // 确保字符串以null结尾
        if (dataIndex < maxSize)
        {
            byteBuffer[dataIndex] = '\0';
        }
        else
        {
            byteBuffer[maxSize - 1] = '\0';
        }
        // 返回字符串长度（不包括null终止符）
        return dataIndex;
    }
    else if (type == SERIAL_TYPE_NUMBER)
    {
        // 对于数字类型，返回实际接收的字节数
        return dataIndex;
    }

    return 0;
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
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) // 判断是否是USART1的接收事件触发的中断
    {
        uint8_t RxData = USART_ReceiveData(USART1); // 读取数据寄存器，存放在接收的数据变量
        giveQueueOneData(&RxPacketQueue, RxData);

        USART_ClearITPendingBit(USART1, USART_IT_RXNE); // 清除标志位
    }
}
