#include <string.h>
#include <limits.h>
#include <stdio.h>

#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>

#include "../../ma_fingerprint.h"

#define LOG_TAG "test_microarray_fp"

#define HAL_LIBRARY_PATH1 "/lib/libfprint-mips.so"
#define HAL_LIBRARY_PATH2 "/usr/lib/libfprint-mips.so"

typedef enum {
    DELETE_ALL,
    DELETE_BY_ID,
} delete_type_t;

enum {
    ENROLL_TEST,
    AUTH_TEST,
    LIST_TEST,
    DELETE_TEST,
    EXIT_TEST,
    TEST_MAX,
};

enum {
    STATE_IDLE,
    STATE_BUSY
};

static struct signal_handler* signal_handler;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int work_state = STATE_IDLE;
static int enroll_steps;
fingerprint_finger_id_t fingers[FINGERPRINT_SIZE];
uint32_t finger_count = 0;

static const char* test2string(int item) {
    switch (item) {
    case ENROLL_TEST:
        return "enroll test";
    case AUTH_TEST:
        return "authenticate test";
    case LIST_TEST:
        return "list fingers test";
    case DELETE_TEST:
        return "delete test";
    case EXIT_TEST:
        return "exit";
    default:
        return "unknown test";
    }
}

static void print_tips(void) {
    fprintf(stderr, "\n==================== Microarray FP ===================\n");
    fprintf(stderr, "  %d.Enroll\n", ENROLL_TEST);
    fprintf(stderr, "  %d.Authenticate\n", AUTH_TEST);
    fprintf(stderr, "  %d.List enrolled fingers\n", LIST_TEST);
    fprintf(stderr, "  %d.Delete\n", DELETE_TEST);
    fprintf(stderr, "  %d.Exit\n", EXIT_TEST);
    fprintf(stderr, "======================================================\n");
}

static void print_delete_tips(void) {
    fprintf(stderr, "\n==============================================\n");
    fprintf(stderr, "Please select delete type.\n");
    fprintf(stderr, "  %d.Delete all\n", DELETE_ALL);
    fprintf(stderr, "  %d.Delete by id\n", DELETE_BY_ID);
    fprintf(stderr, "==============================================\n");
}

static void print_delete_id_tips(void) {
    fprintf(stderr, "\n==============================================\n");
    fprintf(stderr, "Please enter finger id.\n");
    fprintf(stderr, "==============================================\n");
}

static void dump_fingers(void) {
    fprintf(stderr, "\n============== Finger List ==============\n");
    for (int i = 0; i < finger_count; i++)
        fprintf(stderr, "Finger[%d]: %d\n", i, fingers[i].fid);

    fprintf(stderr, "=========================================\n");
}

static void set_state_idle(void) {
    pthread_mutex_lock(&lock);

    work_state = STATE_IDLE;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void set_state_busy(void) {
    pthread_mutex_lock(&lock);

    work_state = STATE_BUSY;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void wait_state_idle(void) {
    pthread_mutex_lock(&lock);

    while (work_state == STATE_BUSY) {
        pthread_cond_wait(&cond, &lock);
        //msleep(200);
    }

    pthread_mutex_unlock(&lock);
}

static void ma_fp_callback(const fingerprint_msg_t *msg) {
    switch (msg->type) {
    case FINGERPRINT_ERROR:
        LOGI("========> FINGERPRINT_ERROR: error=%d\n", msg->data.error);
        set_state_idle();
        break;

    case FINGERPRINT_ACQUIRED:
        LOGD("========> FINGERPRINT_ACQUIRED: acquired_info=%d\n",
                msg->data.acquired.acquired_info);

        if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_GOOD)
            LOGI("Finger good\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_FINGER_DOWN)
            LOGI("Finger down\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_FINGER_UP)
            LOGI("Finger up\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_DUPLICATE_AREA)
            LOGW("Duplicate finger area\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_IMAGER_DIRTY)
            LOGW("Sensor may be dirty\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_LOW_COVER)
            LOGW("Sensor low cover\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_DUPLICATE_FINGER) {
            LOGW("Duplicate finger\n");
            ma_fingerprint_cancel();
            set_state_idle();
        }

        break;

    case FINGERPRINT_AUTHENTICATED:
        LOGD("========> FINGERPRINT_AUTHENTICATED: fid=%d\n",
                msg->data.authenticated.finger.fid);
        if (msg->data.authenticated.finger.fid == 0)
            LOGE("Finger auth failure!\n");
        else
            LOGI("Finger auth success, id=%d!\n", msg->data.authenticated.finger.fid);
        set_state_idle();
        break;

    case FINGERPRINT_TEMPLATE_ENROLLING:
        LOGD("========> FINGERPRINT_TEMPLATE_ENROLLING: fid=%d, rem=%d\n",
                msg->data.enroll.finger.fid,
                msg->data.enroll.samples_remaining);

        if (enroll_steps == -1)
            enroll_steps = msg->data.enroll.samples_remaining;

        int progress = (enroll_steps + 1 - msg->data.enroll.samples_remaining);

        LOGI("Finger enrolling %d%%\n", 100 * progress / (enroll_steps + 1));

        if (msg->data.enroll.samples_remaining == 0)
            set_state_idle();
        break;

    case FINGERPRINT_TEMPLATE_REMOVED:
        LOGD("========> FINGERPRINT_TEMPLATE_REMOVED: fid=%d\n",
                msg->data.removed.finger.fid);
        LOGI("Finger removed, id=%d\n", msg->data.removed.finger.fid);
        set_state_idle();
        break;

    default:
        LOGE("Invalid msg type: %d", msg->type);
        break;
    }
}

static void handle_signal(int signal) {
    int error = 0;

    error = ma_fingerprint_close();
    if (error)
        LOGE("Failed to close microarray library\n");

    exit(1);
}

static int do_enroll(void) {
    int error = 0;

    enroll_steps = -1;

    error = ma_fingerprint_enroll();
    if (error) {
        LOGE("Failedl to enroll fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_auth(void) {
    int error = 0;

    error = ma_fingerprint_enumerate(fingers, &finger_count);
    if (error) {
        LOGE("Failed to enumerate fingers\n");
        return -1;
    }

    if (finger_count <= 0) {
        LOGW("No any valid finger's templete\n");
        set_state_idle();
        return 0;
    }

    error = ma_fingerprint_authenticate();
    if (error) {
        LOGE("Failed to auth fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_list(void) {
    int error = 0;

    error = ma_fingerprint_enumerate(fingers, &finger_count);
    if (error) {
        LOGE("Failed to enumerate fingers\n");
        return -1;
    }
    dump_fingers();

    return 0;
}

static int do_delete(void) {
    int error = 0;
    char id_buf[128] = {0};
    char type_buf[32] = {0};

restart:
    print_delete_tips();
    if (fgets(type_buf, sizeof(type_buf), stdin) == NULL)
        goto restart;
    if (strlen(type_buf) != 2)
        goto restart;

    int type = strtol(type_buf, NULL, 0);
    if (type > DELETE_BY_ID || type < 0)
        goto restart;

    if (type == DELETE_BY_ID) {
restart2:
        print_delete_id_tips();

        if (fgets(id_buf, sizeof(id_buf), stdin) == NULL)
            goto restart2;
        uint32_t id = strtol(id_buf, NULL, 0);

        error = ma_fingerprint_remove(id);
        if (error)
            LOGE("Failed to delete finger by id %d\n", id);

    } else if (type == DELETE_ALL) {
        error = ma_fingerprint_remove(type);
        if (error)
            LOGE("Failed to delete all finger\n");
    }

    return error;
}

static int do_exit(void) {
    int error = 0;

    error = ma_fingerprint_close();
    if (error)
        LOGE("Failed to close microarray library\n");

    exit(error);
}

static void do_work(void) {
    int error = 0;
    char sel_buf[64] = {0};

    setbuf(stdin, NULL);

    do_list();

    for (;;) {
restart:
        wait_state_idle();

        print_tips();

        if (fgets(sel_buf, sizeof(sel_buf), stdin) == NULL)
            goto restart;

        if (strlen(sel_buf) != 2)
            goto restart;

        int action = strtol(sel_buf, NULL, 0);
        if (action >= TEST_MAX || action < 0)
            goto restart;

        LOGI("Going to %s\n", test2string(action));

        if (action != LIST_TEST && action != EXIT_TEST)
            set_state_busy();

        switch(action) {
        case ENROLL_TEST:
            error = do_enroll();
            break;

        case AUTH_TEST:
            error = do_auth();
            break;

        case LIST_TEST:
            error = do_list();
            break;

        case DELETE_TEST:
            error = do_delete();
            break;

        case EXIT_TEST:
            error = do_exit();
            break;

        default:
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int error = 0;

    signal_handler = _new(struct signal_handler, signal_handler);

    signal_handler->set_signal_handler(signal_handler, SIGINT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGQUIT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGTERM, handle_signal);

    error = ma_fingerprint_set_notify_callback(ma_fp_callback);
    if (error) {
        LOGE("Failed to set callback\n");
        return -1;
    }

    error = ma_fingerprint_open(get_user_system_dir(getuid()));
    if (error) {
        LOGE("Failed to open microarray lib\n");
        return -1;
    }

    do_work();

    while (1)
        sleep(1000);

    return 0;
}