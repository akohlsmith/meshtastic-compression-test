#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#include <pb_decode.h>
#include "meshtastic/mqtt.pb.h"
#include "meshtastic/mesh.pb.h"

#include "arithcode.h"

static bool debug, dump, verbose;

struct user_context {
	const char *topic;
};

static const char *_portnum_str(meshtastic_port_num_t portnum)
{
	const char *s;

	switch (portnum) {
	case MESHTASTIC_PORT_NUM_UNKNOWN_APP:		s = "UNKNOWN_APP"; break;
	case MESHTASTIC_PORT_NUM_TEXT_MESSAGE_APP:	s = "TEXT_MESSAGE_APP"; break;
	case MESHTASTIC_PORT_NUM_REMOTE_HARDWARE_APP:	s = "REMOTE_HARDWARE_APP"; break;
	case MESHTASTIC_PORT_NUM_POSITION_APP:		s = "POSITION_APP"; break;
	case MESHTASTIC_PORT_NUM_NODEINFO_APP:		s = "NODEINFO_APP"; break;
	case MESHTASTIC_PORT_NUM_ROUTING_APP:		s = "ROUTING_APP"; break;
	case MESHTASTIC_PORT_NUM_ADMIN_APP:		s = "ADMIN_APP"; break;
	case MESHTASTIC_PORT_NUM_TEXT_MESSAGE_COMPRESSED_APP:	s = "TEXT_MESSAGE_COMPRESSED_APP"; break;
	case MESHTASTIC_PORT_NUM_WAYPOINT_APP:		s = "WAYPOINT_APP"; break;
	case MESHTASTIC_PORT_NUM_AUDIO_APP:		s = "AUDIO_APP"; break;
	case MESHTASTIC_PORT_NUM_DETECTION_SENSOR_APP:	s = "DETECTION_SENSOR_APP"; break;
	case MESHTASTIC_PORT_NUM_REPLY_APP:		s = "REPLY_APP"; break;
	case MESHTASTIC_PORT_NUM_IP_TUNNEL_APP:		s = "IP_TUNNEL_APP"; break;
	case MESHTASTIC_PORT_NUM_PAXCOUNTER_APP:	s = "PAXCOUNTER_APP"; break;
	case MESHTASTIC_PORT_NUM_SERIAL_APP:		s = "SERIAL_APP"; break;
	case MESHTASTIC_PORT_NUM_STORE_FORWARD_APP:	s = "STORE_FORWARD_APP"; break;
	case MESHTASTIC_PORT_NUM_RANGE_TEST_APP:	s = "RANGE_TEST_APP"; break;
	case MESHTASTIC_PORT_NUM_TELEMETRY_APP:		s = "TELEMETRY_APP"; break;
	case MESHTASTIC_PORT_NUM_ZPS_APP:		s = "ZPS_APP"; break;
	case MESHTASTIC_PORT_NUM_SIMULATOR_APP:		s = "SIMULATOR_APP"; break;
	case MESHTASTIC_PORT_NUM_TRACEROUTE_APP:	s = "TRACEROUTE_APP"; break;
	case MESHTASTIC_PORT_NUM_NEIGHBORINFO_APP:	s = "NEIGHBORINFO_APP"; break;
	case MESHTASTIC_PORT_NUM_ATAK_PLUGIN:		s = "ATAK_PLUGIN"; break;
	case MESHTASTIC_PORT_NUM_MAP_REPORT_APP:	s = "MAP_REPORT_APP"; break;
	case MESHTASTIC_PORT_NUM_POWERSTRESS_APP:	s = "POWERSTRESS_APP"; break;
	case MESHTASTIC_PORT_NUM_PRIVATE_APP:		s = "PRIVATE_APP"; break;
	case MESHTASTIC_PORT_NUM_ATAK_FORWARDER:	s = "ATAK_FORWARDER"; break;
	default: s = "(unknown)"; break;
	};

	return s;
}


static char *time_str(uint32_t seconds)
{
	static char s[80];
	char ss[80];
	uint32_t v;

	s[0] = 0x00;

	v = seconds / 86400;
	seconds %= 86400;
	if (v > 0) { sprintf(ss, "%d day%s, ", v, (v > 1) ? "s" : ""); strcat(s, ss); }

	v = seconds / 3600;
	seconds %= 3600;
	if (v > 0) { sprintf(ss, "%d hour%s, ", v, (v > 1) ? "s" : ""); strcat(s, ss); }

	v = seconds / 60;
	seconds %= 60;
	if (v > 0) { sprintf(ss, "%d minute%s, ", v, (v > 1) ? "s" : ""); strcat(s, ss); }

	sprintf(ss, "%d seconds", v);
	strcat(s, ss);
	return s;
}


static int dump_nodeinfo(pb_istream_t *s)
{
	bool pb_ret;
	int ret;
	meshtastic_user_t u;

	printf("  Decoding nodeinfo:\n");
	if ((pb_ret = pb_decode(s, MESHTASTIC_USER_FIELDS, &u))) {
		printf("    hw %d, id: %s - %s - \"%s\"\n", u.hw_model, u.id, u.short_name, u.long_name);
		ret = 0;

	} else {
		printf("    (decoding failed: %s)\n", PB_GET_ERROR(s));
		ret = -1;
	}

	return ret;
}


static int dump_device_metrics(meshtastic_device_metrics_t *dm)
{
	printf("  device metrics:\n");
	if (dm->has_uptime_seconds)		{ printf("    uptime: %d (%s)\n", dm->uptime_seconds, time_str(dm->uptime_seconds)); }
	if (dm->has_battery_level)		{ printf("    battery level: %d\n", dm->battery_level); }
	if (dm->has_voltage)			{ printf("    voltage: %.2f\n", dm->voltage); }
	if (dm->has_channel_utilization)	{ printf("    ch. util: %.2f\n", dm->channel_utilization); }
	if (dm->has_air_util_tx)		{ printf("    air util: %.2f\n", dm->air_util_tx); }
	return 0;
}


static int dump_environment_metrics(meshtastic_environment_metrics_t *em)
{
	printf("  environment metrics:\n");
	if (em->has_temperature)		{ printf("    temperature: %.1f\n", em->temperature); }
	if (em->has_relative_humidity)		{ printf("    humidity: %.1f\n", em->relative_humidity); }
	if (em->has_barometric_pressure)	{ printf("    barometric pressure: %.1f\n", em->barometric_pressure); }
	if (em->has_wind_direction)		{ printf("    wind bearing: %hd\n", em->wind_direction); }
	if (em->has_wind_speed)			{ printf("    wind speed: %.1f\n", em->wind_speed); }
	if (em->has_wind_gust)			{ printf("    wind gust: %.1f\n", em->wind_gust); }
	if (em->has_wind_lull)			{ printf("    wind lull: %.1f\n", em->wind_lull); }
	if (em->has_gas_resistance)		{ printf("    gas resistance: %.1f\n", em->gas_resistance); }
	if (em->has_voltage)			{ printf("    voltage: %.2f\n", em->voltage); }
	if (em->has_current)			{ printf("    current: %.2f\n", em->current); }
	if (em->has_iaq)			{ printf("    IAQ: %hd\n", em->iaq); }
	if (em->has_distance)			{ printf("    distance: %.1f\n", em->distance); }
	if (em->has_lux)			{ printf("    LUX: %.1f\n", em->lux); }
	if (em->has_white_lux)			{ printf("    LUX (white): %.1f\n", em->white_lux); }
	if (em->has_ir_lux)			{ printf("    LUX (IR): %.1f\n", em->ir_lux); }
	if (em->has_uv_lux)			{ printf("    LUX (UV): %.1f\n", em->uv_lux); }
	if (em->has_weight)			{ printf("    weight: %.1f\n", em->weight); }
	return 0;
}


static int dump_airquality_metrics(meshtastic_air_quality_metrics_t *aqm)
{
	printf("  air quality metrics:\n");
	if (aqm->has_pm10_standard)		{ printf("    PM10 std: %d\n", aqm->pm10_standard); }
	if (aqm->has_pm25_standard)		{ printf("    PM25 std: %d\n", aqm->pm25_standard); }
	if (aqm->has_pm100_standard)		{ printf("    PM100 std: %d\n", aqm->pm100_standard); }
	if (aqm->has_pm10_environmental)	{ printf("    PM10 env: %d\n", aqm->pm10_environmental); }
	if (aqm->has_pm25_environmental)	{ printf("    PM25 env: %d\n", aqm->pm25_environmental); }
	if (aqm->has_pm100_environmental)	{ printf("    PM100 env: %d\n", aqm->pm100_environmental); }
	if (aqm->has_particles_03um)		{ printf("    3um particles: %d\n", aqm->particles_03um); }
	if (aqm->has_particles_05um)		{ printf("    5um particles: %d\n", aqm->particles_05um); }
	if (aqm->has_particles_10um)		{ printf("    10um particles: %d\n", aqm->particles_10um); }
	if (aqm->has_particles_25um)		{ printf("    25um particles: %d\n", aqm->particles_25um); }
	if (aqm->has_particles_50um)		{ printf("    50um particles: %d\n", aqm->particles_50um); }
	if (aqm->has_particles_100um)		{ printf("    100um particles: %d\n", aqm->particles_100um); }
	if (aqm->has_co2)			{ printf("    CO2: %d\n", aqm->co2); }
	return 0;
}


static int dump_power_metrics(meshtastic_power_metrics_t *pm)
{
	printf("  power metrics:\n");
	if (pm->has_ch1_voltage)	{ printf("    CH1 V: %.2f\n", pm->ch1_voltage); }
	if (pm->has_ch1_current)	{ printf("    CH1 mA: %.2f\n", pm->ch1_current); }
	if (pm->has_ch2_voltage)	{ printf("    CH2 V: %.2f\n", pm->ch2_voltage); }
	if (pm->has_ch2_current)	{ printf("    CH2 mA: %.2f\n", pm->ch2_current); }
	if (pm->has_ch3_voltage)	{ printf("    CH3 V: %.2f\n", pm->ch3_voltage); }
	if (pm->has_ch3_current)	{ printf("    CH3 mA: %.2f\n", pm->ch3_current); }
	return 0;
}


static int dump_localstats_metrics(meshtastic_local_stats_t *t) { return 0; }
static int dump_health_metrics(meshtastic_health_metrics_t *t) { return 0; }


static int dump_telemetry(pb_istream_t *s)
{
	bool pb_ret;
	int ret;
	meshtastic_telemetry_t t = MESHTASTIC_TELEMETRY_INIT_DEFAULT;

	printf("  Decoding telemetry:\n");
	if ((pb_ret = pb_decode(s, MESHTASTIC_TELEMETRY_FIELDS, &t))) {
		switch (t.which_variant) {
		case MESHTASTIC_TELEMETRY_DEVICE_METRICS_TAG:
			return dump_device_metrics(&t.variant.device_metrics);
			break;

		case MESHTASTIC_TELEMETRY_ENVIRONMENT_METRICS_TAG:
			return dump_environment_metrics(&t.variant.environment_metrics);
			break;

		case MESHTASTIC_TELEMETRY_AIR_QUALITY_METRICS_TAG:
			return dump_airquality_metrics(&t.variant.air_quality_metrics);
			break;

		case MESHTASTIC_TELEMETRY_POWER_METRICS_TAG:
			return dump_power_metrics(&t.variant.power_metrics);
			break;

		case MESHTASTIC_TELEMETRY_LOCAL_STATS_TAG:
			return dump_localstats_metrics(&t.variant.local_stats);
			break;

		case MESHTASTIC_TELEMETRY_HEALTH_METRICS_TAG:
			return dump_health_metrics(&t.variant.health_metrics);
			break;

		default:
			printf("    (unknown variant tag %d)\n", t.which_variant);
			break;
		};

		ret = 0;

	} else {
		printf("    (decoding failed: %s)\n", PB_GET_ERROR(s));
		ret = -1;
	}

	return ret;
}


static int dump_position(pb_istream_t *s)
{
	bool pb_ret;
	int ret;
	meshtastic_position_t p = MESHTASTIC_POSITION_INIT_DEFAULT;

	printf("  Decoding position:\n");
	if ((pb_ret = pb_decode(s, MESHTASTIC_POSITION_FIELDS, &p))) {
		if (p.has_latitude_i)  { printf("  lat: %.3f\n", 1e-7f * p.latitude_i); }
		if (p.has_longitude_i) { printf("  lon: %.3f\n", 1e-7f * p.longitude_i); }
		if (p.has_altitude)    { printf("  alt: %d\n", p.altitude); }
		printf("    precision: %d\n", p.precision_bits);

		if (p.sats_in_view > 0) {
			printf("    fix type: %d\n", p.fix_type);
			printf("    fix quality: %d\n", p.fix_quality);
			printf("    satellites: %d\n", p.sats_in_view);
		}

		ret = 0;

	} else {
		printf("    (decoding failed: %s)\n", PB_GET_ERROR(s));
		ret = -1;
	}

	return ret;
}


static int dump_text(void *buf, int len)
{
	printf("  text: \"%*s\"\n", len, (char *)buf);
	return 0;
}


static int dump_traceroute(pb_istream_t *s)
{
	bool pb_ret;
	int ret;
	meshtastic_route_discovery_t rd = MESHTASTIC_ROUTE_DISCOVERY_INIT_DEFAULT;

	printf("  Decoding route discovery:\n");
	if ((pb_ret = pb_decode(s, MESHTASTIC_ROUTE_DISCOVERY_FIELDS, &rd))) {
		printf("    forward: ");
		for (int i = 0; i < rd.route_count; i++) {
			if (rd.snr_towards_count >  i) {
				printf("%s!%08x (%.2f)", (i) ? " --> " : "", rd.route[i], (float)rd.snr_towards[i] / 4.0f);
			} else {
				printf("%s!%08x (?)", (i) ? " --> " : "", rd.route[i]);
			}
		}

		if (rd.route_back_count) {
			printf("\n    reverse: ");
			for (int i = 0; i < rd.route_back_count; i++) {
				if (rd.snr_back_count >  i) {
					printf("%s!%08x (%.2f)", (i) ? " --> " : "", rd.route_back[i], (float)rd.snr_back[i] / 4.0f);
				} else {
					printf("%s!%08x (?)", (i) ? " --> " : "", rd.route_back[i]);
				}
			}
		}

		printf("\n");
		ret = 0;

	} else {
		printf("    (decoding failed: %s)\n", PB_GET_ERROR(s));
		ret = -1;
	}

	return ret;
}


static int decode_portnum(void *buf, size_t len, meshtastic_port_num_t portnum)
{
	pb_istream_t s;
	s = pb_istream_from_buffer(buf, len);

	switch (portnum) {
	case MESHTASTIC_PORT_NUM_NODEINFO_APP:
		return dump_nodeinfo(&s);
		break;

	case MESHTASTIC_PORT_NUM_TELEMETRY_APP:
		return dump_telemetry(&s);
		break;

	case MESHTASTIC_PORT_NUM_TRACEROUTE_APP:
		return dump_traceroute(&s);
		break;

	case MESHTASTIC_PORT_NUM_NEIGHBORINFO_APP:
		break;

	case MESHTASTIC_PORT_NUM_POSITION_APP:
		return dump_position(&s);
		break;

	case MESHTASTIC_PORT_NUM_TEXT_MESSAGE_APP:
		return dump_text(buf, len);
		break;

	case MESHTASTIC_PORT_NUM_REMOTE_HARDWARE_APP:
	case MESHTASTIC_PORT_NUM_ROUTING_APP:
	case MESHTASTIC_PORT_NUM_ADMIN_APP:
	case MESHTASTIC_PORT_NUM_TEXT_MESSAGE_COMPRESSED_APP:
	case MESHTASTIC_PORT_NUM_WAYPOINT_APP:
	case MESHTASTIC_PORT_NUM_AUDIO_APP:
	case MESHTASTIC_PORT_NUM_DETECTION_SENSOR_APP:
	case MESHTASTIC_PORT_NUM_REPLY_APP:
	case MESHTASTIC_PORT_NUM_PAXCOUNTER_APP:
	case MESHTASTIC_PORT_NUM_SERIAL_APP:
	case MESHTASTIC_PORT_NUM_MAP_REPORT_APP:
	case MESHTASTIC_PORT_NUM_ATAK_PLUGIN:
	case MESHTASTIC_PORT_NUM_ATAK_FORWARDER:
	default:
		printf("  (don't know how to decode %d (%s) yet)\n", portnum, _portnum_str(portnum));
		break;
	};

	return -1;
}


/* 16 bytes of random PSK for our _public_ default channel that all devices power up on (AES128) */
static const uint8_t default_psk[] = {
	0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59, 0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01
};


typedef struct {
	uint8_t ctr_start;
	uint8_t idx;
	uint8_t ctr[16];
	uint8_t state[16];
	uint8_t schedule[16];
} aes128_ctx_t;

#define OUT(col, row)   output[(col) * 4 + (row)]
#define IN(col, row)    input[(col) * 4 + (row)]

#define gmul2(x)    (t = ((uint16_t)(x)) << 1, ((uint8_t)t) ^ (uint8_t)(0x1B * ((uint8_t)(t >> 8))))

#define KCORE(n) \
	do { \
		keyScheduleCore(temp, schedule + 12, (n)); \
		schedule[0] ^= temp[0]; \
		schedule[1] ^= temp[1]; \
		schedule[2] ^= temp[2]; \
		schedule[3] ^= temp[3]; \
	} while (0)

#define KXOR(a, b) \
	do { \
		schedule[(a) * 4] ^= schedule[(b) * 4]; \
		schedule[(a) * 4 + 1] ^= schedule[(b) * 4 + 1]; \
		schedule[(a) * 4 + 2] ^= schedule[(b) * 4 + 2]; \
		schedule[(a) * 4 + 3] ^= schedule[(b) * 4 + 3]; \
	} while (0)

static uint8_t const sbox[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};


static void subBytesAndShiftRows(uint8_t *output, const uint8_t *input)
{
	OUT(0, 0) = (*(sbox + IN(0, 0)));
	OUT(0, 1) = (*(sbox + IN(1, 1)));
	OUT(0, 2) = (*(sbox + IN(2, 2)));
	OUT(0, 3) = (*(sbox + IN(3, 3)));
	OUT(1, 0) = (*(sbox + IN(1, 0)));
	OUT(1, 1) = (*(sbox + IN(2, 1)));
	OUT(1, 2) = (*(sbox + IN(3, 2)));
	OUT(1, 3) = (*(sbox + IN(0, 3)));
	OUT(2, 0) = (*(sbox + IN(2, 0)));
	OUT(2, 1) = (*(sbox + IN(3, 1)));
	OUT(2, 2) = (*(sbox + IN(0, 2)));
	OUT(2, 3) = (*(sbox + IN(1, 3)));
	OUT(3, 0) = (*(sbox + IN(3, 0)));
	OUT(3, 1) = (*(sbox + IN(0, 1)));
	OUT(3, 2) = (*(sbox + IN(1, 2)));
	OUT(3, 3) = (*(sbox + IN(2, 3)));
}

static void mixColumn(uint8_t *output, uint8_t *input)
{
	uint16_t t; /* Needed by the gmul2 macro */

	uint8_t a = input[0];
	uint8_t b = input[1];
	uint8_t c = input[2];
	uint8_t d = input[3];

	uint8_t a2 = gmul2(a);
	uint8_t b2 = gmul2(b);
	uint8_t c2 = gmul2(c);
	uint8_t d2 = gmul2(d);

	output[0] = a2 ^ b2 ^ b ^ c ^ d;
	output[1] = a ^ b2 ^ c2 ^ c ^ d;
	output[2] = a ^ b ^ c2 ^ d2 ^ d;
	output[3] = a2 ^ a ^ b ^ c ^ d2;
}

static void keyScheduleCore(uint8_t *output, const uint8_t *input, uint8_t iteration)
{
	/*
	 * Rcon(i), 2^i in the Rijndael finite field, for i = 0..10.
	 * http://en.wikipedia.org/wiki/Rijndael_key_schedule
	 */
	static uint8_t const rcon[11] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

	output[0] = (*(sbox + input[1])) ^ (*(rcon + iteration));
	output[1] = (*(sbox + input[2]));
	output[2] = (*(sbox + input[3]));
	output[3] = (*(sbox + input[0]));
}

static void encryptBlock(aes128_ctx_t *ctx)
{
	uint8_t schedule[16];
	uint8_t state1[16];
	uint8_t state2[16];
	uint8_t temp[4];
	uint8_t i, round;

	/* Start with the key in the schedule buffer */
	memcpy(schedule, ctx->schedule, 16);

	/* Copy the ctr into the state and XOR with the key schedule */
	for (i = 0; i < 16; i++) {
		state1[i] = ctx->ctr[i] ^ schedule[i];
	}

	/* Perform the first 9 rounds of the cipher. */
	for (round = 1; round <= 9; round++) {
		/* Expand the next 16 bytes of the key schedule */
		KCORE(round);
		KXOR(1, 0);
		KXOR(2, 1);
		KXOR(3, 2);

		/* Encrypt using the key schedule */
		subBytesAndShiftRows(state2, state1);
		mixColumn(state1,      state2);
		mixColumn(state1 + 4,  state2 + 4);
		mixColumn(state1 + 8,  state2 + 8);
		mixColumn(state1 + 12, state2 + 12);
		for (i = 0; i < 16; ++i) {
			state1[i] ^= schedule[i];
		}
	}

	/* Expand the final 16 bytes of the key schedule */
	KCORE(10);
	KXOR(1, 0);
	KXOR(2, 1);
	KXOR(3, 2);

	/* Perform the final round */
	subBytesAndShiftRows(state2, state1);
	for (i = 0; i < 16; i++) {
		ctx->state[i] = state2[i] ^ schedule[i];
	}
}

void aes128_init(aes128_ctx_t *ctx)
{
	ctx->idx = 16;
	ctx->ctr_start = 0;
}

void aes128_set_ctrlen(aes128_ctx_t *ctx, size_t len)
{
	ctx->ctr_start = 16 - len;
}

void aes128_set_iv(aes128_ctx_t *ctx, const uint8_t *iv, size_t len)
{
	memcpy(ctx->ctr, iv, len);
	ctx->idx = 16;
}

void aes128_set_key(aes128_ctx_t *ctx, const uint8_t *key, size_t len)
{
	memcpy(ctx->schedule, key, 16);
}

void aes128_crypt(aes128_ctx_t *ctx, uint8_t *output, const uint8_t *input, size_t len)
{
	while (len > 0) {
		uint8_t templen;

		if (ctx->idx >= 16) {
			/* Generate a new encrypted ctr block. */
			encryptBlock(ctx);
			ctx->idx = 0;

			/*
			 * Increment the ctr, taking care not to reveal
			 * any timing information about the starting value.
			 * We iterate through the entire ctr region even
			 * if we could stop earlier because a byte is non-zero.
			 */
			uint16_t temp = 1;
			uint8_t i = 16;
			while (i > ctx->ctr_start) {
				--i;
				temp += ctx->ctr[i];
				ctx->ctr[i] = (uint8_t)temp;
				temp >>= 8;
			}
		}

		templen = 16 - ctx->idx;
		if (templen > len) {
			templen = len;
		}

		len -= templen;
		while (templen > 0) {
			*output++ = *input++ ^ ctx->state[ctx->idx++];
			--templen;
		}
	}
}


/*
 * generate our 128 bit nonce for a new packet
 *
 * The nonce is constructed by concatenating (from MSB to LSB):
 * a 64 bit packet number (stored in little endian order)
 * a 32 bit sending node number (stored in little endian order)
 * a 32 bit block counter (starts at zero)
 *
 * nonce pointer must be able to hold 16 bytes
 */
static void gen_nonce(uint8_t *nonce, uint32_t from_nodeid, uint32_t packet_id)
{
	memset(nonce, 0, 16);

	/* use memcpy to avoid breaking strict-aliasing */
	memcpy(nonce, &packet_id, sizeof(packet_id));
	memcpy(nonce + sizeof(uint64_t), &from_nodeid, sizeof(from_nodeid));
}


/* try to decrypt the given packet. if out is NULL, copy the decrypted payload back to the packet structure */
static void mesh_decrypt(uint32_t src, uint32_t id, uint8_t *buf, size_t len)
{
	aes128_ctx_t ctx;
	uint8_t nonce[16];
	static uint8_t crypt_buffer[256];
	const uint8_t payload_len = len;
	uint8_t crypt_len;

	aes128_init(&ctx);
	aes128_set_ctrlen(&ctx, 4);
	aes128_set_key(&ctx, default_psk, sizeof(default_psk));

	gen_nonce(nonce, src, id);
	aes128_set_iv(&ctx, nonce, sizeof(nonce));

	/* pad the crypt_buffer with zeroes up to the next multiple of 16 bytes */
	crypt_len = (payload_len + 15) & -16;
	memcpy(crypt_buffer, buf, payload_len);
	memset(crypt_buffer + payload_len, 0, crypt_len - payload_len);

	if (debug) {
		printf("nonce"); for (int i = 0; i < 16; i++) printf(" %02hhx", nonce[i]); printf("\n");
		printf("key  "); for (int i = 0; i < 16; i++) printf(" %02hhx", default_psk[i]); printf("\n");
		printf("enc  "); for (int i = 0; i < crypt_len; i++) printf(" %02hhx", crypt_buffer[i]); printf("\n");
	}

	aes128_crypt(&ctx, crypt_buffer, crypt_buffer, crypt_len);

	if (debug) {
		printf("dec  "); for (int i = 0; i < crypt_len; i++) printf(" %02hhx", crypt_buffer[i]); printf("\n");
	}

	memcpy(buf, crypt_buffer, payload_len);
}


struct compression_stats {
	uint8_t portnum;
	int num, num_interval;			/* raw count of number of packets (in this interval) of this type */
	int unc_len_min, comp_len_min;		/* minimum length (and what it compressed to) */
	int unc_len_max, comp_len_max;		/* maximum length (and what it compressed to) */
	float unc_len_avg, comp_ratio_avg;	/* average length and compression ratio */
};

static void test_compression(meshtastic_data_t *md)
{
	static bool first = true;
	static time_t t1;
	static uint32_t total_packets, total_this_run;
	static struct compression_stats cstats[256];
	struct compression_stats *cs;

	/* "weight" for new data coming into the EMA filter */
	const float cs_alpha = 0.1f;

	int ret;
	float cdf[CDF_MAX_SYMB];
	size_t nsym;

	/* original data source */
	const void *buf = md->payload.bytes;
	const size_t len = md->payload.size;

	/* compressed output buffer */
	uint8_t out[CDF_MAX_SYMB], *outp = out;
	size_t nout = sizeof(out);

	/* uncompressed output buffer (for testing decompression) */
	uint8_t unc[CDF_MAX_SYMB], *uncp = unc;
	size_t nunc = sizeof(unc);

	/* first time through, initialize the compression stats array */
	if (first) {
		for (int i = 0; i < sizeof(cstats)/sizeof(cstats[0]); i++) {
			cs = &cstats[i];
			cs->num = cs->num_interval = 0;
			cs->unc_len_min = cs->comp_len_min = -1;
			cs->unc_len_max = cs->comp_len_max = 0;
			cs->unc_len_avg = cs->comp_ratio_avg = 0.0f;
		}

		time(&t1);
		total_packets = total_this_run = 0;
		first = false;
	}

	nsym = 0;
	if (cdf_build(cdf, &nsym, (uint8_t *)buf, len) == NULL) {
		printf("  ** building CDF failed\n");
		return;
	}

	if ((ret = encode_u8_u8((void **)&outp, &nout, (void *)buf, len, cdf, nsym)) == 0) {
		if ((ret = decode_u8_u8((void **)&uncp, &nunc, out, nout, cdf, nsym)) == 0) {
			if (nunc == len && memcmp(buf, uncp, len) == 0) {
				cs = &cstats[md->portnum];
				cs->portnum = md->portnum;
				++cs->num;
				++cs->num_interval;

				/* don't count packets that didn't compress in the min/max/avg calculations */
				float ratio = 100.0f - (100.0f * (float)nout / (float)len);
				if (ratio > 1.0f) {
					if (cs->num == 1) {
						cs->unc_len_avg = (float)len;
						cs->comp_ratio_avg = (float)ratio;
					} else {
						cs->unc_len_avg = (1.0f - cs_alpha) * cs->unc_len_avg + (cs_alpha * (float)len);
						cs->comp_ratio_avg = (1.0f - cs_alpha) * cs->comp_ratio_avg + (cs_alpha * ratio);
					}

					/* update the min original/compressed sizes as a pair */
					if (len < cs->unc_len_min || cs->num == 1) {
						cs->unc_len_min = len;
						cs->comp_len_min = nout;
					}

					/* update the max original/compressed sizes as a pair */
					if (len > cs->unc_len_max || cs->num == 1) {
						cs->unc_len_max = len;
						cs->comp_len_max = nout;
					}

					printf("    %20s: %3.2f%% (%zd symbols: %zd -> %zd bytes) best: %d -> %d, worst: %d -> %d, avg %.1f bytes, avg ratio %3.2f%% over %d packets\n", _portnum_str(md->portnum), ratio, nsym, nunc, nout, cs->unc_len_min, cs->comp_len_min, cs->unc_len_max, cs->comp_len_max, cs->unc_len_avg, cs->comp_ratio_avg, cs->num);
				}

			} else {
				fprintf(stderr, "  ** decompression succeeded but output does not match original data!\n");
				fprintf(stderr, "  original data: ");
				for (int i = 0; i < len; i++) { fprintf(stderr, "%02hhx ", ((uint8_t *)buf)[i]); } fprintf(stderr, "\n");
				fprintf(stderr, "  compressed data: ");
				for (int i = 0; i < nout; i++) { fprintf(stderr, "%02hhx ", out[i]); } fprintf(stderr, "\n");
				fprintf(stderr, "  uncompressed data: ");
				for (int i = 0; i < nunc; i++) { fprintf(stderr, "%02hhx ", unc[i]); } fprintf(stderr, "\n\n");
			}

		} else {
			printf("  ** decompression failed\n");
		}

	} else {
		printf("  ** compression failed\n");
	}

	++total_packets;
	++total_this_run;
	if (total_this_run >= 1000) {
		time_t t2, dt;
		time(&t2);
		dt = difftime(t2, t1);
		printf("\n\nCOMPRESSION STATS (%d packets total, %d in the last %s):\n", total_packets, total_this_run, time_str(dt));
		for (int i = 0; i < sizeof(cstats)/sizeof(cstats[0]); i++) {
			cs = &cstats[i];

			if (cs->num > 0) {
				printf("%20s: min: %d -> %d, max: %d -> %d, avg unc. length %.1f bytes, avg comp. ratio %3.2f%% over %d packets (%d in this interval), %.1f%%/%.1f%% of all packets this interval/ever\n", _portnum_str(cs->portnum), cs->unc_len_min, cs->comp_len_min, cs->unc_len_max, cs->comp_len_max, cs->unc_len_avg, cs->comp_ratio_avg, cs->num, cs->num_interval, 100.0f * cs->num_interval / total_this_run, 100.0f * cs->num / total_packets);
			}

			cs->num_interval = 0;
		}

		total_this_run = 0;
		time(&t1);
		printf("\n");
	}
}


/* maintains a short list of recently seen packet IDs. Returns true if this packet ID was seen already */
static bool is_duplicate_packetid(uint32_t packet_id)
{
	static uint32_t id_list[32], idx;

	/* don't count multiple messages about the same packet ID */
	for (int i = 0; i < sizeof(id_list)/sizeof(id_list[0]); i++) {
		if (id_list[i] == packet_id) {
			return true;
		}
	}

	id_list[idx++] = packet_id;
	idx %= sizeof(id_list)/sizeof(id_list[0]);
	return false;
}


/* MQTT received message callback */
static void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg)
{
	if (verbose) {
		printf("\nReceived message on %s len %d:\n", msg->topic, msg->payloadlen);
	}

	if (msg->payloadlen) {
		meshtastic_service_envelope_t e = MESHTASTIC_SERVICE_ENVELOPE_INIT_DEFAULT;

		pb_istream_t s = pb_istream_from_buffer(msg->payload, msg->payloadlen);
		if (pb_decode(&s, MESHTASTIC_SERVICE_ENVELOPE_FIELDS, &e)) {
			meshtastic_mesh_packet_t *p = e.packet;

			if (verbose) {
				printf("Decoded ServiceEnvelope:\nChannel ID: %s\nGateway ID: %s\n", e.channel_id, e.gateway_id);
			}

			if (p->which_payload_variant == MESHTASTIC_MESH_PACKET_ENCRYPTED_TAG) {
				if (verbose || dump) {
					printf("Packet:\n  From: !%08x\n  To: !%08x\n  ID: 0x%08x\n  Channel: %u\n", p->from, p->to, p->id, p->channel);
				}

				/* only interested in default (LongFast) traffic and assume the default encryption key is used */
				if (p->channel == 8 && p->encrypted.size > 0) {
					if (is_duplicate_packetid(p->id) == false) {
						mesh_decrypt(p->from, p->id, (uint8_t *)&p->encrypted.bytes, p->encrypted.size);

						meshtastic_data_t md = MESHTASTIC_DATA_INIT_DEFAULT;
						pb_istream_t md_s = pb_istream_from_buffer((uint8_t *)&p->encrypted.bytes, p->encrypted.size);
						if (pb_decode(&md_s, MESHTASTIC_DATA_FIELDS, &md)) {
							if (dump) {
								printf("  Decoded meshdata packet:\n");
								printf("    Portnum: %d (%s)\n", md.portnum, _portnum_str(md.portnum));
								printf("    Payload size: %d bytes\n", md.payload.size);
								printf("    Decrypted Payload: ");
								for (int i = 0; i < md.payload.size; i++) {
									printf("%02hhx ", md.payload.bytes[i]);
								}
								printf("\n");
							}

							if (md.payload.size > 0) {
								if (dump) {
									decode_portnum(md.payload.bytes, md.payload.size, md.portnum);
								}

								test_compression(&md);
							}

						} else {
							printf("    (failed to decode decrypted protobuf)");
						}

					} else {
						/* this message is a dupliate from another MQTT client which uplinked it */
					}

				} else {
					/* not from the default channel or zero payload */
				}

			} else {
				/* not an encrypted packet */
			}

		} else {
			printf("Failed to decode ServiceEnvelope: %s\n", PB_GET_ERROR(&s));
		}

	} else {
		/* empty message? */
	}
}


/* MQTT connect callback */
static void on_connect(struct mosquitto *m, void *ctx, int rc)
{
	struct user_context *u = (struct user_context *)ctx;

	if (rc == 0) {
		printf("Connected to MQTT broker\n");
		if ((rc = mosquitto_subscribe(m, NULL, u->topic, 0)) == MOSQ_ERR_SUCCESS) {
			printf("Subscribed to %s\n", u->topic);

		} else {
			fprintf(stderr, "Subscribe failed: %s\n", mosquitto_strerror(rc));
		}

	} else {
		printf("Failed to connect (%d)\n", rc);
	}
}


int main(int argc, char *argv[])
{
	verbose = debug = dump = false;

	if (argc < 6) {
		fprintf(stderr, "Usage: %s <broker_host> <port> <topic> <username> <password> [ca_file]\n", argv[0]);
		return -1;
	}

	const char *host = argv[1];
	int port = atoi(argv[2]);
	const char *topic = argv[3];
	const char *username = argv[4];
	const char *password = argv[5];
	const char *cafile = argc > 6 ? argv[6] : NULL;
	char client_id[32];
	time_t t;

	struct user_context context = {
		.topic = topic
	};

	mosquitto_lib_init();

	time(&t);
	srand((int)t);
	snprintf(client_id, sizeof(client_id), "compression_test-%u", rand());
	struct mosquitto *m = mosquitto_new(client_id, true, &context);
	if (!m) {
		fprintf(stderr, "Error: Out of memory\n");
		return -1;
	}

	if (mosquitto_username_pw_set(m, username, password) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error setting credentials\n");
		mosquitto_destroy(m);
		mosquitto_lib_cleanup();
		return -1;
	}

	// Set TLS options if CA file is provided
	if (cafile) {
		int rc = mosquitto_tls_set(m, cafile, NULL, NULL, NULL, NULL);
		if (rc != MOSQ_ERR_SUCCESS) {
			fprintf(stderr, "Error: TLS setup failed: %s\n", mosquitto_strerror(rc));
			mosquitto_destroy(m);
			mosquitto_lib_cleanup();
			return 1;
		}

		// Force TLSv1.2
		mosquitto_tls_opts_set(m, 1, "tlsv1.2", NULL);
	}

	mosquitto_connect_callback_set(m, on_connect);
	mosquitto_message_callback_set(m, on_message);

	int rc = mosquitto_connect(m, host, port, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error: Could not connect to broker: %s\n", mosquitto_strerror(rc));
		mosquitto_destroy(m);
		mosquitto_lib_cleanup();
		return -1;
	}

	printf("Connecting to %s:%d\n", host, port);
	mosquitto_loop_forever(m, -1, 1);

	mosquitto_destroy(m);
	mosquitto_lib_cleanup();
	return 0;
}

