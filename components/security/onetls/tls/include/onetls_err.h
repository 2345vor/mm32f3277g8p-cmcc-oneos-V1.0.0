#ifndef __ONETLS_ERR_H__
// 错误码定义
enum onetls_errno {
    ONETLS_SUCCESS                  = 0,            // 通用成功
    ONETLS_FAIL                     = 0xffffffff,   // 通用失败
    
    ONETLS_INVALID_PARA             = 0x00001000,   // 通用参数错误
    ONETLS_INVALID_STATE,                           // 非法状态
    ONETLS_TOO_MUCH_EARLY_DATA,                     // early data超过限制
    ONETLS_TOO_MUCH_DATA,                           // 发报文时，超过MTU的报文了
    
    // 内存相关错误码       
    ONETLS_MEM_INVALID_LEN          = 0x00002000,   // 内存长度非法
    ONETLS_MEM_NO_ZONE,                             // 内存空间不足
    
    // PSK相关错误
    ONETLS_NO_PSK_HINT              = 0x00003000,   // 没有配置hint信息
    ONETLS_NO_PSK_KEY,                              // 没有配置key信息
    ONETLS_NO_TICKET,                               // 缺少ticket
    ONETLS_NO_CIPHER_MATCH,                         // 算法没有命中
    ONETLS_NO_KEY_SHARE_MATCH,                      // key_share没有命中
    
    // socket相关错误码
    ONETLS_SOCKET_SEND_FAIL         = 0x00004000,   // socket发送失败
    ONETLS_SOCKET_READ_FAIL,                        // socket接收失败
    ONETLS_SOCKET_TRYAGAIN,                         // socket收发不全，想继续处理
    ONETLS_SOCKET_CLOSE,                            // socket被close了
    ONETLS_SOCKET_ZERO_PACKET,                      // 收到了空报文
    ONETLS_SOCKET_PACKET_LEN,                       // 报文长度不够

    // 报文相关错误码
    ONETLS_EXCEPTION_MSG            = 0x00005000,   // 异常报文
    ONETLS_RECORD_INVALID_LEN,                      // 记录层中的数据长度不合理
    ONETLS_WRONG_VERSION,                           // 不支持的版本    
    ONETLS_WRONG_RANDWOM_NUM,                       // 异常的随机数    
    ONETLS_EPOCH_NOT_IN_WINDOWS,                    // 序列号不在滑窗内
    ONETLS_SEQ_NUM_NOT_IN_WINDOWS,                  // 序列号不在滑窗内
    ONETLS_SEQ_NUM_HITED_AGAIN,                     // 收到了相同的序列号。这也是不对的。
    ONETLS_RECV_ALERT,                              // 收到了告警
    ONETLS_SHOULD_NOT_GET_CID,                      // 不应该有cid
    ONETLS_SHOULD_GET_CID,                          // 应该有cid

    // 扩展相关错误
    ONETLS_EXT_LEN_WRONG            = 0x00006000,   // 长度不正确
    ONETLS_EXT_WRONG_VER,                           // 版本不正确
    ONETLS_EXT_NO_KEY_SHARE,                        // 没有找到keyshare
    ONETLS_EXT_COOKIE_RECV,                         // 收到了cookie
    ONETLS_EXT_UNKNOWN,                             // 不认识的扩展
    ONETLS_EXT_NO_NEED,                             // 不应该存在的
    ONETLS_EXT_REPEAT,                              // 有重复的扩展
};
#endif
