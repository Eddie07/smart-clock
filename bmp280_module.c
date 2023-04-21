
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define I2C_BUS 		1
#define SLAVE_DEVICE_NAME 	"BMP280"
#define SLAVE_DEVICE_ADDRESS	0x76
#define CHIP_ID			0x60

#define BMP280_REG_CONFIG	0xF5
#define BMP280_REG_PRESSURE_MSB              0xF7  /*Pressure MSB Register */
#define BMP280_REG_PRESSURE_LSB              0xF8  /*Pressure LSB Register */
#define BMP280_REG_PRESSURE_XLSB             0xF9  /*Pressure XLSB Register */
#define BMP280_REG_TEMP_MSB           0xFA  /*Temperature MSB Reg */
#define BMP280_REG_TEMP_LSB           0xFB  /*Temperature LSB Reg */
#define BMP280_REG_TEMP_XLSB          0xFC  /*Temperature XLSB Reg */

#define BMP280_REG_CTRL_MEAS		0xF4

/*
 * constants for setPowerMode()
 */
#define BMP280_SLEEP_MODE 0x00
#define BMP280_FORCED_MODE 0x01
#define BMP280_NORMAL_MODE 0x03

/* BMP180 and BMP280 common registers */
#define BMP280_REG_CTRL_MEAS		0xF4
#define BMP280_REG_RESET		0xE0
#define BMP280_REG_ID			0xD0

/* callibration registers */
#define BMP280_REG_T1_LSB 0x88
#define BMP280_REG_T1_MSB 0x89
#define BMP280_REG_T2_LSB 0x8A
#define BMP280_REG_T2_MSB 0x8B
#define BMP280_REG_T3_LSB 0x8C
#define BMP280_REG_T3_MSB 0x8D
#define BMP280_REG_P1_LSB 0x8E
#define BMP280_REG_P1_MSB 0x8F
#define BMP280_REG_P2_LSB 0x90
#define BMP280_REG_P2_MSB 0x91
#define BMP280_REG_P3_LSB 0x92
#define BMP280_REG_P3_MSB 0x93
#define BMP280_REG_P4_LSB 0x94
#define BMP280_REG_P4_MSB 0x95
#define BMP280_REG_P5_LSB 0x96
#define BMP280_REG_P5_MSB 0x97
#define BMP280_REG_P6_LSB 0x98
#define BMP280_REG_P6_MSB 0x99
#define BMP280_REG_P7_LSB 0x9A
#define BMP280_REG_P7_MSB 0x9B
#define BMP280_REG_P8_LSB 0x9C
#define BMP280_REG_P8_MSB 0x9D
#define BMP280_REG_P9_LSB 0x9E
#define BMP280_REG_P9_MSB 0x9F

struct BMP280CalibrationData {
  uint16_t T1;
  int16_t T2;
  int16_t T3;
  uint16_t P1;
  int16_t P2;
  int16_t P3;
  int16_t P4;
  int16_t P5;
  int16_t P6;
  int16_t P7;
  int16_t P8;
  int16_t P9;
} calib_data;

	uint8_t pmsb, plsb, pxsb;
    	uint8_t tmsb, tlsb, txsb;

static uint32_t temperature, pressure;

static void getRawData(struct i2c_client *client) {
    plsb = i2c_smbus_read_byte_data(client,BMP280_REG_PRESSURE_LSB);
    pmsb = i2c_smbus_read_byte_data(client,BMP280_REG_PRESSURE_MSB);
    pxsb = i2c_smbus_read_byte_data(client,BMP280_REG_PRESSURE_XLSB);

    tmsb = i2c_smbus_read_byte_data(client,BMP280_REG_TEMP_MSB);
    tlsb = i2c_smbus_read_byte_data(client,BMP280_REG_TEMP_LSB);
    txsb = i2c_smbus_read_byte_data(client,BMP280_REG_TEMP_XLSB);

    temperature = 0;
    temperature = (temperature | tmsb) << 8;
    temperature = (temperature | tlsb) << 8;
    temperature = (temperature | txsb) >> 4;

    pressure = 0;
    pressure = (pressure | pmsb) << 8;
    pressure = (pressure | plsb) << 8;
    pressure = (pressure | pxsb) >> 4;
}



static struct i2c_adapter *bmp_i2c_adapter;  	/* Adapter to I2c Bus */
static struct i2c_client *bmp_i2c_client;	/* Our i2c device */	

struct bmp280_data {
	struct i2c_client *client;
	int pressure;
	//int humidity;
	//int temperature;
};



/* I2c driver registration */

static const struct i2c_device_id bmp_id[] = {
        { SLAVE_DEVICE_NAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, bmp_id);
/*
static const struct of_device_id bmp_of_match[] = {
        { .compatible = "bosch, bmp280" },
        { }
};
MODULE_DEVICE_TABLE(of, bmp_of_match);
*/
static int bmp_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  	int ret;
	uint32_t msb, lsb, xlsb;
	uint8_t reg;
	uint16_t dig_t1,dig_t2,dig_t3,dig_p1,dig_p2,dig_p3,dig_p4,dig_p5,dig_p6,dig_p7,dig_p8,dig_p9;
	uint32_t t_fine;
	struct bmp280_data *data;
  	pr_err("%s: probing ", SLAVE_DEVICE_NAME); 
    	ret=i2c_smbus_read_byte_data(bmp_i2c_client, 0xD0);
	if (ret != CHIP_ID) return -ENODEV;
	pr_err("%s: chip is 0x%x \n ", SLAVE_DEVICE_NAME, ret); 

	
	/*reset*/
	i2c_smbus_write_byte_data(client,BMP280_REG_RESET, 0xB6);
    	udelay(200); //let complete reset
	udelay(435);
	/*get calibration */
	calib_data.T1 = (u16)((u16)i2c_smbus_read_word_data(client,BMP280_REG_T1_LSB));
	calib_data.T2 = (s16)((s16)i2c_smbus_read_word_data(client,BMP280_REG_T2_LSB));
	calib_data.T3 = (u16)((u16)i2c_smbus_read_word_data(client,BMP280_REG_T3_LSB));
	//calib_data.T1 = le16_to_cpu((i2c_smbus_read_byte_data(client,BMP280_REG_T1_LSB) | i2c_smbus_read_byte_data(client,BMP280_REG_T1_MSB)) << 8);
	//calib_data.T2 = le16_to_cpu((i2c_smbus_read_byte_data(client,BMP280_REG_T2_LSB) | i2c_smbus_read_byte_data(client,BMP280_REG_T2_MSB)) << 8);
	//calib_data.T3 = le16_to_cpu((i2c_smbus_read_byte_data(client,BMP280_REG_T3_LSB) | i2c_smbus_read_byte_data(client,BMP280_REG_T3_MSB)) << 8);
	dig_p1 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P1_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P1_MSB)) << 8;
	dig_p2 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P2_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P2_MSB)) << 8;
	dig_p3 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P3_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P3_MSB)) << 8;
	dig_p4 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P4_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P4_MSB)) << 8;
	dig_p5 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P5_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P5_MSB)) << 8;
	dig_p6 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P6_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P6_MSB)) << 8;
	dig_p7 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P7_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P7_MSB)) << 8;
	dig_p8 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P8_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P8_MSB)) << 8;
	dig_p9 = le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P9_LSB)) | le16_to_cpu(i2c_smbus_read_byte_data(client,BMP280_REG_P9_MSB)) << 8;
        /*write regs*/
	//i2c_smbus_write_byte_data(client, BMP280_REG_CONFIG, 0x10);
	//i2c_smbus_write_byte_data(client, BMP280_REG_CTRL_MEAS, 0x57);
 	//udelay(435); //sleep 43.2 ms, maximum time for a measurement, to take a first measurement
	
	/*set powermode*/
	reg = i2c_smbus_read_byte_data (client, BMP280_REG_CTRL_MEAS);
    	reg &= ~3; //clear bits
    	reg |= BMP280_NORMAL_MODE & 3; //set bits
    	i2c_smbus_write_byte_data(client,BMP280_REG_CTRL_MEAS, reg);
	udelay(435);
       // i2c_smbus_write_byte_data(client,BMP280_REG_RESET, 0xB6);
    	//udelay(200); //let complete reset  
 

	//reg = i2c_smbus_read_byte_data (client, BMP280_REG_CTRL_MEAS);
    	//reg &= ~3; //clear bits
    	//reg |= BMP280_NORMAL_MODE & 3; //set bits
    	//i2c_smbus_write_byte_data(client,BMP280_REG_CTRL_MEAS, reg);

    
	data->client = client;
	getRawData(client);
	
	uint32_t var1 = ((((temperature >> 3) - ((int32_t)calib_data.T1 << 1))) *
		((int32_t)calib_data.T2)) >> 11;
	uint32_t var2 = (((((temperature >> 4) - ((int32_t)calib_data.T1 )) *
		  ((temperature >> 4) - ((int32_t)calib_data.T1))) >> 12) *
		(calib_data.T3)) >> 14;
	     
	pr_err("%d \n", (((var1 + var2) * 5 + 128) >> 8));

	//var1 = (value / 16384 - calib_data.T1 / 1024) * calib_data.T2;
       // var2 = (value / 131072 - calib_data.T1 / 8192) * (value / 131072 - calib_data.T1 / 8192) * calib_data.T3;
	
	pr_err("%d \n",(var1 + var2) / 5120);
	//return (data->t_fine * 5 + 128) >> 8;
    	//double var1 = (value / 16384.0 - dig_t1 / 1024.0) * dig_t2;
   // /double var2 = (value / 131072.0 - dig_t1 / 8192.0) * (value / 131072.0 - dig_t1 / 8192.0) * dig_t3;
	//t_fine = (int32_t) (var1 + var2);
	//return (var1 + var2) / 5120.0;

	//temp = (s16)((u16)i2c_smbus_read_word_swapped(client, BMP280_REG_TEMP_MSB));
	//pr_err("%s: %d  %d  %d \n", SLAVE_DEVICE_NAME, temp, temp2, temp3);
    
    return 0;
}

static int bmp_remove(struct i2c_client *client) {

  pr_err("%s: removing ", SLAVE_DEVICE_NAME); 

return 0;};



static struct i2c_driver bmp_driver = {
        .driver = {
            .name   = SLAVE_DEVICE_NAME,
            .owner  = THIS_MODULE,
	   // .of_match_table = bmp_of_match, 
        },
           .probe          = bmp_probe,
          .remove         = bmp_remove,
          .id_table       = bmp_id
};

/* registration end*/


static struct i2c_board_info bmp_board_info= {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, SLAVE_DEVICE_ADDRESS)
};


static int __init init_i2c_module(void) {
	uint8_t  ret,id;
	bmp_i2c_adapter=i2c_get_adapter(I2C_BUS);
	if( bmp_i2c_adapter == NULL ) {
		pr_err("%s: failed to create adapter ", SLAVE_DEVICE_NAME); 
		return -1; }
         pr_err("%s: adapter create ", SLAVE_DEVICE_NAME); 

	bmp_i2c_client=i2c_new_client_device(bmp_i2c_adapter, &bmp_board_info);
	if( bmp_i2c_client == NULL ) {
		pr_err("%s: failed to create client ", SLAVE_DEVICE_NAME); 
	}
        pr_err("%s: client create \n", SLAVE_DEVICE_NAME); 

	i2c_put_adapter(bmp_i2c_adapter);
	return i2c_add_driver(&bmp_driver);
	//pr_err("%s: print final %d \n", SLAVE_DEVICE_NAME);
	//
 

}

static void __exit exit_i2c_module(void){
	i2c_unregister_device(bmp_i2c_client);
	i2c_del_driver(&bmp_driver);
}


module_init (init_i2c_module);
module_exit (exit_i2c_module);

MODULE_LICENSE("GPL");





