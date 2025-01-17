/*
 * This file is part of cyanrip.
 *
 * cyanrip is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * cyanrip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with cyanrip; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../config.h"

#include <cdio/paranoia/paranoia.h>
#include <discid/discid.h>
#include <musicbrainz5/mb5_c.h>
#include <libavutil/mem.h>
#include <libavutil/dict.h>
#include <libavutil/avstring.h>
#include <libavutil/intreadwrite.h>

enum cyanrip_output_formats {
    CYANRIP_FORMAT_FLAC,
    CYANRIP_FORMAT_TTA,
    CYANRIP_FORMAT_OPUS,
    CYANRIP_FORMAT_AAC,
    CYANRIP_FORMAT_WAVPACK,
    CYANRIP_FORMAT_ALAC,
    CYANRIP_FORMAT_MP3,
    CYANRIP_FORMAT_VORBIS,
    CYANRIP_FORMAT_WAV,
    CYANRIP_FORMAT_AAC_MP4,
    CYANRIP_FORMAT_OPUS_MP4,
    CYANRIP_FORMAT_PCM,

    CYANRIP_FORMATS_NB,
};

enum cyanrip_pregap_action {
    CYANRIP_PREGAP_DEFAULT = 0,
    CYANRIP_PREGAP_DROP,
    CYANRIP_PREGAP_MERGE,
    CYANRIP_PREGAP_TRACK,
};

typedef struct cyanrip_settings {
    char *dev_path;
    char *base_dst_folder;
    int speed;
    int frame_max_retries;
    int offset;
    int over_under_read_frames;
    int print_info_only;
    int disable_mb;
    float bitrate;
    int decode_hdcd;
    int enc_fifo_size;
    int overread_leadinout;
    int eject_on_success_rip;
    enum cyanrip_pregap_action pregap_action[99];
    int rip_indices_count;
    int rip_indices[99];

    enum cyanrip_output_formats outputs[CYANRIP_FORMATS_NB];
    int outputs_num;
} cyanrip_settings;

typedef struct cyanrip_track {
    int number;
    AVDictionary *meta; /* Disc's AVDictionary gets copied here */

    int track_is_data;
    int preemphasis;

    size_t nb_samples; /* Track duration in samples */

    int frames_before_disc_start;
    lsn_t frames; /* Actual number of frames to read, != samples */
    int frames_after_disc_end;

    lsn_t pregap_lsn;
    lsn_t start_lsn;
    lsn_t start_lsn_sig;
    lsn_t end_lsn;
    lsn_t end_lsn_sig;

    ptrdiff_t partial_frame_byte_offs;

    int computed_crcs;
    uint32_t eac_crc;
    uint32_t acurip_crc_v1;
    uint32_t acurip_crc_v2;
} cyanrip_track;

typedef struct cyanrip_ctx {
    cdrom_drive_t     *drive;
    cdrom_paranoia_t  *paranoia;
    CdIo_t            *cdio;
    FILE              *logfile[CYANRIP_FORMATS_NB];
    cyanrip_settings   settings;

    cyanrip_track tracks[99];
    int nb_tracks;

    /* Drive caps */
    cdio_drive_read_cap_t  rcap;
    cdio_drive_write_cap_t wcap;
    cdio_drive_misc_cap_t  mcap;

    /* Metadata */
    AVDictionary *meta;

    /* Destination folder */
    const char *base_dst_folder;

    int success;
    int total_error_count;
    lsn_t start_lsn;
    lsn_t end_lsn;
    lsn_t duration_frames;
} cyanrip_ctx;

extern uint64_t paranoia_status[PARANOIA_CB_FINISHED + 1];

static inline const char *dict_get(AVDictionary *dict, const char *key)
{
    AVDictionaryEntry *e = av_dict_get(dict, key, NULL, 0);
    return e ? e->value : NULL;
}

static inline void cyanrip_frames_to_duration(uint32_t frames, char *str)
{
    if (!str)
        return;
    const double tot = frames/75.0; /* 75 frames per second */
    const int hr    = tot/3600.0f;
    const int min   = (tot/60.0f) - (hr * 60);
    const int sec   = tot - ((hr * 3600) + min * 60);
    const int msec  = tot - sec;
    snprintf(str, 12, "%02i:%02i:%02i.%i", hr, min, sec, msec);
}

static inline void cyanrip_samples_to_duration(uint32_t samples, char *str)
{
    if (!str)
        return;
    const double tot = samples/44100.0; /* 44100 samples per second */
    const int hr    = tot/3600.0f;
    const int min   = (tot/60.0f) - (hr * 60);
    const int sec   = tot - ((hr * 3600) + min * 60);
    const int msec  = tot - sec;
    snprintf(str, 12, "%02i:%02i:%02i.%i", hr, min, sec, msec);
}

static inline int cmp_numbers(const void *a, const void *b)
{
    return *((int *)a) > *((int *)b);
}

static inline char *cyanrip_sanitize_fn(const char *src)
{
    char *str = av_strdup(src);
    char forbiddenchars[] = "<>:/\\|?*";
    char *ret = str;
    while(*str) {
        if (*str == '"')
            *str = '\'';
        else if (strchr(forbiddenchars, *str))
            *str = '_';
        str++;
    }
    return ret;
}
