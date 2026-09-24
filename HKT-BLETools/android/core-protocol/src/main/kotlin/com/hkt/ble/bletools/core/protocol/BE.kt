package com.hkt.ble.bletools.core.protocol

/** Big-endian 帧内数值编码（协议全大端：固件 setDataPackage 各分支先移 >>8）。跨模块可见（core-ota 组帧复用）。 */
fun u16be(v: Int): ByteArray = byteArrayOf(((v shr 8) and 0xFF).toByte(), (v and 0xFF).toByte())

fun u32be(v: Long): ByteArray = byteArrayOf(
    ((v shr 24) and 0xFF).toByte(),
    ((v shr 16) and 0xFF).toByte(),
    ((v shr 8) and 0xFF).toByte(),
    (v and 0xFF).toByte(),
)
