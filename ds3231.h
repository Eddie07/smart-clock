
#ifndef __DS3231_H__
#define __DS3231_H__


#define rtc2val(x)	(((x) & 0x0f) + ((x) >> 4) * 10)
#define val2rtc(x)	((((x) / 10) << 4) + (x) % 10)



#define DS3231_REG_SEC    	0x00
#define DS3231_REG_MIN    	0x01
#define DS3231_REG_HOUR    	0x02
#define DS3231_REG_DOW    	0x03
#define DS3231_REG_DAY    	0x04
#define DS3231_REG_MONTH    	0x05
#define DS3231_REG_YEAR    	0x06



#endif
