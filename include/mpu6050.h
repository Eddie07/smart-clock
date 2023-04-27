/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Leds GPIO's driver for RGB leds header
 *
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#ifndef __MPU6050_H__
#define __MPU6050_H__

/* MPU6050 registers */

#define MPU6050_PWR_MGMT_1   = 0x6b
#define MPU6050_SMPLRT_DIV   = 0x19
#define MPU6050_CONFIG       = 0x1a
#define MPU6050_GYRO_CONFIG  = 0x1b
#define MPU6050_INT_ENABLE   = 0x38
#define MPU6050_ACCEL_XOUT_H = 0x3b
#define MPU6050_ACCEL_YOUT_H = 0x3d
#define MPU6050_ACCEL_ZOUT_H = 0x3f
#define MPU6050_GYRO_XOUT_H  = 0x43
#define MPU6050_GYRO_YOUT_H  = 0x45
#define MPU6050_GYRO_ZOUT_H  = 0x47

#endif
