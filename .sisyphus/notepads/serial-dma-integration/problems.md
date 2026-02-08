# Problems - Serial DMA Integration

## 未解决的问题
（暂无 - 项目刚开始）

## 阻塞问题
（暂无 - 所有任务可开始）

## 技术难题
1. **队列线性化算法**: 需要将环形缓冲区转换为DMA可用的线性内存块
2. **DMA中断处理**: 需要协调DMA TC/HT/TE中断与现有USART中断
3. **内存对齐**: 需要确保DMA缓冲区满足对齐要求
4. **状态同步**: DMA传输状态与队列状态需要同步

## 资源限制
1. **内存**: DMA缓冲区需要额外内存（128+256字节）
2. **DMA通道**: USART1占用DMA1_Channel4和Channel5
3. **中断向量**: 需要添加DMA1_Channel4和Channel5中断处理