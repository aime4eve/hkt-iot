
/* Includes ------------------------------------------------------------------*/
#include "gps.h"
#include "systick.h"
#include "uart.h"
#include "gpio.h"
#include "control_center.h"
#include "math.h"
#include <ctype.h> /* for isdigit() */
#include "communicate.h"
#include "tim.h"
#include "rtc.h"

#include "LoRaWAN_ATCMD.h"

u8 gps_rmc[100];

static u8 processRMC(char *field[], struct gps_fix_t *session);
struct gps_fix_t gps_l76k;
time_t gps_stamp;

void nmea_add_checksum(char *sentence)
/* add NMEA checksum to a possibly  *-terminated sentence */
{
	unsigned char sum = '\0';
	char c, *p = sentence;

	if (*p == '$' || *p == '!')
	{
		p++;
	}
	while (((c = *p) != '*') && (c != '\0'))
	{
		sum ^= c;
		p++;
	}
	*p++ = '*';
	(void)snprintf(p, 5, "%02X\r\n", (unsigned)sum);
}

void nmea_write(struct gps_fix_t *session, char *buf)
/* ship a command to the GPS, adding * and correct checksum */
{
	(void)strlcpy(session->msgbuf, buf, sizeof(session->msgbuf));
	if (session->msgbuf[0] == '$')
	{
		(void)strlcat(session->msgbuf, "*", sizeof(session->msgbuf));
		nmea_add_checksum(session->msgbuf);
	}
	else
		(void)strlcat(session->msgbuf, "\r\n", sizeof(session->msgbuf));
	session->msgbuflen = strlen(session->msgbuf);
	Gps_SendData((u8 *)session->msgbuf, session->msgbuflen);
}

// Galaxy configuration
void set_gps_galaxy(u8 galaxy)
{
	char vaule[] = {"$PCAS04,3*1A\r\n"};
	vaule[8] = galaxy + 0x30;
	nmea_write(&gps_l76k, vaule);
}

void set_gps_restart(void)
{
	char vaule[] = {"$PCAS10,0*1C\r\n"};
	Gps_SendCMD(vaule);
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromGpsDataHandle(u8 *buffer, u16 size)
{
	int num = 0;
	//$ NMEA 语句的起始字段（Hex 0x24）
	//	if (buffer[0] == 0x24)
	{
		// DEBUG_TRACE(LOG_TAG, "Recv Uart Gps Data: %s", (char *)buffer);
		cmd_split((char *)buffer, "$", splitBuf, &num);
		for (int i = 0; i < num; i++)
		{
			if (strstr(splitBuf[i], "RMC"))
			{
				memcpy(gps_rmc, splitBuf[i], strlen(splitBuf[i]));
				memset(&splitBuf, 0, sizeof(splitBuf));
				cmd_split((char *)gps_rmc, ",", splitBuf, &num);
				processRMC(splitBuf, &gps_l76k);
				break;
			}
		}
	}
}

#define DD(s) ((int)((s)[0] - '0') * 10 + (int)((s)[1] - '0'))

/* sentence supplied ddmmyy, but no century part
 *
 * return: 0 == OK,  greater than zero on failure
 */
static int gps_ddmmyy(char *ddmmyy, struct tm *time)
{
	int yy;
	int mon;
	int mday;
	int year;
	unsigned i; /* NetBSD complains about signed array index */

	if (NULL == ddmmyy)
	{
		return 1;
	}
	for (i = 0; i < 6; i++)
	{
		/* NetBSD 6 wants the cast */
		if (0 == isdigit((int)ddmmyy[i]))
		{
			/* catches NUL and non-digits */
			/* Telit HE910 can set year to "-1" (1999 - 2000) */
			// DEBUG_TRACE(WARN_TAG,
			// 			"gps_ddmmyy(%s), malformed date", ddmmyy);
			return 2;
		}
	}
	/* check for termination */
	if ('\0' != ddmmyy[6])
	{
		/* missing NUL */
		// DEBUG_TRACE(WARN_TAG,
		// 			"gps_ddmmyy(%s), malformed date", ddmmyy);
		return 3;
	}

	/* should be no defects left to segfault DD() */
	yy = DD(ddmmyy + 4);
	mon = DD(ddmmyy + 2);
	mday = DD(ddmmyy);

	year = 2000 + yy;

	utctime.tm_year = year;
	utctime.tm_mon = mon - 1;
	utctime.tm_mday = mday;

	/* 32 bit systems will break in 2038.
	 * Telix fails on GPS rollover to 2099, which 32 bit system
	 * can not handle.  So wrap at 2080.  That way 64 bit systems
	 * work until 2080, and 2099 gets reported as 1999.
	 * since GPS epoch started in 1980, allows for old NMEA to work.
	 */
	if (2080 <= year)
	{
		year -= 100;
	}

	if ((1 > mon) || (12 < mon))
	{
		DEBUG_TRACE(WARN_TAG, "gps_ddmmyy(%s), malformed month", ddmmyy);
		return 4;
	}
	else if ((1 > mday) || (31 < mday))
	{
		DEBUG_TRACE(WARN_TAG, "gps_ddmmyy(%s), malformed day", ddmmyy);
		return 5;
	}
	else
	{
		// DEBUG_TRACE(LOG_TAG, "gps_ddmmyy(%s) sets year %d", ddmmyy, year);
		time->tm_year = year - 1900;
		time->tm_mon = mon - 1;
		time->tm_mday = mday;

		Timestamp = mktime(time);
		stamp_to_date(Timestamp + 8 * 60 * 60, time);
		time->tm_year += 1900;
	}
	// DEBUG_TRACE(LOG_TAG, "gps_ddmmyy(%s) %d-%d-%d, timestamp: %ld", ddmmyy, time->tm_year, time->tm_mon, time->tm_mday, Timestamp);
	return 0;
}

/* update from a UTC time
 *
 * return: 0 == OK,  greater than zero on failure
 */
static int gps_hhmmss(char *hhmmss, struct tm *time)
{
	int i;

	if (NULL == hhmmss)
	{
		return 1;
	}
	for (i = 0; i < 6; i++)
	{
		/* NetBSD 6 wants the cast */
		if (0 == isdigit((int)hhmmss[i]))
		{
			/* catches NUL and non-digits */
			DEBUG_TRACE(WARN_TAG,
						"gps_hhmmss(%s), malformed time", hhmmss);
			return 2;
		}
	}
	/* don't check for termination, might have fractional seconds */

	time->tm_hour = DD(hhmmss);
	time->tm_min = DD(hhmmss + 2);
	time->tm_sec = DD(hhmmss + 4);

	utctime.tm_hour = time->tm_hour;
	utctime.tm_min = time->tm_min;
	utctime.tm_sec = time->tm_sec;

	//	timespec_t gps_time;

	//	gps_time.tv_sec = 0;
	//	if ('.' == hhmmss[6] &&
	//		/* NetBSD 6 wants the cast */
	//		0 != isdigit((int)hhmmss[7]))
	//	{
	//		i = atoi(hhmmss + 7);
	//		sublen = strlen(hhmmss + 7);
	//		gps_time.tv_nsec = (long)i * (long)pow(10.0, 9 - sublen);
	//	}
	//	else
	//	{
	//		gps_time.tv_nsec = 0;
	//	}

	return 0;
}

/* process a pair of latitude/longitude fields starting at field index BEGIN
 * The input fields look like this:
 *     field[0]: 4404.1237962
 *     field[1]: N
 *     field[2]: 12118.8472460
 *     field[3]: W
 * input format of lat/lon is NMEA style  DDDMM.mmmmmmm
 * yes, 7 digits of precision from survey grade GPS
 *
 * return: 0 == OK, non zero is failure.
 */
static int do_lat_lon(char *field[], struct gps_fix_t *out)
{
	double d, m;
	double lon;
	double lat;

	if ('\0' == field[0][0] ||
		'\0' == field[1][0] ||
		'\0' == field[2][0] ||
		'\0' == field[3][0])
	{
		return 1;
	}

	lat = atof(field[0]);
	d = (int)(lat / 100);
	m = lat - d * 100;
	// m = 100.0 * modf(lat / 100.0, &d);
	lat = d + m / 60.0;
	if ('S' == field[1][0])
		lat = -lat;

	lon = atof(field[2]);
	d = (int)(lon / 100);
	m = lon - d * 100;
	// m = 100.0 * modf(lon / 100.0, &d);
	lon = d + m / 60.0;
	if ('W' == field[3][0])
		lon = -lon;

	// if (0 == isfinite(lat) ||
	// 	0 == isfinite(lon))
	// {
	// 	return 2;
	// }

	out->latitude = lat;
	out->longitude = lon;
	return 0;
}

/* Recommend Minimum Course Specific GPS/TRANSIT Data */
static u8 processRMC(char *field[], struct gps_fix_t *session)
{
	/*
	 * RMC,225446.33,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E,A*68
	 * 1     225446.33    Time of fix 22:54:46 UTC
	 * 2     A            Status of Fix:
	 *                     A = Autonomous, valid;
	 *                     D = Differential, valid;
	 *                     V = invalid
	 * 3,4   4916.45,N    Latitude 49 deg. 16.45 min North
	 * 5,6   12311.12,W   Longitude 123 deg. 11.12 min West
	 * 7     000.5        Speed over ground, Knots
	 * 8     054.7        Course Made Good, True north
	 * 9     181194       Date of fix ddmmyy.  18 November 1994
	 * 10,11 020.3,E      Magnetic variation 20.3 deg East
	 * 12    A            FAA mode indicator (NMEA 2.3 and later)
	 *                     see faa_mode() for possible mode values
	 * 13    V            Nav Status (NMEA 4.1 and later)
	 *                     A=autonomous,
	 *                     D=differential,
	 *                     E=Estimated,
	 *                     M=Manual input mode
	 *                     N=not valid,
	 *                     S=Simulator,
	 *                     V = Valid
	 * *68        mandatory nmea_checksum
	 *
	 * SiRF chipsets don't return either Mode Indicator or magnetic variation.
	 */
	char status = field[2][0];
	static time_t stamp;

	switch (status)
	{
	default:
		/* missing */
		/* FALLTHROUGH */
	case 'V':
		/* Invalid */
		session->status = 0;
		break;
	case 'D':
		/* Differential Fix */
		/* FALLTHROUGH */
	case 'A':
		/* Valid Fix */
		/*
		 * The MTK3301, Royaltek RGM-3800, and possibly other
		 * devices deliver bogus time values when the navigation
		 * warning bit is set.
		 */
		if ('\0' != field[1][0] &&
			'\0' != field[9][0])
		{
			struct tm gps_time;
			if (0 == gps_hhmmss(field[1], &gps_time) &&
				0 == gps_ddmmyy(field[9], &gps_time))
			{
				/* got a good data/time */
				systime.tm_hour = gps_time.tm_hour;
				systime.tm_min = gps_time.tm_min;
				systime.tm_sec = gps_time.tm_sec;

				systime.tm_mday = gps_time.tm_mday;
				systime.tm_mon = gps_time.tm_mon;
				systime.tm_year = gps_time.tm_year;

				systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);

				stamp = Timestamp;
				getTimestamp();
				if (Timestamp - stamp > 60 * 60 * 24) //首次成功同步GPS时间
				{
					sensor_stamp = Timestamp + device_t.reportInterval * 60;
					gps_stamp = Timestamp + device_t.gpsLocateInterval * 60; // 默认24小时定位一次
				}
#if HIGH_LEVEL_DEBUG_ENABLE
				DEBUG_TRACE(LOG_TAG, "Update GPS UTC Time %d-%d-%d-%d:%d:%d",
							utctime.tm_year, utctime.tm_mon + 1, utctime.tm_mday, utctime.tm_hour, utctime.tm_min, utctime.tm_sec);
				DEBUG_TRACE(LOG_TAG, "Update SysTime %d-%d-%d-%d:%d:%d",
							systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
#endif
			}
		}
		/* else, no point to the time only case, no regressions with that */

		if (0 == do_lat_lon(&field[3], session))
		{
			/* we have at least a 2D fix */
			/* might cause blinking */
			session->mode = MODE_2D;
			session->status = 1;
		}
		else
		{
			session->mode = MODE_NO_FIX;
		}
		if ('\0' != field[7][0])
		{
			session->speed = atof(field[7]) * KNOTS_TO_MPS;
		}
		if ('\0' != field[8][0])
		{
			session->track = atof(field[8]);
		}

		/* get magnetic variation */
		if ('\0' != field[10][0] &&
			'\0' != field[11][0])
		{
			session->magnetic_var = atof(field[10]);

			switch (field[11][0])
			{
			case 'E':
				/* no change */
				break;
			case 'W':
				session->magnetic_var = -session->magnetic_var;
				break;
			default:
				/* huh? */
				session->magnetic_var = NAN;
				break;
			}
			if (0 == isfinite(session->magnetic_var) ||
				0.09 >= fabs(session->magnetic_var))
			{
				/* some GPS set 0.0,E, or 0,w instead of blank */
				session->magnetic_var = NAN;
			}
		}

		/*
		 * This copes with GPSes like the Magellan EC-10X that *only* emit
		 * GPRMC. In this case we set mode and status here so the client
		 * code that relies on them won't mistakenly believe it has never
		 * received a fix.
		 */
		//		if (0 != isfinite(session->altHAE) ||
		//			0 != isfinite(session->altMSL))
		//		{
		//			/* we probably have at least a 3D fix */
		//			/* this handles old GPS that do not report 3D */
		//			session->mode = MODE_3D;
		//		}
	}

	// DEBUG_TRACE(LOG_TAG, "RMC: ddmmyy=%s hhmmss=%s lat=%f lon=%f speed=%f track=%f mode=%d var=%.1f status=%d",
	// 			field[9], field[1],
	// 			session->latitude,
	// 			session->longitude,
	// 			session->speed,
	// 			session->track,
	// 			session->mode,
	// 			session->magnetic_var,
	// 			session->status);
	return 0;
}

void Gps_Handler(void)
{
	static u32 startTimer;
	static u16 wait_result_count;
	if (startTimer > get_syspant_ms())
		return;
	startTimer = get_syspant_ms() + 500;
	if (gps_stamp <= Timestamp || device_t.getGpsInfo)
	{
		device_t.getGpsInfo = 0;
		gps_stamp = Timestamp + device_t.gpsLocateInterval * 60; // 默认24小时定位一次
		GPS_PWR_CTRL_H;											 //开始GPS模块
		HAL_GPIO_WritePin(GPIOB, GPS_VBACK_Pin, GPIO_PIN_SET);	 //开启热启动
		gps_l76k.latitude = 0;									 //清除历史定位记录
		gps_l76k.longitude = 0;
		wait_result_count = 180; // 90秒
		DEBUG_TRACE(LOG_TAG, "Open Gps Mode Power, Next Convert Timestamp: %d, Now Timestamp: %d", gps_stamp, Timestamp);
	}
	if (wait_result_count)
	{
		wait_result_count--;
		DEBUG_TRACE(LOG_TAG, "Convert Gps Mode Result Cnt: %d, Now Timestamp: %d", wait_result_count, Timestamp);
		if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
			device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
		if (!wait_result_count || (gps_l76k.latitude && gps_l76k.longitude))
		{
			wait_result_count = 0;
			GPS_PWR_CTRL_L;
			// HAL_GPIO_WritePin(GPIOB, GPS_VBACK_Pin, GPIO_PIN_RESET); //关闭热启动
			// gps_stamp = Timestamp +  15;
			gps_stamp = Timestamp + device_t.gpsLocateInterval * 60; // 重设定位时间
			DEBUG_TRACE(LOG_TAG, "Close Gps Mode Power, Next Convert Timestamp: %d, Now Timestamp: %d", gps_stamp, Timestamp);
			device_t.syncGpsState = 1;
		}
	}
}
