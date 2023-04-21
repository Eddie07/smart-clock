
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/rtc.h>


#include "bmp280.h"
#include "ds3231.h"
#include "project1.h"


#define I2C_BUS 		1

#define BMP280_DEVICE_NAME 	"bmp280"
#define BMP280_CHIP_ID		0x60

#define DC3231_DEVICE_NAME 	"ds3231"

#define DS3231_REG_TIME             0x00


/*----------------------------------------------------------------------*/
/* bmp280 part								*/
/*----------------------------------------------------------------------*/


struct bmp280 bmp280;
struct ds3231 ds3231;
//struct tm tm_now;
struct timespec64 curr_tm;
struct rtc_time rtc;


static uint8_t i2cReadByte(struct i2c_client *client,uint8_t command) {
	return i2c_smbus_read_byte_data(client,command);
} 


   

static void bmp280_getRawData(void)
{
    uint8_t pmsb, plsb, pxsb, tmsb, tlsb, txsb;
    plsb = i2cReadByte(bmp280.client,BMP280_REG_PRESSURE_LSB);
    pmsb = i2cReadByte(bmp280.client,BMP280_REG_PRESSURE_MSB);
    pxsb = i2cReadByte(bmp280.client,BMP280_REG_PRESSURE_XLSB);
    tmsb = i2cReadByte(bmp280.client,BMP280_REG_TEMP_MSB);
    tlsb = i2cReadByte(bmp280.client,BMP280_REG_TEMP_LSB);
    txsb = i2cReadByte(bmp280.client,BMP280_REG_TEMP_XLSB);

   bmp280.adc_t = (((( tmsb << 8) | tlsb )<<8) | txsb )>>4;
   bmp280.adc_p = (((( pmsb << 8) | plsb )<<8) | pxsb )>>4;
}



static int bmp280_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  	int ret;
	bmp280.client=client;

  	pr_err("%s: probing ", BMP280_DEVICE_NAME); 
    	ret=i2c_smbus_read_byte_data(bmp280.client, 0xD0);
	if (ret != BMP280_CHIP_ID) return -ENODEV;
	pr_err("%s: chip is 0x%x \n ", BMP280_DEVICE_NAME, ret); 

	/*get calibration */
	
	calib_data.T1 = (i2cReadByte(bmp280.client,BMP280_REG_T1_LSB) | i2cReadByte(bmp280.client,BMP280_REG_T1_MSB) <<8 );
	pr_err("%s:T1 %d \n ", BMP280_DEVICE_NAME, calib_data.T1); 
	pr_err("%s:T1 1%d \n ", BMP280_DEVICE_NAME, i2cReadByte(bmp280.client,BMP280_REG_T1_LSB));
	pr_err("%s:T1 2%d \n ", BMP280_DEVICE_NAME, i2cReadByte(bmp280.client,BMP280_REG_T1_MSB));

	//calib_data.T2 = (s16)((s16)i2c_smbus_read_word_data(bmp280.client,BMP280_REG_T2_LSB));
//pr_err("%s: %d \n ", DEVICE_NAME, calib_data.T2); 
	//calib_data.T3 = (u16)((u16)i2c_smbus_read_word_data(bmp280.client,BMP280_REG_T3_LSB));
//pr_err("%s: %d \n ", DEVICE_NAME, calib_data.T3); 
	//calib_data.T1 = le16_to_cpu((i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_T1_LSB) | i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_T1_MSB)) << 8);
	calib_data.T2 = (i2cReadByte(bmp280.client,BMP280_REG_T2_LSB) | i2cReadByte(bmp280.client,BMP280_REG_T2_MSB) << 8);
	pr_err("%s:T2 %d \n ", BMP280_DEVICE_NAME, calib_data.T2); 
	calib_data.T3 = (i2cReadByte(bmp280.client,BMP280_REG_T3_LSB) | i2cReadByte(bmp280.client,BMP280_REG_T3_MSB) << 8);
	pr_err("%s:T3 %d \n ", BMP280_DEVICE_NAME, calib_data.T3); 
	//dig_p1 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P1_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P1_MSB)) << 8;
	//dig_p2 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P2_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P2_MSB)) << 8;
	//dig_p3 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P3_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P3_MSB)) << 8;
	//dig_p4 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P4_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P4_MSB)) << 8;
	//dig_p5 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P5_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P5_MSB)) << 8;
	//dig_p6 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P6_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P6_MSB)) << 8;
	//dig_p7 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P7_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P7_MSB)) << 8;
	//dig_p8 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P8_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P8_MSB)) << 8;
	//dig_p9 = le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P9_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(bmp280.client,BMP280_REG_P9_MSB)) << 8;

    	i2c_smbus_write_byte_data(bmp280.client,BMP280_REG_CTRL_MEAS, 0x27);
	udelay(500);

 	i2c_smbus_write_byte_data(bmp280.client, BMP280_REG_CONFIG, 0xA0);


    
	bmp280_getRawData();
	pr_err("%s:par.adc_t %d \n ", BMP280_DEVICE_NAME, bmp280.adc_t); 
	uint32_t var1 = ((((bmp280.adc_t >> 3) - ((int32_t)calib_data.T1 << 1))) *
		((int32_t)calib_data.T2)) >> 11;
	uint32_t var2 = (((((bmp280.adc_t >> 4) - ((int32_t)calib_data.T1 )) *
		  ((bmp280.adc_t >> 4) - ((int32_t)calib_data.T1))) >> 12) *
		(calib_data.T3)) >> 14;
	     
	pr_err("%d \n", (((var1 + var2) * 5 + 128) >> 8));

	bmp280.temperature = (((var1 + var2) * 5 + 128) >> 8);
	//var1 = (value / 16384 - calib_data.T1 / 1024) * calib_data.T2;
       // var2 = (value / 131072 - calib_data.T1 / 8192) * (value / 131072 - calib_data.T1 / 8192) * calib_data.T3;
	
	pr_err("%d \n",(var1 + var2) / 5120);
	//return (data->t_fine * 5 + 128) >> 8;
    	//double var1 = (value / 16384.0 - dig_t1 / 1024.0) * dig_t2;
   // /double var2 = (value / 131072.0 - dig_t1 / 8192.0) * (value / 131072.0 - dig_t1 / 8192.0) * dig_t3;
	//t_fine = (int32_t) (var1 + var2);
	//return (var1 + var2) / 5120.0;

	//temp = (s16)((u16)i2c_smbus_read_word_swapped(bmp280.client, BMP280_REG_TEMP_MSB));
	//pr_err("%s: %d  %d  %d \n", SLAVE_DEVICE_NAME, temp, temp2, temp3);
    
    return 0;
}


static int bmp280_remove(struct i2c_client *client) {

  pr_err("%s: removing ", BMP280_DEVICE_NAME);
  	

return 0;
};


/*----------------------------------------------------------------------*/
/* bmp280 part	END							*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* dc3231 part								*/
/*----------------------------------------------------------------------*/


void ds3231_getRtcTime(void) 	{
	
	ds3231.sec=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_SEC));
	ds3231.min=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_MIN));
	ds3231.hour=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_HOUR));
	ds3231.dow=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_DOW));
	ds3231.day=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_DAY));
	ds3231.month=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_MONTH));
	ds3231.year=rtc2val(i2cReadByte(ds3231.client, DS3231_REG_YEAR));
	//pr_err ("%d %d %d %d %d %d %d \n", ds3231.sec, ds3231.min, ds3231.hour,ds3231.dow, ds3231.day, ds3231.month, ds3231.year);	
}

void ds3231_setRtcTime(void) 	{
	
	rtc.tm_sec=37;
	rtc.tm_min=35;
	rtc.tm_hour=11;
	rtc.tm_mday=28;
	rtc.tm_mon=9;
	rtc.tm_year=109;
	//pr_err ("%d %d %d %d %d %d %d \n", ds3231.sec, ds3231.min, ds3231.hour,ds3231.dow, ds3231.day, ds3231.month, ds3231.year);	
}




//static void rtcSetTime(void) {
//	tm_now.tm_sec=ds3231.sec;
//	tm_now.tm_min=ds3231.min;
//	tm_now.tm_hour=ds3231.hour;
//}

static int ds3231_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{  

	struct timespec64 curr_tm_set = {
		.tv_nsec = NSEC_PER_SEC >> 1,
		
	};

	pr_err("%s: probing ", DC3231_DEVICE_NAME);
	ds3231.client=client;

	// ds3231_setRtcTime();
	//rtc_tm_to_ktime(rtc); 
	//rtcSetTime();
	//curr_tm_set.tv_sec=12567;
 	//pr_err("set time of the day %d \n", do_settimeofday64(&curr_tm_set));
	//ktime_set (1256729737,0);
	
	//ktime_get_real_ts64(&curr_tm);
	
	pr_err("get time of the day %llu \n", curr_tm.tv_sec);
	ds3231_getRtcTime();
	

	return 0;
}


static int ds3231_remove(struct i2c_client *client) {

  pr_err("%s: removing ", DC3231_DEVICE_NAME); 

return 0;
};
/*----------------------------------------------------------------------*/
/* dc3231 part	END							*/
/*----------------------------------------------------------------------*/


/* pre-register Device table for bmp280 START*/
static const struct i2c_device_id bmp280_id[] = {
        { BMP280_DEVICE_NAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, bmp280_id);

static const struct of_device_id bmp280_of_match[] = {
        { .compatible = "smart-clock, bmp280" },
        { }
};
MODULE_DEVICE_TABLE(of, bmp280_of_match);


static struct i2c_driver bmp280_driver = {
        .driver = {
            .name   = BMP280_DEVICE_NAME,
            .owner  = THIS_MODULE,
	    .of_match_table = bmp280_of_match, 
        },
           .probe          = bmp280_probe,
          .remove         = bmp280_remove,
          .id_table       = bmp280_id
};

/* pre-register Device table for bmp280 END */


/* pre-register Device table for dc3231 START*/
static const struct i2c_device_id ds3231_id[] = {
        { DC3231_DEVICE_NAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, ds3231_id);

static const struct of_device_id ds3231_of_match[] = {
        { .compatible = "smart-clock, ds3231" },
        { }
};
MODULE_DEVICE_TABLE(of, ds3231_of_match);


static struct i2c_driver ds3231_driver = {
        .driver = {
            .name   = DC3231_DEVICE_NAME,
            .owner  = THIS_MODULE,
	    .of_match_table = ds3231_of_match, 
        },
           .probe          = ds3231_probe,
          .remove         = ds3231_remove,
          .id_table       = ds3231_id
};

/* pre-register Device table for dc3231 END */




int  bmp280_init(void) {

	return i2c_add_driver(&bmp280_driver);
}

int  ds3231_init(void) {

	return i2c_add_driver(&ds3231_driver);
}

//int  mpu6050_init(void) {

//	return i2c_add_driver(&mpu6050_driver);
//}
void  bmp280_deinit(void){
	
	i2c_del_driver(&bmp280_driver);
}

//void  mpu6050_deinit(void){
	
//	i2c_del_driver(&mpu6050_driver);
//}

void  ds3231_deinit(void){
	
	i2c_del_driver(&ds3231_driver);
}




