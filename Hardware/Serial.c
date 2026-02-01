#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "Serial.h"

Queue_t TxPacketQueue = {
	.ReadIndex = 0,
	.WirteIndex = 0,
	.DataQueue = {0}
};

Queue_t RxPacketQueue = {
	.ReadIndex = 0,
	.WirteIndex = 0,
	.DataQueue = {0}
};

uint8_t rxQueueIsEmpty(void)
{
	if(RxPacketQueue.WirteIndex == RxPacketQueue.ReadIndex)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void wirteIndexAdd(Queue_t *Queue, uint8_t QueueLength)
{
	if(Queue->WirteIndex < QueueLength - 1)
	{
		Queue->WirteIndex++;
	}
	else if(Queue->WirteIndex == QueueLength - 1)
	{
		Queue->WirteIndex = 0;
	}
}

void readIndexAdd(Queue_t *Queue, uint8_t QueueLength)
{
	if(Queue->ReadIndex < QueueLength - 1)
	{
		Queue->ReadIndex++;
	}
	else if(Queue->ReadIndex == QueueLength - 1)
	{
		Queue->ReadIndex = 0;
	}
}

void giveQueueOneData(Queue_t *Queue, uint8_t QueueLength, uint8_t Data)
{
	Queue->DataQueue[Queue->WirteIndex] = Data;
	wirteIndexAdd(Queue, QueueLength);
}

uint8_t getQueueOneData(Queue_t *Queue, uint8_t QueueLength)
{
	uint8_t Data;
	Data = Queue->DataQueue[Queue->ReadIndex];
	Queue->DataQueue[Queue->ReadIndex] = 0;
	readIndexAdd(Queue, QueueLength);
	return Data;
}

void getQueueAllData(Queue_t *Queue, uint8_t QueueLength, uint8_t *Data)
{
	for(uint8_t i = 0; Queue->WirteIndex != Queue->ReadIndex; i++)
	{
		Data[i] = getQueueOneData(Queue, QueueLength);
	}
}

void sendTxQueueOneDataToSerial(void)
{
	if(TxPacketQueue.WirteIndex != TxPacketQueue.ReadIndex)
	{
		Serial_SendByte(getQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH));
	}
}

void sendTxQueueAllDataToSerial(void)
{
	do
	{
		sendTxQueueOneDataToSerial();
	} while(TxPacketQueue.ReadIndex != TxPacketQueue.WirteIndex);
}

/**
  * 函    数：串口初始化
  * 参    数：无
  * 返 回 值：无
  */
void Serial_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);	//开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA9引脚初始化为复用推挽输出
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA10引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 9600;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART1, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART1
	
	/*中断输出配置*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//选择配置NVIC的USART1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*USART使能*/
	USART_Cmd(USART1, ENABLE);								//使能USART1，串口开始运行
}

/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);		//将字节数据写入数据寄存器，写入后USART自动生成时序波形
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成
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
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}


/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
    uint8_t str_len = strlen(String);
    if(String != 0)
	{
		Serial_SendByte(0xFF);
		for(uint8_t i = 0; i < str_len; i++)
		{
			giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, String[i]);
		}
		sendTxQueueAllDataToSerial();
		Serial_SendByte(0xFE);
	}
}

uint8_t Serial_GetString(char *String, uint8_t MaxLength)
{
    uint8_t dataIndex = 0;
    uint8_t receivedStartFlag = 0;
    uint8_t receivedEndFlag = 0;
    uint8_t length = 0;
    
    // 确保传入的指针有效
    if(String == NULL || MaxLength == 0)
    {
        return 0;
    }
    
    // 清空目标字符串
    memset(String, 0, MaxLength);
    
    // 等待接收完整的数据包
    while(RxPacketQueue.WirteIndex != RxPacketQueue.ReadIndex && !receivedStartFlag)
    {
        // 检查队列中的数据
        if(!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue, RX_PACKET_QUEUE_LENGTH);
            
            if(tempData == 0xFF)  // 接收到起始标志
            {
                receivedStartFlag = 1;
                dataIndex = 0;  // 重置索引
            }
        }
    }
    
    // 接收数据直到结束标志
    while(receivedStartFlag && !receivedEndFlag && dataIndex < MaxLength - 1)
    {
        if(!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue, RX_PACKET_QUEUE_LENGTH);
            
            if(tempData == 0xFE)  // 接收到结束标志
            {
                receivedEndFlag = 1;
            }
            else
            {
                String[dataIndex++] = tempData;  // 存储接收到的字符
            }
        }
    }
    
    String[dataIndex] = '\0';  // 确保字符串以null结尾
    length = dataIndex;
    
    return length;
}

/**
  * 函    数：次方函数（内部使用）
  * 返 回 值：返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//设置结果初值为1
	while (Y --)			//执行Y次
	{
		Result *= X;		//将X累乘到结果
	}
	return Result;
}

/**
  * 函    数：串口发送数字
  * 参    数：Number 要发送的数字，范围：0~4294967295
  * 返 回 值：无
  */
void Serial_SendNumber(uint32_t Number)
{
	Serial_SendByte(0xFF);
	if(Number <= 255)
	{
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, Number & 0xFF);
		
	}
	else if(Number <= 65535)
	{
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, Number & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 8) & 0xFF);
	}
	else if(Number <= 16777215)
	{
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, Number & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 8) & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 16) & 0xFF);
	}
	else if(Number <= 4294967295)
	{
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, Number & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 8) & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 16) & 0xFF);
		giveQueueOneData(&TxPacketQueue, TX_PACKET_QUEUE_LENGTH, (Number >> 24) & 0xFF);
	}
	else
	{
		return;
	}
	sendTxQueueAllDataToSerial();
	Serial_SendByte(0xFE);
}

uint32_t Serial_GetNumber(void)
{
    uint8_t dataBuffer[4] = {0};  // 最多存储4个字节的数据
    uint8_t dataIndex = 0;
    uint8_t receivedStartFlag = 0;
    uint8_t receivedEndFlag = 0;
    uint32_t result = 0;
    
    // 等待接收完整的数据包
    while(RxPacketQueue.WirteIndex != RxPacketQueue.ReadIndex && !receivedStartFlag)
    {
        // 检查队列中的数据
        if(!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue, RX_PACKET_QUEUE_LENGTH);
            
            if(tempData == 0xFF)  // 接收到起始标志
            {
                receivedStartFlag = 1;
                dataIndex = 0;  // 重置索引
            }
        }
    }
    
    // 接收数据直到结束标志
    while(receivedStartFlag && !receivedEndFlag)
    {
        if(!rxQueueIsEmpty())
        {
            uint8_t tempData = getQueueOneData(&RxPacketQueue, RX_PACKET_QUEUE_LENGTH);
            
            if(tempData == 0xFE)  // 接收到结束标志
            {
                receivedEndFlag = 1;
            }
            else if(dataIndex < 4)  // 数据未满4个字节时继续接收
            {
                dataBuffer[dataIndex++] = tempData;
            }
        }
    }
    
    // 根据接收到的数据字节数组合成最终的数字
    for(uint8_t i = 0; i < dataIndex; i++)
    {
        result += ((uint32_t)dataBuffer[i]) << (i * 8);
    }
    
    return result;
}

/**
  * 函    数：使用printf需要重定向的底层函数
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);			//将printf的底层重定向到自己的发送字节函数
	return ch;
}

/**
  * 函    数：自己封装的prinf函数
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Serial_Printf(char *format, ...)
{
	char String[100];				//定义字符数组
	va_list arg;					//定义可变参数列表数据类型的变量arg
	va_start(arg, format);			//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);					//结束变量arg
	Serial_SendString(String);		//串口发送字符数组（字符串）
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
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)		//判断是否是USART1的接收事件触发的中断
	{
		uint8_t RxData = USART_ReceiveData(USART1);				//读取数据寄存器，存放在接收的数据变量
		giveQueueOneData(&RxPacketQueue, RX_PACKET_QUEUE_LENGTH, RxData);
		
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		//清除标志位
	}
}
