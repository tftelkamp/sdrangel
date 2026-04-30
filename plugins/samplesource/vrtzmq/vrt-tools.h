/* VRT tools helper functions */

#ifndef INCLUDE_VRTTOOLS_H
#define INCLUDE_VRTTOOLS_H

#define VRT_SAMPLES_PER_PACKET 10000

#define SIZE (VRT_SAMPLES_PER_PACKET+7)
#define VRT_DATA_PACKET_SIZE (VRT_SAMPLES_PER_PACKET+7)

#define ZMQ_BUFFER_SIZE 100000

#define MAX_CHANNELS    10

#define DEFAULT_MAIN_PORT       50100
#define DEFAULT_GNURADIO_PORT   (DEFAULT_MAIN_PORT+100)
#define DEFAULT_CONTROL_PORT    (DEFAULT_MAIN_PORT+200)
#define DEFAULT_TX_PORT         (DEFAULT_MAIN_PORT+400)

// Context update interval in ms
#define VRT_CONTEXT_INTERVAL 200

// VRT
#include <vrt/vrt_init.h>
#include <vrt/vrt_string.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>
#include <vrt/vrt_write.h>
#include <vrt/vrt_read.h>

#include <complex>

#ifdef __cplusplus
extern "C" {
#endif

struct context_type {
    bool context_received;
    bool context_changed;
    int64_t rf_freq;
    double rf_frac_freq;
    uint32_t sample_rate;
    int32_t gain;
    float temperature;
    uint32_t bandwidth;
    bool reflock;
    bool time_cal;
    uint32_t stream_id;
    uint64_t starttime_integer;
    uint64_t starttime_fractional;
    int32_t last_data_counter;
    uint64_t fractional_seconds_timestamp;
    uint64_t integer_seconds_timestamp;
    uint32_t timestamp_calibration_time;
};

struct packet_type {
    bool context;
    bool data;
    bool extended_context;
    bool lost_frame;
    bool first_frame;
    uint32_t oui;
    uint16_t information_class_code;
    uint16_t packet_class_code;
    uint32_t stream_id;
    uint32_t channel_filt;
    uint32_t num_rx_samps;
    uint32_t offset;
    uint64_t fractional_seconds_timestamp;
    uint64_t integer_seconds_timestamp;
};

void init_context(context_type* context);
bool check_packet_count(int8_t counter, context_type* vrt_context);
void vrt_print_context(context_type* vrt_context);
bool vrt_process(uint32_t* buffer, uint32_t size, context_type* vrt_context, packet_type* vrt_packet);
void vrt_init_data_packet(struct vrt_packet* p);
void vrt_init_context_packet(struct vrt_packet* pc);

#ifdef __cplusplus
}
#endif

#endif
