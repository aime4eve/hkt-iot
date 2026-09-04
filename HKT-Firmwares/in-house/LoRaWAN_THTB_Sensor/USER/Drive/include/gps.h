
#ifndef __GPS_H__
#define __GPS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "time.h"

#define ONLINE_SET (1llu << 1)
#define TIME_SET (1llu << 2)
#define TIMERR_SET (1llu << 3)
#define LATLON_SET (1llu << 4)
#define ALTITUDE_SET (1llu << 5)
#define SPEED_SET (1llu << 6)
#define TRACK_SET (1llu << 7)
#define CLIMB_SET (1llu << 8)
#define STATUS_SET (1llu << 9)
#define MODE_SET (1llu << 10)
#define DOP_SET (1llu << 11)
#define HERR_SET (1llu << 12)
#define VERR_SET (1llu << 13)
#define ATTITUDE_SET (1llu << 14)
#define SATELLITE_SET (1llu << 15)
#define SPEEDERR_SET (1llu << 16)
#define TRACKERR_SET (1llu << 17)
#define CLIMBERR_SET (1llu << 18)
#define DEVICE_SET (1llu << 19)
#define DEVICELIST_SET (1llu << 20)
#define DEVICEID_SET (1llu << 21)
#define RTCM2_SET (1llu << 22)
#define RTCM3_SET (1llu << 23)
#define AIS_SET (1llu << 24)
#define PACKET_SET (1llu << 25)
#define SUBFRAME_SET (1llu << 26)
#define GST_SET (1llu << 27)
#define VERSION_SET (1llu << 28)
#define POLICY_SET (1llu << 29)
#define LOGMESSAGE_SET (1llu << 30)
#define ERROR_SET (1llu << 31)
#define TOFF_SET (1llu << 32) /* not yet used */
#define PPS_SET (1llu << 33)
#define NAVDATA_SET (1llu << 34)
#define OSCILLATOR_SET (1llu << 35)
#define ECEF_SET (1llu << 36)
#define VECEF_SET (1llu << 37)
#define MAGNETIC_TRACK_SET (1llu << 38)
#define RAW_SET (1llu << 39)
#define NED_SET (1llu << 40)
#define VNED_SET (1llu << 41)

#define STATUS_NO_FIX 0 /* no */
/* yes, plain GPS (SPS Mode), without DGPS, PPS, RTK, DR, etc. */
#define STATUS_FIX 1
#define STATUS_DGPS_FIX 2 /* yes, with DGPS */
#define STATUS_RTK_FIX 3  /* yes, with RTK Fixed */
#define STATUS_RTK_FLT 4  /* yes, with RTK Float */
#define STATUS_DR 5       /* yes, with dead reckoning */
#define STATUS_GNSSDR 6   /* yes, with GNSS + dead reckoning */
#define STATUS_TIME 7     /* yes, time only (surveyed in, manual) */
#define STATUS_SIM 8      /* yes, simulated */
/* yes, Precise Positioning Service (PPS)
 * Not to be confused with Pulse per Second (PPS)
 * PPS is the encrypted military P(Y)-code */
#define STATUS_PPS_FIX 9

#define MODE_NOT_SEEN 0 /* mode update not seen yet */
#define MODE_NO_FIX 1   /* none */
#define MODE_2D 2       /* good for latitude/longitude */
#define MODE_3D 3       /* good for altitude/climb too */

/* some multipliers for interpreting GPS output */
#define METERS_TO_FEET (1 / 0.3048) /* Meters to International Foot */
/* Note: not the same as the USA Survey Foot: (3937 / 1200)
 * Some states use the International Foot, not the USA Survey Foot */
#define METERS_TO_MILES 0.00062137119 /* Meters to miles */
#define METERS_TO_FATHOMS 0.54680665  /* Meters to fathoms */
#define KNOTS_TO_MPH 1.1507794        /* Knots to miles per hour */
#define KNOTS_TO_KPH 1.852            /* Knots to kilometers per hour */
#define KNOTS_TO_MPS 0.51444444       /* Knots to meters per second */
#define MPS_TO_KPH 3.6                /* Meters per second to klicks/hr */
#define MPS_TO_MPH 2.2369363          /* Meters/second to miles per hour */
#define MPS_TO_KNOTS 1.9438445        /* Meters per second to knots */
/* miles and knots are both the international standard versions of the units */

/* angle conversion multipliers */
#define GPS_PI 3.1415926535897932384626433832795029
#define RAD_2_DEG 57.2957795130823208767981548141051703
#define DEG_2_RAD 0.0174532925199432957692369076848861271

/* other mathematical constants */
#define GPS_LN2 0.693147180559945309417232121458176568

    //#define NAN (0.0f / 0.0f)

    enum GALAXY_CONFIG
    {
        GALAXY_CONFIG_GPS = 1,
        GALAXY_CONFIG_BEIDOU,
        GALAXY_CONFIG_GPS_BEIDOU,
        GALAXY_CONFIG_GLONASS,
        GALAXY_CONFIG_GPS_GLONASS,
        GALAXY_CONFIG_BEIDOU_GLONASS,
        GALAXY_CONFIG_GPS_BEIDOU_GLONASS,
    };

    struct timespec
    {
        time_t tv_sec;
        long tv_nsec;
    };

    typedef struct timespec timespec_t; /* Unix time as sec, nsec */

    /* GPS error estimates are all over the map, and often unspecified.
     * try for 1-sigma if we can... */
    struct gps_fix_t
    {
        int mode; /* Mode of fix */
        int status;
        double latitude;       /* Latitude in degrees (valid if mode >= 2) */
        double longitude;      /* Longitude in degrees (valid if mode >= 2) */
        double track;          /* Course made good (relative to true north) */
        double speed;          /* Speed over ground, meters/sec */
        double magnetic_track; /* Course (relative to Magnetic North) */
        double magnetic_var;   /* magnetic variation in degrees */
        char msgbuf[100];
        int msgbuflen;
    };

    extern struct gps_fix_t gps_l76k;
    extern time_t gps_stamp;
    
    void fromGpsDataHandle(u8 *buffer, u16 size);
    void set_gps_restart(void);
    void set_gps_galaxy(u8 galaxy);
    void Gps_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
