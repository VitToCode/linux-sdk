/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include <types.h>
#include <fingerprint/fingerprint.h>
#include <fingerprint/fingerprint_list.h>

struct authentication_result {
    struct fingerprint *fingerprint;
};

struct enrollment_callback {
    void (*on_enrollment_error)(int error_code, const char* error_string);
    void (*on_enrollment_help)(int help_code, const char* help_string);
    void (*on_enrollment_progress)(int remaining);
};

struct remove_callback {
    void (*on_remove_error)(struct fingerprint *fp, int error_code,
            const char* error_string);
    void (*on_remove_successed)(struct fingerprint *fp);
};

struct authentication_callback {
    void (*on_authentication_error)(int error_code, const char* error_string);
    void (*on_authentication_help)(int help_code, const char* help_string);
    void (*on_authentication_successed)(struct authentication_result *result);
    void (*on_authentication_failed)(void);
    void (*on_authentication_acquired)(int acquire_info);
};

struct lockout_reset_callback {
    void (*on_lockout_reset)(void);
};

struct fingerprint_manager {
    int (*init)(void);
    int (*deinit)(void);
    int (*is_hardware_detected)(void);
    int (*has_enrolled_fingerprints)(void);
    void (*authenticate)(struct authentication_callback *callback);
    void (*enroll)(char* token, int len, struct enrollment_callback *callback);
    void (*pre_enroll)(void);
    void (*post_enroll)(void);
    void (*remove_fingerprint)(struct fingerprint *fp, struct remove_callback* callabck);
    void (*rename_fingerprint)(int fp_id, const char* new_name);

    struct fingerprint_list* (*get_enrolled_fingerprints)(void);

    uint64_t (*get_authenticator_id)(void);
    void (*reset_timeout)(char* token, int len);
    void (*add_lockout_reset_callback)(struct lockout_reset_callback* callback);
};

struct fingerprint_manager* get_fingerprint_manager(void);

#endif /* FINGERPRINT_MANAGER_H */
