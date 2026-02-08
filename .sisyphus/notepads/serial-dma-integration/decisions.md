# Decisions - Serial DMA Integration

## 架构决策
1. **DMA管理器设计**: 独立MyDMA模块，与串口驱动解耦
2. **集成方式**: 扩展Serial_t结构体，添加可选DMA字段
3. **缓冲区策略**: DMA直接线性访问队列缓冲区（处理回绕）
4. **配置方式**: 编译时宏定义（USE_DMA），无运行时配置
5. **错误处理**: 基本错误处理，DMA错误时重置通道

## 技术决策
1. **DMA通道分配**:
   - USART1_TX: DMA1_Channel4
   - USART1_RX: DMA1_Channel5
2. **DMA模式**:
   - TX: DMA_Mode_Normal（正常模式）
   - RX: DMA_Mode_Circular（循环缓冲区模式）
3. **缓冲区大小**:
   - TX缓冲区: 128字节
   - RX缓冲区: 256字节（循环）
4. **中断优先级**: DMA中断优先级高于USART中断（确保数据一致性）
5. **队列同步**: 线性化算法处理环形缓冲区回绕

## 兼容性决策
1. **API不变性**: 保持Serial_Send/Serial_UnpackReceive函数签名不变
2. **向后兼容**: 现有应用程序无需修改即可工作
3. **可选功能**: DMA功能可通过USE_DMA宏启用/禁用
4. **多实例支持**: 设计支持USART2/3，但仅实现USART1